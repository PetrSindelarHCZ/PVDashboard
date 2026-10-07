#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
import base64
import os

ROOT = Path(__file__).resolve().parents[1]
OUT_FACES = ROOT / "src/display/fonts/DashboardSansV2FullFaces.h"
OUT_API = ROOT / "src/display/DashboardSansV2.h"

SIZES = (8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36)
ASCII = "".join(chr(i) for i in range(32, 127))
CZECH = "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽáčďéěíňóřšťúůýž"
EXTRA = "°−×±←→↑↓"
CHARS = "".join(dict.fromkeys(ASCII + CZECH + EXTRA))

DEFAULT_REGULAR = "/usr/share/fonts/opentype/inter/InterDisplay-Regular.otf"
DEFAULT_BOLD = "/usr/share/fonts/opentype/inter/InterDisplay-Bold.otf"
FONTS = {
    "Regular": Path(os.environ.get("DASHBOARD_SANS_REGULAR", DEFAULT_REGULAR)),
    "Bold": Path(os.environ.get("DASHBOARD_SANS_BOLD", DEFAULT_BOLD)),
}

def validate_font(path: Path):
    if not path.exists():
        raise SystemExit(f"Font not found: {path}")
    tt = TTFont(str(path))
    cmap = tt.getBestCmap() or {}
    missing = [ch for ch in CHARS if ord(ch) not in cmap]
    if missing:
        raise SystemExit(
            f"{path.name} missing required glyphs: " +
            " ".join(f"{ch}(U+{ord(ch):04X})" for ch in missing)
        )

def choose_size(path: Path, target_height: int):
    # V2 nominal size is the ink height of ordinary capitals/digits, not the
    # font's em size. This preserves the proportions of the approved V2 raster.
    probe = "H0123456789FVEWkWh"
    best = None
    for size in range(6, 96):
        font = ImageFont.truetype(str(path), size=size)
        box = font.getbbox(probe, anchor="ls")
        height = box[3] - box[1]
        score = abs(height - target_height)
        candidate = (score, abs(size - target_height * 4 / 3), size, font, height)
        if best is None or candidate[:2] < best[:2]:
            best = candidate
    return best[3], best[2], best[4]

