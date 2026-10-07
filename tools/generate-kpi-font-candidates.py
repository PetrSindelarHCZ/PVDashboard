#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.misc.transform import Transform
import base64
import tempfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/display/fonts/KpiFontCandidates.h"
SOURCE_FONT = ROOT / "tools/fonts/kpi-candidates/Quantico-Bold.ttf"

SIZES = (24, 28, 32, 36)
CHARS = "".join(dict.fromkeys(
    " 0123456789.,+-:%/°"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ"
    "áčďéěíňóřšťúůýž"
))

# Missing Czech glyphs in the distributed Quantico Bold. We build a real
# derived TTF first, at vector-outline level, then rasterize that font. Accent
# placement follows Quantico's own Glyphs source geometry:
#   lower-case caron: spacing caron at its native y
#   upper-case caron: same caron shifted +195 font units
#   lower-case ring: spacing ring at native y
#   upper-case ring: same ring shifted +50 font units
# Czech d/ť use a small apostrophe-like caron at the upper-right.
SPECS = {
    "č": ("c", "caron", 0),
    "Č": ("C", "caron", 195),
    "ď": ("d", "apostrophe", 0),
    "Ď": ("D", "caron", 195),
    "ě": ("e", "caron", 0),
    "Ě": ("E", "caron", 195),
    "ň": ("n", "caron", 0),
    "Ň": ("N", "caron", 195),
    "ť": ("t", "apostrophe", 0),
    "Ť": ("T", "caron", 195),
    "ů": ("u", "ring", 0),
    "Ů": ("U", "ring", 50),
}

def unicode_cmap(tt):
    cmap = {}
    for table in tt["cmap"].tables:
        if table.isUnicode():
            cmap.update(table.cmap)
    return cmap

def glyph_advance(tt, glyph_name):
    return tt["hmtx"].metrics[glyph_name][0]

def center_dx(tt, base_name, mark_name):
    return round((glyph_advance(tt, base_name) - glyph_advance(tt, mark_name)) / 2)

def add_composite(tt, target_char, base_char, mark_kind, y_shift):
    cmap = unicode_cmap(tt)
    base_name = cmap[ord(base_char)]
    glyph_set = tt.getGlyphSet()

    if mark_kind == "caron":
        mark_name = cmap[0x02C7]  # spacing caron
        dx = center_dx(tt, base_name, mark_name)
        transform = Transform(1, 0, 0, 1, dx, y_shift)
    elif mark_kind == "ring":
        mark_name = cmap[0x02DA]  # spacing ring

        # The stock Quantico spacing ring is visually too large when reused
        # directly on U/u at our 1-bit KPI sizes. Scale it down while keeping
        # the original Quantico outline, then center it over the base glyph.
        scale = 0.75 if target_char == "ů" else 0.80
        base_adv = glyph_advance(tt, base_name)
        mark_adv = glyph_advance(tt, mark_name) * scale
        dx = round((base_adv - mark_adv) / 2)

        # Lift the smaller ring slightly so it remains clearly detached from
        # the U/u at 24..36 px.
        dy = y_shift + (25 if target_char == "ů" else 20)
        transform = Transform(scale, 0, 0, scale, dx, dy)
    elif mark_kind == "apostrophe":
        # Quantico's own quotesingle outline, reduced to a Czech d/t caron.
        # Keep it close to the base rather than looking like a separate quote.
        mark_name = cmap[0x0027]
        base_adv = glyph_advance(tt, base_name)
        # 70% retains Quantico's stroke character but shortens the mark.
        scale = 0.70
        mark_adv = glyph_advance(tt, mark_name) * scale
        dx = round(base_adv - mark_adv * 0.55)
        dy = 70

        # The caron on Czech ť needs a little more breathing room than on ď.
        # Move it slightly right and up in font units so it does not visually
        # stick to the top-right of the t at 24..36 px.
        if target_char == "ť":
            dx += 35
            dy += 35

        transform = Transform(scale, 0, 0, scale, dx, dy)
    else:
        raise ValueError(mark_kind)

    target_name = f"uni{ord(target_char):04X}"
    pen = TTGlyphPen(glyph_set)
    pen.addComponent(base_name, Transform())
    pen.addComponent(mark_name, transform)
    glyph = pen.glyph()

    if target_name not in tt.getGlyphOrder():
        tt.setGlyphOrder(tt.getGlyphOrder() + [target_name])
    tt["glyf"].glyphs[target_name] = glyph
    tt["hmtx"].metrics[target_name] = tt["hmtx"].metrics[base_name]

    for table in tt["cmap"].tables:
        if table.isUnicode() and table.format in (4, 12, 13):
            table.cmap[ord(target_char)] = target_name

def rename_derived_font(tt):
    # Reserved Font Name "Quantico" must not be used for the modified font.
    replacements = {
        1: "Dashboard KPI",
        2: "Bold",
        4: "Dashboard KPI Bold",
        6: "DashboardKPI-Bold",
    }
    for rec in tt["name"].names:
        if rec.nameID not in replacements:
            continue
        value = replacements[rec.nameID]
        if rec.isUnicode():
            rec.string = value.encode("utf-16-be")
        else:
            try:
                rec.string = value.encode("mac_roman")
            except UnicodeEncodeError:
                rec.string = value.encode("ascii", errors="ignore")

def build_derived_font(out_path: Path):
    tt = TTFont(str(SOURCE_FONT))
    initial = unicode_cmap(tt)

    needed = [ch for ch in SPECS if ord(ch) not in initial]
    for ch in needed:
        base, mark, y_shift = SPECS[ch]
        add_composite(tt, ch, base, mark, y_shift)

    rename_derived_font(tt)
    tt.save(str(out_path))

    check = TTFont(str(out_path))
    cmap = unicode_cmap(check)
    missing = [ch for ch in CHARS if ord(ch) not in cmap]
    if missing:
        raise SystemExit(
            "Dashboard KPI derived font still misses: " +
            " ".join(f"{ch}(U+{ord(ch):04X})" for ch in missing)
        )

    print("Dashboard KPI derived TTF OK")
    print("Added Czech glyphs: " + " ".join(needed))

def choose_size(path: Path, target_height: int):
    probe = "H0123456789FVEWÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ"
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

def make_face(font_path: Path, target_height: int):
    name = f"Quantico{target_height}"
    font = choose_size(font_path, target_height)

    # Common visual top includes all Czech caps, so diacritics never clip.
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
        glyphs.append((code, w, h, adv, left, top - ref_top, offset))

    encoded = base64.b64encode(bytes(all_bytes)).decode("ascii")
    line_height = target_height + max(8, round(target_height * 0.25))

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

with tempfile.TemporaryDirectory() as tmp:
    derived = Path(tmp) / "DashboardKPI-Bold.ttf"
    build_derived_font(derived)

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
        parts.append(make_face(derived, size))
        parts.append("")
    parts.append("} // namespace KpiFontCandidates")
    parts.append("")

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text("\n".join(parts), encoding="utf-8")
print(f"Wrote {OUT}")
