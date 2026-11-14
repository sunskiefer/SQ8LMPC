#!/usr/bin/env python3
"""Extract the original SQ8L artwork (src/gui/data of sq8l-revival/SQ8L) to PNG, pixel for pixel.
   usage: extract_art.py <SQ8L repo> <out dir>"""
import re, sys, os
from PIL import Image
repo, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
H = open(os.path.join(repo, 'src/gui/data/GuiData.h')).read()
bw = int(re.search(r'kBackgroundWidth = (\d+)', H).group(1)); bh = int(re.search(r'kBackgroundHeight = (\d+)', H).group(1))
def arr(src, name):
    m = re.search(name + r'\[[^\]]*\]\s*=\s*\{(.*?)\};', src, re.S)
    return [int(x, 0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+', m.group(1))]
bgsrc = open(os.path.join(repo, 'src/gui/data/GuiBackground.cpp')).read()
pal = arr(bgsrc, r'kBackgroundPalette'); idx = arr(bgsrc, r'kBackgroundIndex')
im = Image.new('RGB', (bw, bh))
im.putdata([((pal[i] >> 16) & 255, (pal[i] >> 8) & 255, pal[i] & 255) for i in idx])
im.save(os.path.join(out, 'background.png')); print('background', im.size)
sp = open(os.path.join(repo, 'src/gui/data/GuiSprites.cpp')).read()
for m in re.finditer(r'\{\s*"(\w+)",\s*(\d+),\s*(\d+),\s*(\d+),\s*(\w+),\s*(\d+),\s*(\w+)\s*\}', sp):
    name, fw, fh, n, palname, psz, idxname = m.groups(); fw, fh, n = int(fw), int(fh), int(n)
    pal = arr(sp, palname); idx = arr(sp, idxname)
    assert len(idx) == fw * fh * n, (name, len(idx))
    strip = Image.new('RGB', (fw, fh * n))
    strip.putdata([((pal[i] >> 16) & 255, (pal[i] >> 8) & 255, pal[i] & 255) for i in idx])
    strip.save(os.path.join(out, name + '_strip.png')); print(name, fw, fh, n)
