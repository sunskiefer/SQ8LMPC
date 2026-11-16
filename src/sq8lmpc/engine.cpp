// SQ8LMPC -- mpc_engine glue: the upstream SQ8L engine (Synth + EditBuffer + library) and the headless VFD page
// controller (LcdController) behind the Force's parameters. The panel's knobs, buttons and display are parameters;
// the 44 x 2 display is read back cell by cell (frame numbers of the original glyph strip).
//
// Threading follows wrapper/engine.h: the wrapper never calls two entry points of one instance at once. File I/O
// (saving the user banks, exports) is handed to a worker thread and never done in set_param / get_param / render.
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dirent.h>
#include <memory>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

extern "C" {
#include "engine.h"
#include "plugin_dir.h"
}

#include "EditBuffer.h"
#include "Synth.h"
#include "WaveRom.h"
#include "controls/LcdDisplay.h"
#include "logic/LcdControl.h"
#include "Sprite.h"

using namespace sq8l;

namespace {

enum Kind { PK_PROGRAM, PK_KNOB, PK_PAGE, PK_ACTION, PK_LED, PK_CELL, PK_NUM, PK_PNAME, PK_PACK };
struct Entry { const char* key; Kind kind; int arg; };
const Entry kTable[] = {
#include "param_table.inc"
};
constexpr int kNumParams = int(sizeof kTable / sizeof kTable[0]);

enum Action { A_UP, A_DOWN, A_BANK, A_WRITE, A_INIT, A_SYNC, A_AM, A_MONO, A_PGUP, A_PGDN, A_LOAD, A_EXPORT };

constexpr int kDefaultOutputCutDb = 6;    // one of Settings::kOutputCutsDb; see tests/render.cpp for the peaks behind it
constexpr const char* kLcdCharset =
    " \"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`{|}~";
constexpr const char* kNumCharset = " 0123456789ABCDPU";

// ------------------------------------------------------------------------------------------ files + worker thread
bool makeDirs(const std::string& path) {
    std::string t;
    for (size_t i = 0; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/') {
            if (!t.empty()) mkdir(t.c_str(), 0755);
        }
        if (i < path.size()) t += path[i];
    }
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out.clear();
    uint8_t buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
        out.insert(out.end(), buf, buf + n);
        if (out.size() > (64u << 20)) break;   // nothing here is bigger than a few hundred KB
    }
    fclose(f);
    return true;
}

std::string upper(std::string s) {
    for (char& c : s) c = char(toupper((unsigned char)c));
    return s;
}

// a name safe for a file: letters, digits, space, - and _; never a path separator or a quote
std::string safeName(const std::string& s) {
    std::string r;
    for (char c : s) r += (isalnum((unsigned char)c) || c == '-' || c == '_') ? c : (c == ' ' ? ' ' : '_');
    while (!r.empty() && r.back() == ' ') r.pop_back();
    return r.empty() ? std::string("UNNAMED") : r;
}

class Worker {
public:
    Worker() : th_([this] { run(); }) {}
    ~Worker() {
        {
            std::lock_guard<std::mutex> l(m_);
            stop_ = true;
        }
        cv_.notify_one();
        th_.join();
    }
    // newest write to one path replaces an older queued one
    void write(const std::string& path, std::vector<uint8_t> bytes) {
        {
            std::lock_guard<std::mutex> l(m_);
            for (auto& j : q_)
                if (j.path == path) { j.bytes = std::move(bytes); cv_.notify_one(); return; }
            q_.push_back({path, std::move(bytes)});
        }
        cv_.notify_one();
    }
private:
    struct Job { std::string path; std::vector<uint8_t> bytes; };
    void run() {
        std::unique_lock<std::mutex> l(m_);
        for (;;) {
            cv_.wait(l, [this] { return stop_ || !q_.empty(); });
            while (!q_.empty()) {
                Job j = std::move(q_.front());
                q_.pop_front();
                l.unlock();
                const std::string tmp = j.path + ".tmp";
                if (FILE* f = fopen(tmp.c_str(), "wb")) {
                    const size_t n = fwrite(j.bytes.data(), 1, j.bytes.size(), f);
                    const bool ok = fclose(f) == 0 && n == j.bytes.size();
                    if (ok) rename(tmp.c_str(), j.path.c_str()); else remove(tmp.c_str());
                }
                l.lock();
            }
            if (stop_) return;
        }
    }
    std::mutex m_;
    std::condition_variable cv_;
    std::deque<Job> q_;
    bool stop_ = false;
    std::thread th_;
};

