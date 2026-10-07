#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import base64

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/display/fonts/KpiFontCandidates.h"
FONT_DIR = ROOT / "tools/fonts/kpi-candidates"

CANDIDATES = [
    ("ChakraPetch", FONT_DIR / "ChakraPetch-Bold.ttf"),
    ("Quantico", FONT_DIR / "Quantico-Bold.ttf"),
    ("Aldrich", FONT_DIR / "Aldrich-Regular.ttf"),
]

TARGET_HEIGHT = 32
CHARS = " 0123456789.,+-:%/FVEkWhADnseC"

def choose_size(path: Path):
    probe = "H0123456789FVEWkWh"
    best = None
    for size in range(12, 80):
        font = ImageFont.truetype(str(path), size=size)
        box = font.getbbox(probe, anchor="ls")
        h = box[3] - box[1]
        score = abs(h - TARGET_HEIGHT)
        if best is None or score < best[0]:
            best = (score, size, font)
    return best[2]

def pack_bitmap(img):
    w, h = img.size
    stride = (w + 7) // 8
    data = bytearray(stride * h)
    pix = img.load()
    for y in range(h):
        for x in range(w):
            if pix[x, y] >= 128:
                data[y * stride + x // 8] |= 0x80 >> (x & 7)
    return bytes(data)

def make_face(name, path):
    font = choose_size(path)

    # Common visual top based on uppercase/digits, with baseline at y=0.
    reference = "H0123456789FVEW"
    rb = font.getbbox(reference, anchor="ls")
    ref_top = rb[1]

    glyphs = []
    all_bytes = bytearray()

    for ch in CHARS:
        code = ord(ch)
        if ch == " ":
            adv = max(1, round(font.getlength(ch)))
            glyphs.append((code, 0, 0, adv, 0, 0, len(all_bytes)))
            continue

        box = font.getbbox(ch, anchor="ls")
        left, top, right, bottom = box
        w = max(1, right - left)
        h = max(1, bottom - top)
        adv = max(1, round(font.getlength(ch)))

        img = Image.new("L", (w, h), 0)
        draw = ImageDraw.Draw(img)
        draw.text((-left, -top), ch, font=font, fill=255, anchor="ls")
        # Crisp 1-bit panel threshold.
        img = img.point(lambda p: 255 if p >= 128 else 0)

        offset = len(all_bytes)
        all_bytes.extend(pack_bitmap(img))
        yoff = top - ref_top
        glyphs.append((code, w, h, adv, left, yoff, offset))

    encoded = base64.b64encode(bytes(all_bytes)).decode("ascii")
    line_height = TARGET_HEIGHT + 6

    lines = []
    lines.append(f"static const char {name}32BitmapBase64[] PROGMEM =")
    for i in range(0, len(encoded), 96):
        lines.append(f'    "{encoded[i:i+96]}"')
    lines[-1] += ";"
    lines.append(f"static const Glyph {name}32Glyphs[] PROGMEM = {{")
    for g in glyphs:
        lines.append("    {%d,%d,%d,%d,%d,%d,%d}," % g)
    lines.append("};")
    lines.append(
        f"static const Face {name}32Face = "
        f"{{32,{line_height},{name}32Glyphs,{len(glyphs)},{name}32BitmapBase64}};"
    )
    return "\n".join(lines)

parts = [
    "#pragma once",
    '#include "../DashboardSansV2Types.h"',
    "",
    "namespace KpiFontCandidates {",
    "using DashboardSansV2::Glyph;",
    "using DashboardSansV2::Face;",
    "",
]
for name, path in CANDIDATES:
    parts.append(make_face(name, path))
    parts.append("")
parts.append("} // namespace KpiFontCandidates")
parts.append("")

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text("\n".join(parts), encoding="utf-8")
print(f"Wrote {OUT}")
