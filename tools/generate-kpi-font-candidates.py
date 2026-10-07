#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
import base64

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/display/fonts/KpiFontCandidates.h"
FONT = ROOT / "tools/fonts/kpi-candidates/Quantico-Bold.ttf"

SIZES = (24, 28, 32, 36)

# Dashboard KPI charset. Keep it deliberately compact for flash usage, but
# include the complete Czech alphabet used by dashboard labels.
CHARS = "".join(dict.fromkeys(
    " 0123456789.,+-:%/°"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ"
    "áčďéěíňóřšťúůýž"
))

# Quantico declares latin-ext, but the distributed Bold face is missing a
# handful of precomposed Czech glyphs. Build those at generation time from the
# base glyph plus a combining mark. The firmware still receives ordinary,
# pre-rasterized 1-bit glyphs.
COMPOSE = {
    "č": "c\u030C", "Č": "C\u030C",
    "ď": "d\u030C", "Ď": "D\u030C",
    "ě": "e\u030C", "Ě": "E\u030C",
    "ň": "n\u030C", "Ň": "N\u030C",
    "ť": "t\u030C", "Ť": "T\u030C",
    "ů": "u\u030A", "Ů": "U\u030A",
}

def best_cmap(path: Path):
    tt = TTFont(str(path), fontNumber=0)
    return tt.getBestCmap() or {}

def validate_charset(path: Path):
    cmap = best_cmap(path)
    missing = []
    composed = []

    for ch in CHARS:
        cp = ord(ch)
        if cp in cmap:
            continue

        sequence = COMPOSE.get(ch)
        if sequence and all(ord(part) in cmap for part in sequence):
            composed.append(ch)
            continue

        missing.append(ch)

    if missing:
        raise SystemExit(
            "Quantico-Bold.ttf cannot provide required glyphs: " +
            " ".join(f"{ch}(U+{ord(ch):04X})" for ch in missing)
        )

    print(f"Dashboard KPI charset OK: {len(CHARS)} glyphs")
    if composed:
        print("Synthesized Czech glyphs: " + " ".join(composed))

def render_sequence_for(ch: str, cmap):
    if ord(ch) in cmap:
        return ch
    return COMPOSE[ch]

def choose_size(path: Path, target_height: int):
    probe = "H0123456789FVEWČŘŽ"
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

def make_face(target_height: int, cmap):
    name = f"Quantico{target_height}"
    font = choose_size(FONT, target_height)

    # Use Czech capitals as well so accents fit inside the common top metric.
    reference = "H0123456789FVEWÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ"
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

        text = render_sequence_for(ch, cmap)

        box = font.getbbox(text, anchor="ls")
        left, top, right, bottom = box
        w = max(1, right - left)
        h = max(1, bottom - top)

        # Keep the advance of the base character for synthesized glyphs.
        base_for_advance = text[0]
        adv = max(1, round(font.getlength(base_for_advance)))

        img = Image.new("L", (w, h), 0)
        draw = ImageDraw.Draw(img)
        draw.text((-left, -top), text, font=font, fill=255, anchor="ls")
        img = img.point(lambda p: 255 if p >= 128 else 0)

        offset = len(all_bytes)
        all_bytes.extend(pack_bitmap(img))
        yoff = top - ref_top
        glyphs.append((code, w, h, adv, left, yoff, offset))

    encoded = base64.b64encode(bytes(all_bytes)).decode("ascii")
    line_height = target_height + 8

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

cmap = best_cmap(FONT)
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
    parts.append(make_face(size, cmap))
    parts.append("")
parts.append("} // namespace KpiFontCandidates")
parts.append("")

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text("\n".join(parts), encoding="utf-8")
print(f"Wrote {OUT}")
