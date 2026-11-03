#!/usr/bin/env python3
"""Zankō 2006 app icon: a glossy Silent-Aero tile — an afterglow sun sinking behind a dead city and a torii, cyan rim.
Writes icon_512.png, icon_256.png, icon.ico (16-256) and favicon_64.png into the output folder."""
import sys, os, math
from PIL import Image, ImageDraw, ImageFilter, ImageFont
out = sys.argv[1]; os.makedirs(out, exist_ok=True)
S = 1024   # draw at 2x, downsample
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
pad, rad = 56, 210
mask = Image.new("L", (S, S), 0); ImageDraw.Draw(mask).rounded_rectangle([pad, pad, S - pad, S - pad], rad, fill=255)
# sky: teal night above, amber afterglow at the horizon
sky = Image.new("RGBA", (S, S))
sd = ImageDraw.Draw(sky)
for y in range(S):
    t = y / S
    if t < 0.62: k = t / 0.62; c = (int(16 + 40 * k), int(36 + 50 * k), int(44 + 40 * k))
    else: k = (t - 0.62) / 0.38; c = (int(56 + 140 * (1 - k) * 0.9), int(86 + 30 * (1 - k)), int(84 - 40 * k))
    sd.line([(0, y), (S, y)], fill=c + (255,))
# the sun: a big low disc with a soft glow
glow = Image.new("RGBA", (S, S), (0, 0, 0, 0)); gd = ImageDraw.Draw(glow)
cx, cy, R = S * 0.5, S * 0.64, S * 0.24
for k in range(30, 0, -1):
    r = R * (1 + k * 0.06); a = int(9 * (30 - k) / 30 * 3)
    gd.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(255, 150, 70, a))
glow = glow.filter(ImageFilter.GaussianBlur(18)); sky.alpha_composite(glow)
sun = Image.new("RGBA", (S, S), (0, 0, 0, 0)); sdd = ImageDraw.Draw(sun)
for y in range(int(cy - R), int(cy + R)):
    t = (y - (cy - R)) / (2 * R); w = math.sqrt(max(0, R * R - (y - cy) ** 2))
    col = (255, int(214 - 110 * t), int(130 - 90 * t), 255)
    if int((y - (cy - R)) / 9) % 3 == 2 and t > 0.45: continue   # retro scan gaps in the lower half
    sdd.line([(cx - w, y), (cx + w, y)], fill=col)
sky.alpha_composite(sun)
# skyline + torii silhouette
sil = Image.new("RGBA", (S, S), (0, 0, 0, 0)); d = ImageDraw.Draw(sil); ink = (10, 16, 20, 255)
base = S * 0.74
blds = [(0.10, 0.20, 0.30), (0.19, 0.26, 0.45), (0.25, 0.33, 0.36), (0.62, 0.70, 0.50), (0.69, 0.78, 0.38), (0.77, 0.88, 0.58), (0.86, 0.95, 0.42)]
for x0, x1, h in blds:
    d.rectangle([S * x0, base - S * h * 0.5, S * x1, S], fill=ink)
    for wy in range(int(base - S * h * 0.5 + 24), int(base), 34):
        for wx in range(int(S * x0 + 14), int(S * x1 - 14), 30):
            if (wx * 7 + wy * 3) % 5 == 0: d.rectangle([wx, wy, wx + 10, wy + 14], fill=(255, 180, 90, 255))
# torii in front of the sun
tx, tw, th = S * 0.5, S * 0.30, S * 0.30; red = (178, 40, 30, 255)
d.rectangle([tx - tw * 0.36, base - th, tx - tw * 0.28, S], fill=red); d.rectangle([tx + tw * 0.28, base - th, tx + tw * 0.36, S], fill=red)
d.polygon([(tx - tw * 0.58, base - th - 30), (tx + tw * 0.58, base - th - 30), (tx + tw * 0.5, base - th + 12), (tx - tw * 0.5, base - th + 12)], fill=(24, 18, 18, 255))
d.rectangle([tx - tw * 0.46, base - th + 46, tx + tw * 0.46, base - th + 70], fill=red)
d.rectangle([0, base + 10, S, S], fill=ink)
sky.alpha_composite(sil)
# the kanji, cream, at the bottom
try:
    f = ImageFont.truetype("/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc", 150, index=0)
    dd = ImageDraw.Draw(sky); t = "残光"; w = dd.textlength(t, font=f)
    dd.text(((S - w) / 2 + 5, S * 0.775 + 5), t, font=f, fill=(0, 0, 0, 200)); dd.text(((S - w) / 2, S * 0.775), t, font=f, fill=(238, 230, 210, 255))
except Exception as e: print("font", e)
tile = Image.new("RGBA", (S, S), (0, 0, 0, 0)); tile.paste(sky, (0, 0), mask)
# glass: split gloss on the top half, rim light, dark outline
gloss = Image.new("RGBA", (S, S), (0, 0, 0, 0)); gl = ImageDraw.Draw(gloss)
gl.rounded_rectangle([pad + 18, pad + 14, S - pad - 18, S * 0.47], rad - 20, fill=(255, 255, 255, 46))
gm = Image.new("L", (S, S), 0); ImageDraw.Draw(gm).rounded_rectangle([pad, pad, S - pad, S - pad], rad, fill=255)
tile.alpha_composite(Image.composite(gloss, Image.new("RGBA", (S, S), (0, 0, 0, 0)), gm))
rim = Image.new("RGBA", (S, S), (0, 0, 0, 0)); rd = ImageDraw.Draw(rim)
rd.rounded_rectangle([pad, pad, S - pad, S - pad], rad, outline=(6, 10, 12, 255), width=22)
rd.rounded_rectangle([pad + 16, pad + 16, S - pad - 16, S - pad - 16], rad - 16, outline=(111, 214, 208, 170), width=8)
tile.alpha_composite(rim)
shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0)); ImageDraw.Draw(shadow).rounded_rectangle([pad + 14, pad + 26, S - pad + 14, S - pad + 26], rad, fill=(0, 0, 0, 120))
shadow = shadow.filter(ImageFilter.GaussianBlur(16)); shadow.alpha_composite(tile)
final = shadow.resize((512, 512), Image.LANCZOS)
final.save(os.path.join(out, "icon_512.png")); final.resize((256, 256), Image.LANCZOS).save(os.path.join(out, "icon_256.png"))
final.resize((64, 64), Image.LANCZOS).save(os.path.join(out, "favicon_64.png"))
final.save(os.path.join(out, "icon.ico"), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("ok")