def pack_bitmap(img):
    w, h = img.size
    stride = (w + 7) // 8
    data = bytearray(stride * h)
    pixels = img.load()
    for y in range(h):
        for x in range(w):
            if pixels[x, y] >= 128:
                data[y * stride + x // 8] |= 0x80 >> (x & 7)
    return bytes(data)

def make_face(weight: str, target_height: int):
    path = FONTS[weight]
    font, source_size, actual_height = choose_size(path, target_height)

    # Keep drawText(top) compatible with the existing V2 convention:
    # top=0 is the top of unaccented capitals/digits. Accents may therefore
    # have a small negative yOffset instead of moving all ordinary text down.
    ref_top = font.getbbox("H0123456789FVEWkWh", anchor="ls")[1]

    glyphs = []
    bitmap = bytearray()
    min_y = 127
    max_bottom = -127

    for ch in CHARS:
        code = ord(ch)

        if ch == " ":
            advance = max(1, round(font.getlength(ch)))
            glyphs.append((code, 0, 0, advance, 0, 0, len(bitmap)))
            continue

        left, top, right, bottom = font.getbbox(ch, anchor="ls")
        width = max(1, right - left)
        height = max(1, bottom - top)
        advance = max(1, round(font.getlength(ch)))

        image = Image.new("L", (width, height), 0)
        draw = ImageDraw.Draw(image)
        draw.text((-left, -top), ch, font=font, fill=255, anchor="ls")
        image = image.point(lambda p: 255 if p >= 128 else 0)

        offset = len(bitmap)
        bitmap.extend(pack_bitmap(image))
        y_offset = top - ref_top
        min_y = min(min_y, y_offset)
        max_bottom = max(max_bottom, y_offset + height)
        glyphs.append(
            (code, width, height, advance, left, y_offset, offset)
        )

    encoded = base64.b64encode(bytes(bitmap)).decode("ascii")
    name = f"{weight}{target_height}"
    line_height = max_bottom - min_y + max(2, target_height // 6)

    lines = [
        f"// {weight} {target_height}px; Inter Display source size "
        f"{source_size}px; cap/digit ink {actual_height}px",
        f"static const char {name}BitmapBase64[] PROGMEM =",
    ]
    for i in range(0, len(encoded), 96):
        lines.append(f'    "{encoded[i:i+96]}"')
    lines[-1] += ";"

    lines.append(f"static const Glyph {name}Glyphs[] PROGMEM = {{")
    for glyph in glyphs:
        lines.append("    {%d,%d,%d,%d,%d,%d,%d}," % glyph)
    lines.append("};")
    lines.append(
        f"static const Face {name}Face = "
        f"{{{target_height},{line_height},{name}Glyphs,"
        f"{len(glyphs)},{name}BitmapBase64}};"
    )

    print(
        f"{weight:7} {target_height:2}px -> source {source_size:2}px, "
        f"{len(glyphs)} glyphs, {len(bitmap)} bitmap bytes"
    )
    return "\n".join(lines)

def write_faces():
    parts = [
        "#pragma once",
        '#include "../DashboardSansV2Types.h"',
        "",
        "namespace DashboardSansV2 {",
        "",
    ]
    for size in SIZES:
        parts.append(make_face("Regular", size))
        parts.append("")
        parts.append(make_face("Bold", size))
        parts.append("")
    parts.append("} // namespace DashboardSansV2")
    parts.append("")
    OUT_FACES.parent.mkdir(parents=True, exist_ok=True)
    OUT_FACES.write_text("\n".join(parts), encoding="utf-8")

def write_api():
    includes = [
        "#pragma once",
        '#include "DashboardSansV2Types.h"',
        '#include "fonts/DashboardSansV2FullFaces.h"',
        "",
        "namespace DashboardSansV2 {",
        "",
        "inline const Face* face(uint8_t px, Weight weight) {",
        "    if (weight == Weight::Regular) {",
        "        switch (px) {",
    ]
    for size in SIZES:
        includes.append(f"            case {size}: return &Regular{size}Face;")
    includes += [
        "            default: return nullptr;",
        "        }",
        "    }",
        "",
        "    switch (px) {",
    ]
    for size in SIZES:
        includes.append(f"        case {size}: return &Bold{size}Face;")
    includes += [
        "        default: return nullptr;",
        "    }",
        "}",
        "",
        "inline bool hasFace(uint8_t px, Weight weight) {",
        "    return face(px, weight) != nullptr;",
        "}",
        "",
        "inline int16_t textWidth(",
        "    const String& text,",
        "    uint8_t px,",
        "    Weight weight) {",
        "",
        "    const Face* selected = face(px, weight);",
        "    return selected != nullptr",
        "        ? DashboardSansV2::textWidth(*selected, text)",
        "        : 0;",
        "}",
        "",
        "inline void drawText(",
        "    IDisplay& display,",
        "    int16_t x,",
        "    int16_t top,",
        "    const String& text,",
        "    uint8_t px,",
        "    Weight weight,",
        "    uint16_t color = 0) {",
        "",
        "    const Face* selected = face(px, weight);",
        "    if (selected == nullptr) return;",
        "",
        "    DashboardSansV2::drawText(",
        "        display, x, top, *selected, text, color);",
        "}",
        "",
        "} // namespace DashboardSansV2",
        "",
    ]
    OUT_API.write_text("\n".join(includes), encoding="utf-8")

for path in FONTS.values():
    validate_font(path)

print(f"Dashboard Sans V2 charset: {len(CHARS)} glyphs")
write_faces()
write_api()
print(f"Wrote {OUT_FACES}")
print(f"Wrote {OUT_API}")