struct Pack {
    std::string name;                // shown on the display (upper case)
    std::string stem;                // file name stem under User/ for this set's saved state
    std::vector<uint8_t> bytes;      // the dropped-in file ("My library": empty)
    int kind = 0;                    // 0 library (A+B), 1 bank (into A), 2 sysex bank (40 programs into A)
};

// ------------------------------------------------------------------------------------------ the instance
class Inst final : public gui::LcdListener, public MasterNotify {
public:
    explicit Inst(const char* dataDir) {
        if (dataDir && dataDir[0]) base_ = dataDir;
        else {
            char here[400];
            if (mpc_plugin_dir(here, sizeof here)) base_ = std::string(here) + "/data";
        }
        if (!base_.empty() && !(makeDirs(base_ + "/User") && makeDirs(base_ + "/Banks") && makeDirs(base_ + "/Export")))
            base_.clear();   // read-only card: the plugin still plays, it just cannot save
        if (!base_.empty()) makeDirs(base_ + "/ROM");
        loadRom();

        synth_ = std::make_unique<Synth>(44100.0f, nullptr, nullptr, this);
        synth_->setOutputCut(kDefaultOutputCutDb);
        synth_->setProgramChangeEnabled(true);

        // headless display: the glyph strip is only needed for its geometry (11 x 18), nothing is painted
        stub_ = std::make_unique<gui::Sprite>("charGIF", 11, 18, 272,
                                              std::vector<gui::Color>(size_t(11) * 18 * 272));
        lcd_ = std::make_unique<gui::LcdDisplay>("lcd");
        lcd_->setBounds(0, 0, 8, 8);
        lcd_->setAniGif(stub_.get());
        lcd_->setCharMap(kLcdCharset, 4);
        lcd_->mapLowerToUpper();
        lcd_->setWidthChar(44);
        lcd_->setHeightChar(2);
        lcd_->setCharGapX(1);
        lcd_->setCharGapY(3);
        ctr_ = std::make_unique<gui::LcdController>(*lcd_, &synth_->editBuffer(), this);
        ctr_->buildPages();
        ctr_->numKnobs = 10;
        ctr_->setRefreshDiv(1);

        worker_ = std::make_unique<Worker>();
        scanPacks();
        loadPack(0, false);
        refreshAll();
        ctr_->selectPage(0);
        refreshAll();
        ctr_->showMessage(-1, -1, "*** SQ8L CROSS WAVE SYNTHESIZER ***", "SQ8LMPC FOR FORCE / MPC", 0x42);
    }

    // ---------------------------------------------------------------- MIDI + audio
    void midi(const uint8_t* m, int len) {
        if (len < 1) return;
        RawMidiEvent ev{0, {m[0], uint8_t(len > 1 ? m[1] : 0), uint8_t(len > 2 ? m[2] : 0)}, 0};
        synth_->processEvents(&ev, 1);
    }

    void render(int16_t* out, int frames) {
        if (frames > 128) frames = 128;
        synth_->process(l_, r_, frames);
        if (!rom_) {   // no wave ROM: the engine runs (pages, programs, saving all work) but stays silent
            for (int i = 0; i < 2 * frames; i++) out[i] = 0;
        } else {
            for (int i = 0; i < frames; i++) {
                out[2 * i] = toS16(l_[i]);
                out[2 * i + 1] = toS16(r_[i]);
            }
        }
        tickFrames_ += frames;
        while (tickFrames_ >= 882) {   // 20 ms: the original's display timer (message timeouts)
            tickFrames_ -= 882;
            ctr_->tick();
        }
        if (needRefresh_) { needRefresh_ = false; refreshAll(); }
    }

