#!/usr/bin/env python3
"""Subset Noto Sans CJK JP Bold to the characters in zk_jp.h (+ ASCII) and convert CFF -> TrueType (glyf) so
stb_truetype loads it everywhere. Output: Data/UI/jp.ttf"""
import sys, re
from fontTools.ttLib import TTCollection, TTFont, newTable
from fontTools import subset
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.ttGlyphPen import TTGlyphPen

src, hdr, out = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(hdr, encoding="utf-8").read()
chars = set(c for c in text if ord(c) > 127) | set(chr(c) for c in range(32, 127))
coll = TTCollection(src)
font = next(f for f in coll.fonts if "JP" in f["name"].getDebugName(1))
opts = subset.Options(); opts.layout_features = []; opts.name_IDs = ["*"]; opts.notdef_outline = True
sub = subset.Subsetter(opts); sub.populate(unicodes=[ord(c) for c in chars]); sub.subset(font)
# CFF -> glyf
gs = font.getGlyphSet(); order = font.getGlyphOrder(); glyf = {}
for name in order:
    pen = TTGlyphPen(gs); gs[name].draw(Cu2QuPen(pen, 1.0, reverse_direction=True)); glyf[name] = pen.glyph()
font["glyf"] = newTable("glyf"); font["glyf"].glyphOrder = order; font["glyf"].glyphs = glyf
font["loca"] = newTable("loca"); del font["CFF "]
if "VORG" in font: del font["VORG"]
font["maxp"].tableVersion = 0x00010000
for k in ("maxZones", "maxTwilightPoints", "maxStorage", "maxFunctionDefs", "maxInstructionDefs", "maxStackElements", "maxSizeOfInstructions", "maxComponentElements"):
    setattr(font["maxp"], k, 0 if k != "maxZones" else 1)
font["maxp"].maxComponentDepth = 0
font["head"].glyphDataFormat = 0; font["head"].indexToLocFormat = 0
font["post"].formatType = 3.0
font.sfntVersion = "\0\1\0\0"
font.save(out)
print(out, len(chars), "chars")
