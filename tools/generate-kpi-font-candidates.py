#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
import base64

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/display/fonts/KpiFontCandidates.h"
FONT = ROOT / "tools/fonts/kpi-candidates/Quantico-Bold.ttf"

SIZES = (24, 28, 32, 36)
ASCII_KPI = " 0123456789.,+-:%/°FVEkWhADnseCPřlišžťoučkýůňČeskBrodVýbaSíÁĎÉĚÍŇÓŘŠŤÚŮÝŽáďéěíóřšťúý"
CHARS = "".join(dict.fromkeys(ASCII_KPI))

def validate_charset(path: Path):
    tt = TTFont(str(path), fontNumber=0)
    cmap = tt.getBestCmap() or {}
    missing = [ch for ch in CHARS if ord(ch) not in cmap]
    if missing:
        raise SystemExit(
            "Quantico-Bold.ttf is missing required glyphs: " +
            " ".join(f"{ch}(U+{ord(ch):04X})" for ch in missing)
        )
    print(f"Quantico charset OK: {len(CHARS)} glyphs")

def choose_size(path: Path, target_height: int):
    probe = "H0123456789FVEWkWhČŘŽ"
    best = None
    for size in range(10, 96):
        font = ImageFont.truetype(str(path), size=size)
        box = font.getbbox(probe, anchor="ls")
        h = box[3] - box[1]
        score = abs(h - target_height)
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

def make_face(target_height: int):
    name = f"Quantico{target_height}"
    font = choose_size(FONT, target_height)

    reference = "H0123456789FVEWČŘŽ"
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
        img = img.point(lambda p: 255 if p >= 128 else 0)

        offset = len(all_bytes)
        all_bytes.extend(pack_bitmap(img))
        yoff = top - ref_top
        glyphs.append((code, w, h, adv, left, yoff, offset))

    encoded = base64.b64encode(bytes(all_bytes)).decode("ascii")
    line_height = target_height + 6

    lines = []
    lines.append(f"static const char {name}BitmapBase64[] PROGMEM =")
    for i in range(0, len(encoded), 96):
        lines.append(f'    "{encoded[i:i+96]}"')
    lines[-1] += ";"
    lines.append(f"static const Glyph {name}Glyphs[] PROGMEM = {{")
    for g in glyphs:
        lines.append("    {%d,%d,%d,%d,%d,%d,%d}," % g)
    lines.append("};")
    lines.append(
        f"static const Face {name}Face = "
        f"{{{target_height},{line_height},{name}Glyphs,{len(glyphs)},{name}BitmapBase64}};"
    )
    return "\n".join(lines)

validate_charset(FONT)

parts = [
    "#pragma once",
    '#include "../DashboardSansV2Types.h"',
    "",
    "namespace KpiFontCandidates {",
    "using DashboardSansV2::Glyph;",
    "using DashboardSansV2::Face;",
    "",
]
for size in SIZES:
    parts.append(make_face(size))
    parts.append("")
parts.append("} // namespace KpiFontCandidates")
parts.append("")

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text("\n".join(parts), encoding="utf-8")
print(f"Wrote {OUT}")