    // ---------------------------------------------------------------- parameters
    void setParam(const char* key, const char* val) {
        if (!strcmp(key, "state")) { setState(val); return; }
        const Entry* e = find(key);
        if (!e) return;
        switch (e->kind) {
        case PK_PROGRAM: {
            int i = atoi(val);
            i = std::max(0, std::min(511, i));
            if (i != synth_->editBuffer().libraryIndex()) { synth_->editBuffer().selectIndex(i); refreshAll(); }
            break;
        }
        case PK_KNOB: setKnob(e->arg, atof(val)); break;
        case PK_PAGE: if (atof(val) >= 0.5) { pageButton(e->arg); } break;
        case PK_ACTION: if (atof(val) >= 0.5) action(e->arg); break;
        default: break;
        }
    }

    int getParam(const char* key, char* buf, int n) {
        if (!strcmp(key, "state")) return getState(buf, n);
        if (!strcmp(key, "display_rev")) { return put(buf, n, std::to_string(rev()).c_str()); }
        if (!strncmp(key, "program:", 8)) {
            const int i = atoi(key + 8);
            if (i < 0 || i > 511) return 0;
            char t[64];
            snprintf(t, sizeof t, "%c%03d %s", 'A' + i / 128, i % 128, synth_->library().programName(i).c_str());
            return put(buf, n, t);
        }
        // "<knob key>_name" / "_display": the page's name and value text for the knob
        const size_t kl = strlen(key);
        if (kl > 5 && !strcmp(key + kl - 5, "_name")) return knobText(std::string(key, kl - 5), true, buf, n);
        if (kl > 8 && !strcmp(key + kl - 8, "_display")) return knobText(std::string(key, kl - 8), false, buf, n);
        const Entry* e = find(key);
        if (!e) return 0;
        char t[96];
        switch (e->kind) {
        case PK_PROGRAM: return put(buf, n, std::to_string(synth_->editBuffer().libraryIndex()).c_str());
        case PK_KNOB: snprintf(t, sizeof t, "%.6f", knobNorm(e->arg)); return put(buf, n, t);
        case PK_PAGE: case PK_ACTION: return put(buf, n, "0");
        case PK_LED: return put(buf, n, led(e->arg) ? "1" : "0");
        case PK_CELL: snprintf(t, sizeof t, "%.6f", cellFrame(e->arg) / 271.0); return put(buf, n, t);
        case PK_NUM: snprintf(t, sizeof t, "%.6f", numFrame(e->arg) / 16.0); return put(buf, n, t);
        case PK_PNAME: return put(buf, n, synth_->editBuffer().name(0).c_str());
        case PK_PACK: return put(buf, n, packs_[size_t(cur_)].name.c_str());
        }
        return 0;
    }

    // ---------------------------------------------------------------- LcdListener
    void lcdKnobValue(int knob, int value) override { if (knob >= 0 && knob < 10) kv_[knob] = value; }
    void lcdKnobRange(int knob, int mn, int mx, bool) override {
        if (knob >= 0 && knob < 10) { kmin_[knob] = mn; kmax_[knob] = mx; }
    }
    void lcdDrawn() override {
        lcd_->writeText(0, 1, synth_->editBuffer().modified() ? "*C*" : "   ", 0);
        if (!rom_) {   // keeps the notice on the display whatever the page or message draws
            lcd_->writeText(0, 0, "NO ROM  COPY SQ80 ROM FILE 262144 BYTES TO  ", 0);
            lcd_->writeText(0, 1, "THE DATA ROM FOLDER OF THE PLUGIN AND RELOAD", 0);
        }
    }
    void lcdParamMouseDown(int) override {}
    void lcdHint(int) override {}
    void lcdSaveMouse() override {}
    void lcdRestoreMouse() override {}
    void lcdScrollArrows(bool down, bool up) override { dn_ = down; up_ = up; }
    void lcdParamChanged(int, int) override {}
    void lcdShowPopup(const gui::LcdPopup&) override {}
    // MasterNotify: a program arrived in the edit buffer (MIDI program change)
    void notifyProgram() override { needRefresh_ = true; }
    void notifyEditor(int32_t, int32_t, int32_t) override { needRefresh_ = true; }

