#!/usr/bin/env python3
"""Build SQ8LMPC's panel skin from layout.json and the original art (art/orig/), into the skin mpc-vst-plugins made
from layout.conf (which only gave the tab and the Q-Links).

    python3 tools/panel_art.py "<skin dir>/Plugin Skins"

Source of truth: the original SQ8L editor's own bitmaps (extract_art.py). Mode: AS-IS art with LABEL FIX only --
the menu strip is dropped, the printed words are replaced by live labels from layout.json (Michroma, >= 26 px), the
knobs, the display characters, the buttons and the LEDs are the original sprites redrawn large (knobs turn in place:
128-frame filmstrips)."""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import panel as P  # noqa: E402

ORIG = os.path.join(P.ROOT, "art", "orig")
LABEL_FONT = os.path.join(P.ROOT, "art", "fonts", "Michroma-Regular.ttf")
INK = (205, 205, 222)           # the original's printed label colour (light grey-blue italic)
PANEL = (52, 51, 67)
SS = 4
KNOB_FRAMES = 128
KNOB_MAX = 126                  # 128 frames must stay under 16384 px


def R(v):
    return int(round(v))


def sprite(name, fw, fh):
    """frames of an original strip (vertical, fw x fh each) as RGB images"""
    im = Image.open(os.path.join(ORIG, name + "_strip.png")).convert("RGB")
    return [im.crop((0, i * fh, fw, (i + 1) * fh)) for i in range(im.size[1] // fh)]


def glow_rgba(frame, size, glow=0.9, radius=1.6):
    """an original sprite frame (light on black) redrawn at `size`, crisp and bright so it reads, with a soft glow:
    the colour stays the original's, the darkest unlit segments stay faintly visible (the back-screen look)"""
    # The original is a pixel font: every lit pixel becomes a square dot (nearest), so the single-pixel diagonals of
    # V, W, X and Y (drawn fainter than the straight segments) stay a staircase that reads, instead of melting into blobs.
    a0 = np.asarray(frame, np.float32) / 255.0
    lum0 = a0.max(axis=2)
    t0 = np.clip((lum0 - 0.22) / 0.25, 0, 1) ** 0.5    # faint diagonal pixels (~0.3-0.6) are lit nearly as much as the main strokes
    lit = np.asarray(Image.fromarray((t0 * 255).astype(np.uint8)).resize((size[0] * 3, size[1] * 3), Image.NEAREST), np.float32) / 255.0
    lit = np.asarray(Image.fromarray((lit * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.6)), np.float32) / 255.0
    lit = np.clip(lit * 1.15, 0, 1)                 # a touch of softening at the dot edges only
    a = np.asarray(frame.resize((size[0] * 3, size[1] * 3), Image.BICUBIC), np.float32) / 255.0
    lum = a.max(axis=2)
    ghost = np.clip(lum, 0, 0.30) * 0.55            # faint unlit segments
    core = np.asarray(Image.fromarray((lit * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(radius * 3.2)), np.float32) / 255.0
    hot = a[lum > 0.6]
    base = (hot.mean(axis=0) / max(hot.mean(axis=0).max(), 1e-6)) if len(hot) else np.array([70, 255, 214], np.float32) / 255.0   # the sprite's own colour
    base = np.asarray(base, np.float32)
    white = base * 0.25 + 0.75
    rgb = base * (1 - lit[..., None] * 0.55) + white * (lit[..., None] * 0.55)
    al = np.clip(np.maximum(lit, ghost) + glow * 0.42 * core * (1 - lit), 0, 1)
    out = np.concatenate([rgb * 255, (al * 255)[..., None]], -1).astype(np.uint8)
    return Image.fromarray(out, "RGBA").resize(size, Image.LANCZOS)


def label_font(px):
    return ImageFont.truetype(LABEL_FONT, px)


# ------------------------------------------------------------------------------------------------ background
def wipe(img, rect):
    """wipe printing inside rect (x, y, w, h): the flat panel colour (with the art's grain) fills it"""
    a = np.asarray(img).astype(np.float32)
    x, y, w, h = rect
    x0, y0 = max(0, x), max(0, y)
    x1, y1 = min(a.shape[1], x + w), min(a.shape[0], y + h)
    ring = np.concatenate([a[y0 - 3:y0, x0:x1, :3].reshape(-1, 3), a[y1:y1 + 3, x0:x1, :3].reshape(-1, 3)])
    fill = np.median(ring, axis=0)
    rng = np.random.default_rng(7)
    a[y0:y1, x0:x1, :3] = np.clip(fill + rng.normal(0, 1.2, (y1 - y0, x1 - x0, 1)), 0, 255)
    return Image.fromarray(a.astype(np.uint8), "RGBA")


def draw_label(d, l):
    font = label_font(l["px"])
    lines = l["text"].split("\n")
    lh = l["px"] * 1.25
    y0 = l["y"] + (l["h"] - lh * len(lines)) / 2 + lh / 2
    for i, t in enumerate(lines):
        d.text((l["x"] + l["w"] / 2, y0 + i * lh), t, font=font, fill=INK + (255,), anchor="mm")


def window_wells(bg, vfd, num):
    """the back screen: a dark glass behind each display cell, a faint grid, a soft inner shadow"""
    d = ImageDraw.Draw(bg)
    cw, ch = vfd["w"] // P.VFD_COLS, vfd["h"] // P.VFD_ROWS
    for r in range(P.VFD_ROWS):
        for c in range(P.VFD_COLS):
            x, y = vfd["x"] + c * cw, vfd["y"] + r * ch
            d.rounded_rectangle([x + 1, y + 1, x + cw - 2, y + ch - 2], 3, fill=(0, 9, 7, 255))
    cw, ch = num["w"] // 4, num["h"]
    for c in range(4):
        x = num["x"] + c * cw
        d.rounded_rectangle([x + 1, num["y"] + 1, x + cw - 2, num["y"] + ch - 2], 3, fill=(14, 2, 2, 255))
    return bg


def label_boxes(L):
    """each label's real ink box (measured with the font), for the overlap check: id -> (x0, y0, x1, y1)"""
    out = {}
    for l in L["labels"]:
        font = label_font(l["px"])
        lines = l["text"].split("\n")
        w = max(font.getlength(t) for t in lines)
        lh = l["px"] * 1.25
        h = lh * len(lines)
        cx, cy = l["x"] + l["w"] / 2, l["y"] + l["h"] / 2
        out["label:" + l["id"]] = (cx - w / 2, cy - h / 2 + l["px"] * 0.12, cx + w / 2, cy + h / 2 - l["px"] * 0.12)
    return out


def overlaps(L, tol=3):
    """pairs of labels / controls whose boxes overlap by more than tol px (never moved, only reported by id)"""
    boxes = label_boxes(L)
    for it in L["items"]:
        if it["type"] in ("button", "led"):
            boxes[it["id"]] = (it["x"], it["y"], it["x"] + it["w"], it["y"] + it["h"])
        elif it["type"] == "knob":   # the touch box
            t = max(60, R(0.68 * min(it["w"], it["h"])))
            cx, cy = it["x"] + it["w"] / 2, it["y"] + it["h"] / 2
            boxes[it["id"]] = (cx - t / 2, cy - t / 2, cx + t / 2, cy + t / 2)
    ids = list(boxes)
    bad = []
    for i, a in enumerate(ids):
        for b in ids[i + 1:]:
            if a.startswith("label:") or b.startswith("label:"):
                A, B = boxes[a], boxes[b]
                if min(A[2], B[2]) - max(A[0], B[0]) > tol and min(A[3], B[3]) - max(A[1], B[1]) > tol:
                    bad.append((a, b))
    return bad


def make_background(L):
    art = Image.open(os.path.join(ORIG, "background.png")).convert("RGBA").crop((P.ART_LEFT, P.ART_TOP, P.ART_RIGHT, P.ART_H))
    bg = art.resize((P.W, P.H), Image.LANCZOS)
    for l in L["labels"]:
        if l.get("wipe"):
            bg = wipe(bg, l["wipe"])
    items = {i["id"]: i for i in L["items"]}
    bg = window_wells(bg, items["vfd"], items["num"])
    d = ImageDraw.Draw(bg)
    for l in L["labels"]:
        draw_label(d, l)
    return bg


# ------------------------------------------------------------------------------------------------ knob
def knob_strip(sk, s):
    """128 frames of the original's black glossy knob with its white pointer and tick ring, redrawn round at s px"""
    name = "sq_knob_%d.png" % s
    if name in sk.used:
        return name
    n = s * SS
    c = n / 2
    base = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(base)
    # tick ring: 11 ticks over 270 degrees, the original's grey
    for i in range(11):
        a = math.radians(-135 + 27 * i)
        r0, r1 = 0.43 * n, 0.50 * n
        d.line([(c + r0 * math.sin(a), c - r0 * math.cos(a)), (c + r1 * math.sin(a), c - r1 * math.cos(a))],
               fill=(150, 150, 168, 255), width=max(2, int(0.022 * n)))
    # shadow + body
    sh = Image.new("L", (n, n), 0)
    ImageDraw.Draw(sh).ellipse([c - 0.37 * n, c - 0.34 * n, c + 0.37 * n, c + 0.40 * n], fill=170)
    sh = sh.filter(ImageFilter.GaussianBlur(0.035 * n))
    shadow = Image.new("RGBA", (n, n), (8, 8, 18, 0))
    shadow.putalpha(sh)
    base.alpha_composite(shadow)
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    rr = np.hypot(xx - c, yy - c) / (0.355 * n)
    body_a = np.clip((1 - rr) * 0.31 * n + 0.5, 0, 1)
    hl = np.exp(-(((xx - (c - 0.09 * n)) ** 2 + (yy - (c - 0.12 * n)) ** 2) / (2 * (0.12 * n) ** 2)))
    rim = np.clip(1 - np.abs(rr - 0.92) / 0.10, 0, 1) * 0.35
    lum = 14 + 70 * hl + 30 * rim * (yy < c)
    body = np.stack([lum, lum, lum * 1.05, body_a * 255], -1).astype(np.uint8)
    base.alpha_composite(Image.fromarray(body, "RGBA"))
    strip = Image.new("RGBA", (s, s * KNOB_FRAMES), (0, 0, 0, 0))
    for k in range(KNOB_FRAMES):
        fr = base.copy()
        dd = ImageDraw.Draw(fr)
        a = math.radians(-135 + 270 * k / (KNOB_FRAMES - 1))
        r0, r1 = 0.08 * n, 0.33 * n
        p0 = (c + r0 * math.sin(a), c - r0 * math.cos(a))
        p1 = (c + r1 * math.sin(a), c - r1 * math.cos(a))
        dd.line([p0, p1], fill=(245, 245, 250, 255), width=max(3, int(0.05 * n)))
        for p in (p0, p1):
            rd = max(3, int(0.05 * n)) / 2
            dd.ellipse([p[0] - rd, p[1] - rd, p[0] + rd, p[1] + rd], fill=(245, 245, 250, 255))
        strip.paste(fr.resize((s, s), Image.LANCZOS), (0, k * s))
    assert strip.size[1] <= 16384
    strip.save(os.path.join(sk.skin, name))
    sk.used.add(name)
    return name
def bounds_s(x, y, w, h):
    return "%d %d %d %d" % (R(x), R(y), R(w), R(h))


def sub(name, typ, data, b, visible="Always"):
    return {"version": 2, "componentData": {"version": 1, "name": name, "type": typ, "data": data},
            "handle remapping": {"version": 1, "map": []},
            "bounds": {"version": 2, "acceptsHWFocus": "No", "showWhenDataModelInvalid": "Show", "whenVisible": visible,
                       "boundsType": "Absolute", "bounds": b, "additionalInvalidatingHandles": []}}


def focus(w, h):
    return sub("Focus", "Focus", {"version": 1, "backgroundColour": "00000000", "outlineColour": "00000000",
                                  "backgroundInset": 2.0, "outlineThickness": 0.0}, bounds_s(0, 0, w, h), "WhenFocussed")


def action(on, handler, extra=""):
    return {"version": 2, "onAction": on, "handler": handler, "handleName": "" if handler == "Show Overlay" else "Data",
            "additionalData": extra, "handle remapping": {"version": 1, "map": []}}


KNOB_ACTIONS = [action("Mouse Down", "Q-Link"), action("Double Click", "Show Overlay", "knob overlay"),
                action("Enter Pressed", "Show Overlay", "knob overlay")]
KEY_ACTIONS = [action("Mouse Down", "Q-Link"), action("Enter Pressed", "Toggle Switch")]
RADIO_ACTIONS = [action("Mouse Down", "Q-Link")]


def definition(key, actions, parts, ignore=False):
    return {"key": key, "value": {
        "version": 4, "actions": actions,
        "backgroundData": {"version": 1, "focussed": {"version": 1, "colour": "0", "image": ""},
                           "unfocussed": {"version": 1, "colour": "0", "image": ""}},
        "ignoreMousePresses": ignore, "disableCoarseDataWheel": False, "repeats": 1, "hideQLinkBounds": True,
        "componentsData": parts}}


def filmstrip(name, img, frames, w, h, drag="Vertical", x=0, y=0):
    return sub(name, "Knob", {"version": 5, "knobType": "FilmStrip", "filmStrip": img, "numFrames": frames,
                              "invert": False, "dragOrientation": drag, "handleName": "Data"}, bounds_s(x, y, w, h))


def button(on_img, off_img, bid, n, w, h):
    return sub("Button", "Button", {"version": 2, "onImage": on_img, "offImage": off_img, "buttonId": bid,
                                    "numButtonsInGroup": n, "handleName": "Data", "gestureBehaviour": "Instant"},
               bounds_s(0, 0, w, h))


def value_label(w, h, px, colour="fb9013"):
    """MPC's own text: the parameter's value (Titillium Web, the font MPC renders)"""
    return sub("Value", "Label", {"version": 1, "textStyle": {"version": 1, "font": {"version": 1, "name": "Titillium Web",
                                                                                  "style": "SemiBold", "height": px},
                                                            "colour": "ff" + colour,
                                                            "justification": "horizontallyCentred verticallyCentred",
                                                            "case": "Original"},
                                  "type": "Value", "handleName": "Data"}, bounds_s(0, 0, w, h))


def placed(dkey, name, index, rect, focus_on=True):
    x, y, w, h = rect
    return {"version": 2,
            "componentData": {"version": 1, "name": name, "type": dkey, "data": {"version": 1, "handleName": "Data"}},
            "handle remapping": {"version": 1, "map": [{"key": "Data", "value": "Parameter %d" % index}]},
            "bounds": {"version": 2, "acceptsHWFocus": "Yes" if focus_on else "No", "showWhenDataModelInvalid": "Hide",
                       "whenVisible": "Always", "boundsType": "Absolute", "bounds": bounds_s(x, y, w, h),
                       "additionalInvalidatingHandles": []}}


def picture(img, rect, index, option, count):
    """an image shown while parameter index is at option (of count): MPC's IndexedEnabling"""
    c = sub("Picture", "Image", {"version": 2, "imageType": "Regular", "colour": "0", "image": img}, bounds_s(*rect))
    c["bounds"]["additionalInvalidatingHandles"] = ["IndexedEnabling/%d/%d/Parameter %d" % (option, count, index)]
    return c


# ------------------------------------------------------------------------------------------------ the builder
class Skin:
    def __init__(self, skin, params):
        self.skin = skin
        self.index = {p["key"]: i for i, p in enumerate(params)}
        self.params = params
        self.defs = []
        self.used = set()
        self.saved = {}

    def save(self, img, name, rgb=True):
        """write a picture once (identical pictures share one file)"""
        data = img.convert("RGB" if rgb else "RGBA")
        h = hash(data.tobytes()) ^ hash(data.size) ^ hash(rgb)
        if h in self.saved:
            return self.saved[h]
        base, n = name[:-4], 2
        while name in self.used:   # a name already taken by another picture: a new one
            name = "%s_%d.png" % (base, n)
            n += 1
        data.save(os.path.join(self.skin, name))
        self.used.add(name)
        self.saved[h] = name
        return name

    def opts(self, key):
        return len(self.params[self.index[key]].get("options") or [])

    def has_def(self, key):
        return any(d["key"] == key for d in self.defs)



# ------------------------------------------------------------------------------------------------ sprites
def button_images(pic, w, h):
    """(pressed, idle) pictures of an original button sprite redrawn at w x h"""
    if pic in ("grey", "red", "teal"):
        fr = sprite("ButtGIF", 44, 22)
        i = {"grey": 0, "red": 2, "teal": 4}[pic]
        a, b = fr[i], fr[i + 1]
    elif pic == "small":
        fr = sprite("smallButtGIF", 30, 18)
        a, b = fr[0], fr[1]
    elif pic in ("updn_up", "updn_down"):
        fr = sprite("upDownGif", 30, 18)
        i = 0 if pic == "updn_up" else 2
        a, b = fr[i], fr[i + 1]
    else:   # scr_up / scr_dn: dim (idle) and lit (pressed)
        fr = sprite("scrButtGif", 20, 18)
        i = 0 if pic == "scr_up" else 2
        a, b = fr[i], fr[i + 1]
    size = (w, h)
    return b.resize(size, Image.LANCZOS).convert("RGB"), a.resize(size, Image.LANCZOS).convert("RGB")


def vfd_strip(sk, cw, ch):
    """272 frames (68 glyphs x 4 attributes) of the original display characters, cw x ch each, glowing"""
    name = "sq_vfd_%dx%d.png" % (cw, ch)
    if name in sk.used:
        return name
    fr = sprite("charGIF", 11, 18)
    k = max(1, (ch - 2) // 18)                      # whole-pixel scale of the 11 x 18 font (2x in a 24 x 40 cell)
    gh, gw = 18 * k, 11 * k
    strip = Image.new("RGBA", (cw, ch * len(fr)), (0, 0, 0, 0))
    for i, f in enumerate(fr):
        g = glow_rgba(f, (gw, gh))
        cell = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
        cell.alpha_composite(g, ((cw - gw) // 2, (ch - gh) // 2))
        strip.paste(cell, (0, i * ch))
    assert strip.size[1] <= 12288, strip.size
    strip.save(os.path.join(sk.skin, name))
    sk.used.add(name)
    return name


def num_strip(sk, cw, ch):
    """17 frames of the original red program-number characters"""
    name = "sq_num_%dx%d.png" % (cw, ch)
    if name in sk.used:
        return name
    fr = sprite("numCharGIF", 18, 23)
    gh = ch - 4
    gw = R(gh * 18 / 23)
    strip = Image.new("RGBA", (cw, ch * len(fr)), (0, 0, 0, 0))
    for i, f in enumerate(fr):
        g = glow_rgba(f, (gw, gh), glow=0.7, radius=1.8)
        cell = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
        cell.alpha_composite(g, ((cw - gw) // 2, (ch - gh) // 2))
        strip.paste(cell, (0, i * ch))
    strip.save(os.path.join(sk.skin, name))
    sk.used.add(name)
    return name


def led_images(w, h):
    fr = sprite("LedGIF", 14, 14)
    return [f.resize((w, h), Image.LANCZOS).convert("RGB") for f in fr]


# ------------------------------------------------------------------------------------------------ the page
TAB = "PANEL"


def build_page(sk, L):
    bg = make_background(L)
    kids = []
    seen = set()

    def place(dkey, name, key, rect):
        f = key not in seen
        seen.add(key)
        kids.append(placed(dkey, name, sk.index[key], rect, f))

    items = {i["id"]: i for i in L["items"]}
    for it in L["items"]:
        t, x, y, w, h = it["type"], it["x"], it["y"], it["w"], it["h"]
        if t == "knob":
            s = min(KNOB_MAX, 2 * R(min(w, h) / 2))
            name = knob_strip(sk, s)
            tb = max(60, R(0.68 * s))              # touch box = the knob body
            dkey = "sqKnob_%d_%d" % (s, tb)
            if not sk.has_def(dkey):
                sk.defs.append(definition(dkey, KNOB_ACTIONS, [focus(tb, tb), filmstrip(
                    "Knob", name, KNOB_FRAMES, s, s, "Vertical", (tb - s) // 2, (tb - s) // 2)]))
            cx, cy = x + w / 2, y + h / 2
            place(dkey, it["id"], it["param"], (R(cx - tb / 2), R(cy - tb / 2), tb, tb))
        elif t == "vfd":
            cw, ch = w // P.VFD_COLS, h // P.VFD_ROWS
            name = vfd_strip(sk, cw, ch)
            dkey = "sqCell_%d_%d" % (cw, ch)
            if not sk.has_def(dkey):
                sk.defs.append(definition(dkey, [], [filmstrip("Cell", name, 272, cw, ch, "Vertical")], ignore=True))
            for r in range(P.VFD_ROWS):
                for c in range(P.VFD_COLS):
                    place(dkey, "vfd %d.%02d" % (r, c), "c%d_%02d" % (r, c), (x + c * cw, y + r * ch, cw, ch))
        elif t == "num":
            cw, ch = w // 4, h
            name = num_strip(sk, cw, ch)
            dkey = "sqNum_%d_%d" % (cw, ch)
            if not sk.has_def(dkey):
                sk.defs.append(definition(dkey, [], [filmstrip("Digit", name, 17, cw, ch, "Vertical")], ignore=True))
            for c in range(4):
                place(dkey, "num %d" % c, "n%d" % c, (x + c * cw, y, cw, ch))
        elif t == "text":
            dkey = "sqText_%d_%d_%d_%s" % (w, h, it["px"], it.get("color", ""))
            if not sk.has_def(dkey):
                sk.defs.append(definition(dkey, [], [value_label(w, h, it["px"], it.get("color", "ff3a3a"))], ignore=True))
            place(dkey, it["id"], it["param"], (x, y, w, h))
        elif t == "button":
            on, off = button_images(it["pic"], w, h)
            on_n = sk.save(on, "sq_btn_%s_on.png" % it["id"])
            off_n = sk.save(off, "sq_btn_%s_off.png" % it["id"])
            dkey = "sqBtn_%s" % it["id"]
            sk.defs.append(definition(dkey, KEY_ACTIONS, [focus(w, h), button(on_n, off_n, 1, 1, w, h)]))
            place(dkey, it["id"], it["param"], (x, y, w, h))
        elif t == "led":
            imgs = led_images(w, h)
            dkey = "sqLed_%s" % it["id"]
            parts = [picture(sk.save(imgs[n], "sq_led_%s_%d.png" % (it["id"], n)), (0, 0, w, h), sk.index[it["param"]], n, 2)
                     for n in range(2)]
            sk.defs.append(definition(dkey, [], parts, ignore=True))
            place(dkey, it["id"], it["param"], (x, y, w, h))
    return bg, kids


def main(skin):
    tui_path = os.path.join(skin, "TUI.json")
    tui = json.load(open(tui_path))
    cdefs = tui["pageData"]["componentDefinitions"]
    defs = cdefs["localComponentDefinitions"]
    params = json.load(open(os.path.join(P.ROOT, "params.json")))["params"]
    L = P.load()
    bad = P.check(L)
    if bad:
        raise SystemExit("layout.json: " + "; ".join(bad))
    for pair in overlaps(L):
        print("panel_art: OVERLAP %s / %s (left where they are)" % pair)
    sk = Skin(skin, params)
    bg, kids = build_page(sk, L)
    bg.convert("RGB").save(os.path.join(skin, "sq_bg.png"))
    sk.used.add("sq_bg.png")
    page = [sub("Background", "Image", {"version": 2, "imageType": "Regular", "colour": "0", "image": "sq_bg.png"},
                bounds_s(0, 0, P.W, P.H))] + kids
    keep = []
    for e in defs:
        tabname = e["key"].split("|")[0]
        if "|" in e["key"] and tabname == TAB:
            e["value"]["componentsData"] = page
            e["value"]["backgroundData"] = {"version": 1, "focussed": {"version": 1, "colour": "ff0e0e0e", "image": ""},
                                            "unfocussed": {"version": 1, "colour": "ff0e0e0e", "image": ""}}
            keep.append(e)
    cdefs["localComponentDefinitions"] = keep + sk.defs
    json.dump(tui, open(tui_path, "w"), indent=1)
    for f in os.listdir(skin):
        if f.endswith(".png") and f not in sk.used:
            os.remove(os.path.join(skin, f))
    print("panel_art: %d definitions, %d pictures, %d components" % (len(sk.defs), len(sk.used), len(kids)))


if __name__ == "__main__":
    main(sys.argv[1])
