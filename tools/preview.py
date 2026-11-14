#!/usr/bin/env python3
"""Composite the built skin's pictures at a parameter state (key<TAB>value lines) -> a PNG of the panel as MPC draws it.
    python3 tools/preview.py "<skin>/Plugin Skins" state.tsv out.png [overlay]
Offline composite only: it reads TUI.json and the pictures, not the device. `overlay` numbers every control by id."""
import json
import os
import sys

from PIL import Image, ImageDraw, ImageFont


def main(skin, state_file, out, overlay=False):
    st = dict(l.rstrip("\n").split("\t", 1) for l in open(state_file) if "\t" in l)
    params = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "params.json")))["params"]
    keys = [p["key"] for p in params]
    tui = json.load(open(os.path.join(skin, "TUI.json")))
    defs = {d["key"]: d for d in tui["pageData"]["componentDefinitions"]["localComponentDefinitions"]}
    page = next(d for k, d in defs.items() if "|" in k)
    canvas = Image.new("RGBA", (1280, 628), (14, 14, 14, 255))
    cache = {}

    def img(name):
        if name not in cache:
            cache[name] = Image.open(os.path.join(skin, name)).convert("RGBA")
        return cache[name]

    def parts(comp, ox, oy, idx):
        d = defs[comp["componentData"]["type"]]["value"]["componentsData"]
        for p in d:
            t = p["componentData"]["type"]
            x, y, w, h = (int(v) for v in p["bounds"]["bounds"].split())
            data = p["componentData"]["data"]
            if t == "Image":
                inv = p["bounds"].get("additionalInvalidatingHandles") or []
                if inv:
                    o, c, _ = inv[0].split("/")[1:4]
                    v = int(float(st.get(keys[idx], "0")))
                    if int(o) != v:
                        continue
                canvas.alpha_composite(img(data["image"]), (ox + x, oy + y))
            elif t == "Knob":
                n = data["numFrames"]
                fim = img(data["filmStrip"])
                fh = fim.size[1] // n
                v = float(st.get(keys[idx], "0"))
                k = max(0, min(n - 1, int(round(v * (n - 1)))))
                canvas.alpha_composite(fim.crop((0, k * fh, fim.size[0], (k + 1) * fh)), (ox + x, oy + y))
            elif t == "Button":
                canvas.alpha_composite(img(data["offImage"]), (ox + x, oy + y))
            elif t == "Label":
                txt = st.get(keys[idx], "")
                f = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", data["textStyle"]["font"]["height"])
                ImageDraw.Draw(canvas).text((ox + x + w / 2, oy + y + h / 2), txt, font=f, fill=(255, 70, 60, 255), anchor="mm")

    for comp in page["value"]["componentsData"]:
        t = comp["componentData"]["type"]
        x, y, w, h = (int(v) for v in comp["bounds"]["bounds"].split())
        if t == "Image":
            canvas.alpha_composite(img(comp["componentData"]["data"]["image"]), (x, y))
            continue
        m = comp["handle remapping"]["map"]
        idx = int(m[0]["value"].split()[1])
        parts(comp, x, y, idx)
        if overlay:
            ImageDraw.Draw(canvas).rectangle([x, y, x + w, y + h], outline=(255, 160, 0, 200))
    canvas.convert("RGB").save(out)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3], len(sys.argv) > 4)