    ~Inst() override { flushLibrary(); }

private:
    static int16_t toS16(float v) {
        const float s = v * 32767.0f;
        return s >= 32767.0f ? 32767 : s <= -32768.0f ? -32768 : int16_t(s);
    }
    static int put(char* buf, int n, const char* s) {
        if (n <= 0) return 0;
        snprintf(buf, size_t(n), "%s", s);
        return int(strlen(s)) > 0 ? int(strlen(s)) : 1;
    }
    static const Entry* find(const char* key) {
        for (const Entry& e : kTable) if (!strcmp(e.key, key)) return &e;
        return nullptr;
    }

    // ---- display state
    int cellFrame(int idx) const {
        const int row = idx / 44, col = idx % 44;
        const auto& c = lcd_->cell(col, row);
        const int glyph = lcd_->charMap()[c.ch];
        const int attr = (c.attr >= 0 && c.attr < 4) ? c.attr : 0;
        return glyph * 4 + attr;
    }
    int numFrame(int i) const {
        const EditBuffer& eb = synth_->editBuffer();
        char s[8];
        snprintf(s, sizeof s, "%c%03d", 'A' + (eb.bank() & 3), eb.programNumber() % 1000);
        const char* p = strchr(kNumCharset, s[i]);
        return p ? int(p - kNumCharset) : 0;
    }
    bool led(int i) const {
        const Program& p = synth_->editBuffer().current();
        switch (i) {
        case 0: return p.s8(ofs::Sync) >= 1;
        case 1: return p.s8(ofs::AmMod) == 1;
        case 2: return p.s8(ofs::Mono) >= 1;
        case 3: return up_;
        case 4: return dn_;
        }
        return false;
    }
    float knobNorm(int k) const {
        const int span = kmax_[k] - kmin_[k];
        if (span <= 0) return 0.5f;
        return float(kv_[k] - kmin_[k]) / float(span);
    }
    int knobText(const std::string& key, bool name, char* buf, int n) {
        if (key.size() < 2 || key[0] != 'k') return 0;
        const int k = atoi(key.c_str() + 1);
        if (k < 0 || k > 9) return 0;
        gui::LcdSubPage* sp = ctr_->currentSub();
        gui::LcdParam* p = sp ? sp->paramForKnob(k) : nullptr;
        if (!p) return put(buf, n, name ? "-" : " ");
        std::string s = name ? p->name : p->valueText;
        // trim the field padding
        while (!s.empty() && s.back() == ' ') s.pop_back();
        size_t a = 0;
        while (a < s.size() && s[a] == ' ') a++;
        s = s.substr(a);
        if (s.empty()) s = name ? "-" : " ";
        return put(buf, n, s.c_str());
    }
    uint32_t rev() {
        // a hash of everything the panel shows; the wrapper polls it and redraws when it moves
        uint32_t h = 2166136261u;
        auto mix = [&h](uint32_t v) { h = (h ^ v) * 16777619u; };
        if (needRefresh_) { needRefresh_ = false; refreshAll(); }
        for (int i = 0; i < 88; i++) mix(uint32_t(cellFrame(i)));
        for (int k = 0; k < 10; k++) { mix(uint32_t(kv_[k] + 1000)); mix(uint32_t(kmin_[k] + 1000)); mix(uint32_t(kmax_[k] + 1000)); }
        for (int i = 0; i < 5; i++) mix(led(i) ? 1u : 0u);
        for (int i = 0; i < 4; i++) mix(uint32_t(numFrame(i)));
        for (char c : synth_->editBuffer().name(0)) mix(uint32_t((unsigned char)c));
        mix(uint32_t(cur_));
        if (h != lastHash_) { lastHash_ = h; revCount_++; }
        return revCount_;
    }

