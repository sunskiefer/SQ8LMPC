"""Builds editor/SQ8LMPC-panel-editor.html: the KutilXXL Panel Editor (tools/editor_template.html) fed with the SQ8L panel and layout.json."""
import base64, io, json, os, sys, tempfile
sys.path.insert(0, os.path.dirname(__file__))
from PIL import Image
import panel as P
import panel_art as A

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = json.load(open(os.path.join(ROOT, "layout.json")))


def b64(im, fmt="PNG", **kw):
    b = io.BytesIO(); im.save(b, fmt, **kw)
    return "data:image/%s;base64,%s" % (fmt.lower(), base64.b64encode(b.getvalue()).decode())


art = Image.open(os.path.join(A.ORIG, "background.png")).convert("RGBA").crop((P.ART_LEFT, P.ART_TOP, P.ART_RIGHT, P.ART_H)).resize((P.W, P.H), Image.LANCZOS)
orig = art.convert("RGB")
for l in RAW["labels"]:
    if l.get("wipe"):
        art = A.wipe(art, l["wipe"])
items = {i["id"]: i for i in RAW["items"]}
art = A.window_wells(art, items["vfd"], items["num"])
BG = b64(art.convert("RGB"), "JPEG", quality=88)
ORIG = b64(orig, "JPEG", quality=72)

pics = {}
for p in ("grey", "red", "teal", "small", "updn_up", "updn_down", "scr_up", "scr_dn"):
    sz = (90, 34) if p in ("grey", "red", "teal") else (61, 28) if p != "scr_up" and p != "scr_dn" else (41, 28)
    pics[p] = b64(A.button_images(p, *sz)[0 if p in ("scr_up", "scr_dn") else 1], "PNG")
pics["led"] = b64(A.led_images(28, 28)[0], "PNG")


class SK:
    def __init__(s, d): s.skin = d; s.used = set()


td = tempfile.mkdtemp()
n = A.knob_strip(SK(td), 112)
knob = Image.open(os.path.join(td, n)).crop((0, 64 * 112, 112, 65 * 112))
KNOBS = {"a": b64(knob, "PNG")}

PH = {"vfd": "DISPLAY  44 x 2", "num": "PROGRAM NUMBER", "pname": "VOYAGER"}
page_items, labels = [], []
for it in RAW["items"]:
    e = {"id": it["id"], "mod": "SQ8L", "role": it["id"], "x": it["x"], "y": it["y"], "w": it["w"], "h": it["h"]}
    if it["type"] == "knob":
        e.update(kind="k", knob="a")
    elif it["type"] == "button":
        e.update(kind="b", pic=it["pic"])
    elif it["type"] == "led":
        e.update(kind="b", pic="led")
    else:
        e.update(kind="d", ph=PH.get(it["id"], it["id"]), px=it.get("px"))
    page_items.append(e)
for l in RAW["labels"]:
    labels.append({"id": l["id"], "mod": "SQ8L", "role": l["id"], "text": l["text"], "fs": l["px"], "x": l["x"], "y": l["y"], "w": l["w"], "h": l["h"]})
model = {"page_w": P.W, "page_h": P.H, "typed": True,
         "pages": [{"screen": "SQ8L", "modules": [{"name": "SQ8L", "type": "SQ8L"}], "items": page_items, "labels": labels, "groups": []}]}

font = base64.b64encode(open(os.path.join(ROOT, "art/fonts/Michroma-Regular.ttf"), "rb").read()).decode()
html = open(os.path.join(os.path.dirname(__file__), "editor_template.html")).read()
for k, v in (("__BACKG__", BG), ("__LAYOUT__", json.dumps(model)), ("__KNOBS__", json.dumps(KNOBS)), ("__ORIG__", json.dumps([ORIG])),
             ("__PICS__", json.dumps(pics)), ("__RAW__", json.dumps(RAW)), ("__FONT__", font)):
    html = html.replace(k, v)
out = os.path.join(ROOT, "editor", "SQ8LMPC-panel-editor.html")
open(out, "w").write(html)
print("editor ->", out, len(html) // 1024, "KB")
