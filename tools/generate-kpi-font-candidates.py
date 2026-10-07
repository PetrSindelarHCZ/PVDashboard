#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont
import base64

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src/display/fonts/KpiFontCandidates.h"
FONT = ROOT / "tools/fonts/kpi-candidates/Quantico-Bold.ttf"

SIZES = (24, 28, 32, 36)

CHARS = "".join(dict.fromkeys(
    " 0123456789.,+-:%/°"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "ÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ"
    "áčďéěíňóřšťúůýž"
))

# Quantico Bold lacks these precomposed Czech glyphs. We derive them during
# raster generation so the ESP32 gets ordinary, self-contained 1-bit glyphs.
SYNTH = {
    "č": ("c", "caron"), "Č": ("C", "caron"),
    "ď": ("d", "apostrophe"), "Ď": ("D", "caron"),
    "ě": ("e", "caron"), "Ě": ("E", "caron"),
    "ň": ("n", "caron"), "Ň": ("N", "caron"),
    "ť": ("t", "apostrophe"), "Ť": ("T", "caron"),
    "ů": ("u", "ring"), "Ů": ("U", "ring"),
}

# Accent donors already drawn by the original Quantico designer. We extract
# only the diacritic pixels by subtracting the unaccented base glyph, then
# reuse that exact Quantico accent on the missing Czech glyphs.
ACCENT_DONORS = {
    "caron": [("ř", "r"), ("š", "s"), ("ž", "z")],
    "ring": [("å", "a"), ("Å", "A")],
}

def best_cmap(path: Path):
    tt = TTFont(str(path), fontNumber=0)
    return tt.getBestCmap() or {}

def validate_charset(cmap):
    missing = []
    synthesized = []
    for ch in CHARS:
        if ord(ch) in cmap:
            continue
        spec = SYNTH.get(ch)
        if spec and ord(spec[0]) in cmap:
            synthesized.append(ch)
            continue
        missing.append(ch)

    if missing:
        raise SystemExit(
            "Quantico-Bold.ttf cannot provide required glyphs: " +
            " ".join(f"{ch}(U+{ord(ch):04X})" for ch in missing)
        )

    print(f"Dashboard KPI charset OK: {len(CHARS)} glyphs")
    print("Synthesized Czech glyphs: " + " ".join(synthesized))

def choose_size(path: Path, target_height: int):
    probe = "H0123456789FVEWŘŠŽÁÉÍÓÚÝ"
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

def render_on_common_canvas(font, text, bounds):
    left, top, right, bottom = bounds
    w = max(1, right - left)
    h = max(1, bottom - top)
    img = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((-left, -top), text, font=font, fill=255, anchor="ls")
    return img.point(lambda p: 255 if p >= 128 else 0)

def extract_accent(font, accented, base):
    ab = font.getbbox(accented, anchor="ls")
    bb = font.getbbox(base, anchor="ls")
    bounds = (
        min(ab[0], bb[0]),
        min(ab[1], bb[1]),
        max(ab[2], bb[2]),
        max(ab[3], bb[3]),
    )

    ai = render_on_common_canvas(font, accented, bounds)
    bi = render_on_common_canvas(font, base, bounds)

    diff = Image.new("L", ai.size, 0)
    ap = ai.load()
    bp = bi.load()
    dp = diff.load()

    for y in range(ai.height):
        for x in range(ai.width):
            if ap[x, y] and not bp[x, y]:
                dp[x, y] = 255

    bbox = diff.getbbox()
    if not bbox:
        raise RuntimeError(f"Could not extract accent from {accented}/{base}")

    accent = diff.crop(bbox)
    # Position in baseline font coordinates.
    accent_left = bounds[0] + bbox[0]
    accent_top = bounds[1] + bbox[1]
    return accent, accent_left, accent_top

def build_accent_templates(font, cmap):
    templates = {}

    for accent_name, donors in ACCENT_DONORS.items():
        for accented, base in donors:
            if ord(accented) in cmap and ord(base) in cmap:
                accent, left, top = extract_accent(font, accented, base)
                base_box = font.getbbox(base, anchor="ls")
                donor_center = (base_box[0] + base_box[2]) / 2.0
                accent_center = left + accent.width / 2.0
                templates[accent_name] = {
                    "img": accent,
                    "top": top,
                    "center_delta": accent_center - donor_center,
                    "base_top": base_box[1],
                }
                break

    # Czech ď/ť use the apostrophe-shaped caron. Quantico does not contain
    # those precomposed glyphs, so use Quantico's own apostrophe artwork rather
    # than drawing a synthetic slash.
    if ord("'") in cmap:
        box = font.getbbox("'", anchor="ls")
        img = render_on_common_canvas(font, "'", box)
        templates["apostrophe"] = {
            "img": img,
            "top": box[1],
            "left": box[0],
        }

    return templates