    void refreshAll() {
        ctr_->refresh(false);
        ctr_->drawAll();
        lcdDrawn();
        for (int k = 0; k < 10; k++) { /* values/ranges arrive through the listener */ }
    }
    void highlight(int param) {
        gui::LcdSubPage* sp = ctr_->currentSub();
        if (!sp) return;
        for (gui::LcdParam& p : sp->params) {
            p.highlight = p.index == param && p.knob >= 0;
            ctr_->drawParam(p);
        }
    }

    // ---- controls
    void setKnob(int k, double v01) {
        gui::LcdSubPage* sp = ctr_->currentSub();
        gui::LcdParam* p = sp ? sp->paramForKnob(k) : nullptr;
        if (!p || kmax_[k] <= kmin_[k]) return;
        v01 = std::max(0.0, std::min(1.0, v01));
        int iv = kmin_[k] + int(v01 * double(kmax_[k] - kmin_[k]) + 0.5);
        iv = std::max(kmin_[k], std::min(kmax_[k], iv));
        if (iv == ctr_->currentValue(p->index)) return;
        kv_[k] = iv;
        ctr_->knobChanged(k, iv);
        highlight(sp->knobParam(k));
    }
    void pageButton(int tag) {
        const int pg = tag >> 4, sub = tag & 0xf;
        const int curPage = ctr_->pageIndex(), curSub = ctr_->subPage();
        if (curPage == pg) {
            if (sub == 0xf || curSub == sub) ctr_->nextSubPage();
            else ctr_->selectSubPage(sub);
        } else if (sub == 0xf) {
            ctr_->selectPage(pg);
        } else {
            ctr_->selectPageSub(pg, sub);
        }
    }
    void message(const char* l1, const char* l2 = "") { ctr_->showMessage(-1, -1, l1, l2, 0x1c); }

    void action(int a) {
        EditBuffer& eb = synth_->editBuffer();
        switch (a) {
        case A_UP: case A_DOWN: {
            const int p = ((eb.programNumber() + (a == A_UP ? 1 : -1)) % 128 + 128) % 128;
            eb.selectProgram(p, -1);
            refreshAll();
            break;
        }
        case A_BANK:
            eb.setBank((eb.bank() + 1) % 4);
            eb.selectProgram(eb.programNumber(), -1);
            refreshAll();
            break;
        case A_WRITE: writeProgram(); break;
        case A_INIT:
            eb.initProgram();
            refreshAll();
            message("** Program initialized **");
            break;
        case A_SYNC: { Program& p = eb.current(); p.setS8(ofs::Sync, p.s8(ofs::Sync) < 1 ? 1 : 0); eb.setModified(true); refreshAll(); break; }
        case A_AM: {
            Program& p = eb.current();
            const int v = p.s8(ofs::AmMod);
            if (v == 0 || v == 2) p.setS8(ofs::AmMod, 1); else if (v == 1) p.setS8(ofs::AmMod, 0);
            eb.setModified(true);
            refreshAll();
            break;
        }
        case A_MONO: { Program& p = eb.current(); p.setS8(ofs::Mono, p.s8(ofs::Mono) < 1 ? 1 : 0); eb.setModified(true); refreshAll(); break; }
        case A_PGUP: ctr_->prevSubPage(); break;
        case A_PGDN: ctr_->nextSubPage(); break;
        case A_LOAD: nextPack(); break;
        case A_EXPORT: exportProgram(); break;
        }
    }

