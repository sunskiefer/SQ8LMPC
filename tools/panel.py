#!/usr/bin/env python3
"""SQ8LMPC panel: the ONE layout file (layout.json) and its default.

    python3 tools/panel.py --write-default     (writes layout.json from the original editor's control positions)
    python3 tools/panel.py --check             (the layout is valid: every control has x, y, w, h)

layout.json is what the Panel Editor edits and what tools/panel_art.py builds the skin from. Every control and every
printed label has an id and a rectangle (x, y, w, h) in Force plugin-screen pixels (1280 x 628); nothing is placed
automatically. "move 14 to x,y" changes only that control.

The default places everything where the original SQ8L editor has it. The original is 626 x 430 (rows from 408 down are the dark strip with the screws, dropped); its top strip (the
FILE / OPTIONS / INFO / PANIC menu) is not used, so the art from y = 24 down is stretched to 1280 x 628 (x 2.0447,
y 1.5468) and the knobs, the display and the buttons are drawn large on it, round."""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "layout.json")
W, H = 1280, 628
ART_W, ART_H, ART_TOP = 626, 408, 24          # original art size; its first rows (the menu strip) are dropped
ART_LEFT, ART_RIGHT = 9, 616                   # the frame runs x 9..615: the dark side margins are dropped too
SX, SY = W / (ART_RIGHT - ART_LEFT), H / (ART_H - ART_TOP)
LABEL_PX = 26                                  # Meta4MPCXXL header/readout size: nothing is printed smaller


def X(x):
    return (x - ART_LEFT) * SX


def XW(w):
    return w * SX


def Y(y):
    return (y - ART_TOP) * SY


def R(v):
    return int(round(v))


def rect(x, y, w, h):
    return {"x": R(X(x)), "y": R(Y(y)), "w": R(w * SX), "h": R(h * SY)}


# (id, param key, original x, y, w, h, button picture) -- the original editor's buttons (EditorView.cpp kButtons)
BUTTONS = [
    ("up", "b_up", 281, 39, 30, 18, "updn_up"), ("down", "b_down", 281, 58, 30, 18, "updn_down"),
    ("write", "b_write", 384, 52, 44, 22, "red"), ("init", "b_init", 432, 52, 44, 22, "red"),
    ("export", "b_export", 497, 52, 44, 22, "grey"), ("load", "b_load", 546, 52, 44, 22, "grey"),
    ("bank", "b_bank", 318, 52, 44, 22, "teal"),
    ("osc1", "pg_osc1", 158, 110, 44, 22, "red"), ("osc2", "pg_osc2", 158, 138, 44, 22, "red"),
    ("osc3", "pg_osc3", 158, 166, 44, 22, "red"),
    ("dca1", "pg_dca1", 277, 110, 44, 22, "grey"), ("dca2", "pg_dca2", 277, 138, 44, 22, "grey"),
    ("dca3", "pg_dca3", 277, 166, 44, 22, "grey"),
    ("filt", "pg_filt", 376, 138, 44, 22, "grey"), ("dca4", "pg_dca4", 445, 138, 44, 22, "grey"),
    ("modes", "pg_modes", 552, 160, 44, 22, "teal"), ("wav", "pg_wav", 28, 138, 44, 22, "teal"),
    ("sync", "b_sync", 110, 126, 30, 18, "small"), ("am", "b_am", 223, 126, 30, 18, "small"),
    ("mono", "b_mono", 561, 110, 30, 18, "small"),
    ("lfo1", "pg_lfo1", 49, 370, 44, 22, "grey"), ("lfo2", "pg_lfo2", 95, 370, 44, 22, "grey"),
    ("lfo3", "pg_lfo3", 141, 370, 44, 22, "grey"), ("lfo4", "pg_lfo4", 187, 370, 44, 22, "grey"),
    ("env1", "pg_env1", 243, 370, 44, 22, "grey"), ("env2", "pg_env2", 289, 370, 44, 22, "grey"),
    ("env3", "pg_env3", 335, 370, 44, 22, "grey"), ("env4", "pg_env4", 381, 370, 44, 22, "grey"),
    ("mat1", "pg_mat1", 440, 370, 44, 22, "grey"), ("mat2", "pg_mat2", 486, 370, 44, 22, "grey"),
    ("mat3", "pg_mat3", 532, 370, 44, 22, "grey"),
    ("pgup", "b_pgup", 570, 252, 20, 18, "scr_up"), ("pgdn", "b_pgdn", 570, 270, 20, 18, "scr_dn"),
]
LEDS = [("led_sync", "led_sync", 94, 127), ("led_am", "led_am", 207, 127), ("led_mono", "led_mono", 545, 111)]
# printed words of the original art: (id, original text, new text, centre x, centre y, original ink rect x0,y0,x1,y1)
# -- the original ink is wiped and the new text is drawn live in the layout; edit text / x / y / w / h in the layout
LABELS = [
    ("bank", "BANK", "BANK", 339, 39, (322, 37, 357, 49)), ("write", "WRITE", "SAVE", 405, 39, (388, 37, 424, 49)),
    ("init", "INIT", "INIT", 455, 39, (443, 37, 468, 49)), ("export", "SEND", "EXP", 519, 39, (503, 37, 536, 49)),
    ("load", "REQ", "LOAD", 567, 39, (555, 37, 580, 49)),
    ("l_mono", "MONO", "MONO", 574, 97, (557, 95, 592, 107)), ("l_mix", "MIX", "MIX", 49, 124, (39, 123, 61, 135)),
    ("l_sync", "SYNC", "SYNC", 78, 109, (107, 113, 141, 125)), ("l_osc", "OSC 1-3", "OSC 1-3", 181, 97, (159, 95, 204, 107)),
    ("l_dca", "DCA 1-3", "DCA 1-3", 299, 97, (277, 95, 322, 107)), ("l_am", "AM", "AM", 270, 134, (254, 128, 273, 140)),
    ("l_filter", "FILTER", "FILTER", 398, 125, (379, 123, 417, 135)), ("l_dca4", "DCA 4", "DCA 4", 468, 125, (451, 123, 486, 135)),
    ("l_modes", "MODES\n/ EMU", "MODES", 574, 150, (554, 134, 596, 158)),
    ("l_left", "Left", "L", 510, 113, (500, 107, 521, 119)), ("l_right", "Right", "R", 510, 182, (498, 176, 524, 188)),
    ("l_pan", "pan", "PAN", 481, 164, (472, 158, 492, 170)),
    ("l_lfo", "LFO 1", "LFO 1", 70, 359, (56, 353, 88, 365)), ("l_lfo2", "2", "2", 116, 359, (112, 353, 120, 365)),
    ("l_lfo3", "3", "3", 163, 359, (159, 353, 167, 365)), ("l_lfo4", "4", "4", 208, 359, (204, 353, 212, 365)),
    ("l_env", "ENV 1", "ENV 1", 265, 359, (250, 353, 281, 365)), ("l_env2", "2", "2", 311, 359, (307, 353, 315, 365)),
    ("l_env3", "3", "3", 358, 359, (354, 353, 362, 365)), ("l_env4", "4", "4", 403, 359, (399, 353, 407, 365)),
    ("l_mat", "MAT 1", "MAT 1", 462, 359, (447, 353, 479, 365)), ("l_mat2", "2", "2", 508, 359, (504, 353, 512, 365)),
    ("l_mat3", "3", "3", 553, 359, (549, 353, 557, 365)),
]
KNOB = 101  # default knob size (screen px); the knob picture is round at any size the layout gives
VFD_COLS, VFD_ROWS = 44, 2
CELL_W, CELL_H = 24, 40