def render_synth(font, base, accent, target_height, templates):
    left, top, right, bottom = font.getbbox(base, anchor="ls")

    if accent not in templates:
        raise RuntimeError(f"No Quantico accent template available for {accent}")

    base_img = render_on_common_canvas(font, base, (left, top, right, bottom))
    tpl = templates[accent]
    accent_img = tpl["img"]

    if accent in ("caron", "ring"):
        base_center = (left + right) / 2.0
        accent_left = round(
            base_center + tpl.get("center_delta", 0.0) - accent_img.width / 2.0
        )

        # Preserve the vertical relationship of the donor accent to its donor
        # base top. This keeps the accent visually native to Quantico.
        accent_top = round(
            top + (tpl["top"] - tpl["base_top"])
        )
    else:
        # Apostrophe-like Czech caron: place Quantico's own apostrophe just
        # outside the upper-right of d/t, aligned to the cap/x-height area.
        gap = max(1, round(target_height * 0.04))
        accent_left = right + gap
        accent_top = top + max(0, round(target_height * 0.01))

    canvas_left = min(left, accent_left)
    canvas_top = min(top, accent_top)
    canvas_right = max(right, accent_left + accent_img.width)
    canvas_bottom = max(bottom, accent_top + accent_img.height)

    out = Image.new(
        "L",
        (max(1, canvas_right - canvas_left),
         max(1, canvas_bottom - canvas_top)),
        0,
    )
    out.paste(base_img, (left - canvas_left, top - canvas_top))
    out.paste(
        accent_img,
        (accent_left - canvas_left, accent_top - canvas_top),
        accent_img,
    )
    out = out.point(lambda p: 255 if p >= 128 else 0)

    adv = max(1, round(font.getlength(base)))
    if accent == "apostrophe":
        adv = max(adv, canvas_right - left)

    return {
        "img": out,
        "left": canvas_left,
        "top": canvas_top,
        "advance": adv,
    }

def render_direct(font, ch):
    left, top, right, bottom = font.getbbox(ch, anchor="ls")
    w = max(1, right - left)
    h = max(1, bottom - top)
    img = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(img)
    draw.text((-left, -top), ch, font=font, fill=255, anchor="ls")
    img = img.point(lambda p: 255 if p >= 128 else 0)
    return {
        "img": img,
        "left": left,
        "top": top,
        "advance": max(1, round(font.getlength(ch))),
    }

def make_face(target_height: int, cmap):
    name = f"Quantico{target_height}"
    font = choose_size(FONT, target_height)
    templates = build_accent_templates(font, cmap)

    needed_accents = {accent for _, accent in SYNTH.values()}
    missing_templates = sorted(needed_accents - set(templates.keys()))
    if missing_templates:
        raise RuntimeError(
            "Missing Quantico accent donor templates: " +
            ", ".join(missing_templates)
        )

    rendered = []
    common_top = 32767

    for ch in CHARS:
        if ch == " ":
            rec = {
                "code": ord(ch), "img": None, "left": 0, "top": 0,
                "advance": max(1, round(font.getlength(ch)))
            }
        elif ord(ch) in cmap:
            rec = render_direct(font, ch)
            rec["code"] = ord(ch)
        else:
            base, accent = SYNTH[ch]
            rec = render_synth(font, base, accent, target_height, templates)
            rec["code"] = ord(ch)

        if rec["img"] is not None:
            common_top = min(common_top, rec["top"])
        rendered.append(rec)

    glyphs = []
    all_bytes = bytearray()

    for rec in rendered:
        offset = len(all_bytes)
        if rec["img"] is None:
            glyphs.append((rec["code"], 0, 0, rec["advance"], 0, 0, offset))
            continue

        img = rec["img"]
        w, h = img.size
        all_bytes.extend(pack_bitmap(img))
        glyphs.append((
            rec["code"], w, h, rec["advance"],
            rec["left"], rec["top"] - common_top, offset
        ))

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

cmap = best_cmap(FONT)
validate_charset(cmap)

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