    // ---- saving
    void writeProgram() {
        EditBuffer& eb = synth_->editBuffer();
        const int b = eb.bank(), p = eb.programNumber();
        if (SoundLibrary::isBankWriteProtected(b)) {
            message("** BANK C/D IS FACTORY **", "SWITCH TO BANK A OR B");
            return;
        }
        if (eb.writeProgram(p, b)) {
            eb.setBank(b);
            message("** Program written **", base_.empty() ? "(NOT SAVED TO CARD)" : "SAVED TO USER FOLDER");
            refreshAll();
            saveLibrary();
        } else {
            message("** Write aborted **");
        }
    }
    void saveLibrary() {
        if (base_.empty()) return;
        worker_->write(base_ + "/User/" + packs_[size_t(cur_)].stem + ".8XL", synth_->library().saveLibrary());
    }
    void flushLibrary() { /* the worker's destructor drains the queue */ }
    void exportProgram() {
        if (base_.empty()) { message("** NO SD CARD FOLDER **"); return; }
        EditBuffer& eb = synth_->editBuffer();
        std::string nm = safeName(eb.name(0));
        char tag[16];
        snprintf(tag, sizeof tag, "%c%03d ", 'A' + (eb.bank() & 3), eb.programNumber());
        worker_->write(base_ + "/Export/" + tag + nm + ".syx", eb.exportSysex());
        message("** Program exported **", "EXPORT FOLDER");
    }

    // ---- the wave ROM (data/ROM/SQ80.ROM, 262144 bytes; not shipped with the plugin)
    void loadRom() {
        rom_ = false;
        if (base_.empty()) return;
        std::vector<uint8_t> bytes;
        static const char* names[] = {"SQ80.ROM", "sq80.rom", "SQ80.rom", "SQ80.BIN", "sq80.bin"};
        for (const char* n : names) {
            if (readFile(base_ + "/ROM/" + n, bytes) && loadWaveRom(bytes.data(), bytes.size())) { rom_ = true; return; }
        }
    }

    // ---- bank sets (drop-in files)
    void scanPacks() {
        packs_.clear();
        packs_.push_back({"MY LIBRARY", "MY", {}, 0});
        if (base_.empty()) return;
        std::vector<std::string> names;
        if (DIR* d = opendir((base_ + "/Banks").c_str())) {
            while (dirent* e = readdir(d)) {
                const std::string n = e->d_name;
                if (!n.empty() && n[0] == '.') continue;
                const size_t dot = n.rfind('.');
                if (dot == std::string::npos) continue;
                const std::string ext = upper(n.substr(dot + 1));
                if (ext == "8XL" || ext == "DAT" || ext == "SYX" || ext == "BNK" || ext == "8XB") names.push_back(n);
            }
            closedir(d);
        }
        std::sort(names.begin(), names.end());
        for (const std::string& n : names) {
            if (packs_.size() >= 64) break;
            Pack p;
            if (!readFile(base_ + "/Banks/" + n, p.bytes) || p.bytes.empty()) continue;
            const size_t dot = n.rfind('.');
            p.stem = safeName(n.substr(0, dot));
            p.name = upper(p.stem);
            if (p.name.size() > 28) p.name.resize(28);
            const std::string ext = upper(n.substr(dot + 1));
            p.kind = ext == "SYX" ? 2 : (p.bytes.size() == SoundLibrary::kBankFileSize ? 1 : 0);
            packs_.push_back(std::move(p));
        }
    }
    // applies the saved state of the set if there is one, else the dropped-in file
    void loadPack(int i, bool announce) {
        SoundLibrary& lib = synth_->library();
        const Pack& p = packs_[size_t(i)];
        std::vector<uint8_t> saved;
        bool ok = false;
        if (!base_.empty() && readFile(base_ + "/User/" + p.stem + ".8XL", saved) && !saved.empty())
            ok = lib.loadLibrary(saved.data(), saved.size());
        if (!ok && i == 0) {
            // first run: the upstream's own start state (banks A/B = copies of C/D)
            lib.restoreBackup(nullptr, 0);
            ok = true;
        }
        if (!ok) {
            if (p.kind == 0) ok = lib.loadLibrary(p.bytes.data(), p.bytes.size());
            else if (p.kind == 1) ok = lib.loadBank(p.bytes.data(), p.bytes.size(), 0);
            else ok = lib.importSysexBank(p.bytes.data(), p.bytes.size(), 0, true);
        }
        cur_ = i;
        EditBuffer& eb = synth_->editBuffer();
        eb.selectProgram(eb.programNumber(), -1);   // reload the slot from the new library
        if (announce) message(ok ? "** BANK SET LOADED **" : "** COULD NOT LOAD FILE **", p.name.c_str());
    }
    void nextPack() {
        if (packs_.size() < 2) { message("NO FILES IN THE BANKS FOLDER", "DROP .8XL / .SYX FILES THERE"); return; }
        cur_ = (cur_ + 1) % int(packs_.size());
        loadPack(cur_, true);
        refreshAll();
        if (!base_.empty()) saveLibrary();   // remembers where this set stands as its own file under User/
    }