def default():
    items = []
    for i in range(10):
        cx, cy = X(138 + 96 * (i % 5)), (296 if i < 5 else 494)   # rows sit clear of the display window (y 341..421)
        items.append({"id": "k%d" % i, "type": "knob", "param": "k%d" % i, "x": R(cx - KNOB / 2), "y": R(cy - KNOB / 2),
                      "w": KNOB, "h": KNOB})
    # the display window (original 25..598 x 242..298): 44 x 2 cells of CELL_W x CELL_H, centred in it
    wx, wy, ww, wh = X(25), Y(242), XW(598 - 25), (298 - 242) * SY
    items.append({"id": "vfd", "type": "vfd", "x": R(X(38)),
                  "y": R(wy + (wh - VFD_ROWS * CELL_H) / 2), "w": VFD_COLS * CELL_W, "h": VFD_ROWS * CELL_H})
    items.append({"id": "num", "type": "num", "x": R(X(30)), "y": R(Y(44)), "w": 148, "h": 38})
    items.append({"id": "pname", "type": "text", "param": "pname", "x": R(X(110)), "y": R(Y(46)), "w": 322, "h": 40,
                  "px": 34, "color": "a8a8c0"})
    for bid, param, x, y, w, h, pic in BUTTONS:
        d = {"id": bid, "type": "button", "param": param, "pic": pic}
        d.update(rect(x, y, w, h))
        items.append(d)
    for lid, param, x, y in LEDS:
        items.append({"id": lid, "type": "led", "param": param, "x": R(X(x + 7) - 14), "y": R(Y(y + 7) - 14), "w": 28, "h": 28})
    labels = []
    for lid, old, new, cx, cy, ink in LABELS:
        lines = new.count("\n") + 1
        w = max(60, R(len(max(new.split("\n"), key=len)) * LABEL_PX * 0.92))
        h = R(LABEL_PX * 1.25 * lines)
        labels.append({"id": lid, "text": new, "px": LABEL_PX, "x": R(X(cx) - w / 2), "y": R(Y(cy) - h / 2), "w": w, "h": h,
                       "wipe": [R(X(ink[0])), R(Y(ink[1])), R(XW(ink[2] - ink[0])), R((ink[3] - ink[1]) * SY)]})
    for o in items + labels:      # the LFO / ENV / MAT strip: labels clear of the knob ticks, buttons clear of the panel frame
        if o["id"].startswith(("lfo", "env", "mat", "l_lfo", "l_env", "l_mat")):
            if o in labels:
                o["y"] += 0
                wx, wy, ww, wh = o["wipe"]
                o["wipe"] = [wx - 3, wy - 2, ww + 6, wh + 4]
            else:
                o["y"] += 0
                o["h"] = 30
    return {"format": "sq8lmpc-layout 1", "screen": [W, H], "items": items, "labels": labels}


def load():
    return json.load(open(LAYOUT))


def check(L):
    bad = []
    for it in L["items"] + L["labels"]:
        for k in ("x", "y", "w", "h"):
            if not isinstance(it.get(k), int):
                bad.append("%s: %s missing" % (it["id"], k))
    ids = [it["id"] for it in L["items"]] + ["label:" + l["id"] for l in L["labels"]]
    if len(ids) != len(set(ids)):
        bad.append("duplicate ids")
    return bad


if __name__ == "__main__":
    if "--write-default" in sys.argv:
        json.dump(default(), open(LAYOUT, "w"), indent=1)
        print("layout.json written:", len(default()["items"]), "controls,", len(default()["labels"]), "labels")
    elif "--check" in sys.argv:
        b = check(load())
        if b:
            sys.exit("layout.json: " + "; ".join(b))
        print("layout.json OK")