    // ---- host state: the upstream chunk (edit buffer) + page + set name, hex
    int getState(char* buf, int n) {
        const std::vector<uint8_t>& ch = synth_->getChunk();
        static const char* hex = "0123456789ABCDEF";
        std::string s = "SQ8LMPC1:";
        for (uint8_t b : ch) { s += hex[b >> 4]; s += hex[b & 15]; }
        s += ":" + std::to_string(ctr_->pageIndex()) + ":" + std::to_string(ctr_->subPage()) + ":" + packs_[size_t(cur_)].stem;
        if (int(s.size()) < n) memcpy(buf, s.c_str(), s.size() + 1);
        return int(s.size()) + 1;   // the wrapper grows its buffer when this is larger than what it gave
    }
    void setState(const char* v) {
        if (strncmp(v, "SQ8LMPC1:", 9)) return;
        const char* p = v + 9;
        std::vector<uint8_t> ch;
        auto hv = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
        while (hv(p[0]) >= 0 && hv(p[1]) >= 0) { ch.push_back(uint8_t(hv(p[0]) * 16 + hv(p[1]))); p += 2; }
        int pg = 0, sb = 0;
        char stem[200] = "";
        if (*p == ':') sscanf(p, ":%d:%d:%199[^\n]", &pg, &sb, stem);
        for (size_t i = 0; i < packs_.size(); i++)
            if (packs_[i].stem == stem && int(i) != cur_) { loadPack(int(i), false); break; }
        synth_->setChunk(ch.data(), ch.size());
        ctr_->selectPageSub(std::max(0, std::min(ctr_->pageCount() - 1, pg)), std::max(0, sb));
        refreshAll();
    }

    std::string base_;
    std::unique_ptr<Synth> synth_;
    std::unique_ptr<gui::Sprite> stub_;
    std::unique_ptr<gui::LcdDisplay> lcd_;
    bool rom_ = false;
    std::unique_ptr<gui::LcdController> ctr_;
    std::unique_ptr<Worker> worker_;
    std::vector<Pack> packs_;
    int cur_ = 0;
    int kv_[10] = {}, kmin_[10] = {}, kmax_[10] = {};
    bool up_ = false, dn_ = false;
    std::atomic<bool> needRefresh_{false};
    uint32_t lastHash_ = 0, revCount_ = 1;
    int tickFrames_ = 0;
    float l_[128], r_[128];
};

void* eCreate(const char* dir) { return new Inst(dir); }
void eDestroy(void* p) { delete static_cast<Inst*>(p); }
void eMidi(void* p, const uint8_t* m, int n) { static_cast<Inst*>(p)->midi(m, n); }
void eSet(void* p, const char* k, const char* v) { static_cast<Inst*>(p)->setParam(k, v); }
int eGet(void* p, const char* k, char* b, int n) { return static_cast<Inst*>(p)->getParam(k, b, n); }
void eRender(void* p, int16_t* o, int f) { static_cast<Inst*>(p)->render(o, f); }

const mpc_engine_t kEngine = {eCreate, eDestroy, eMidi, eSet, eGet, eRender, nullptr, nullptr};

}  // namespace

extern "C" const mpc_engine_t* mpc_engine(void) { return &kEngine; }
