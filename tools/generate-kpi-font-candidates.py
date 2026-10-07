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

def stroke(draw, points, width):
    draw.line(points, fill=255, width=max(1, width), joint="curve")

def render_synth(font, base, accent, target_height):
    left, top, right, bottom = font.getbbox(base, anchor="ls")
    base_w = max(1, right - left)
    base_h = max(1, bottom - top)

    accent_w = max(5, round(target_height * 0.26))
    accent_h = max(3, round(target_height * 0.15))
    gap = max(1, round(target_height * 0.05))
    thick = max(1, round(target_height * 0.07))

    canvas_left = left
    canvas_right = right
    canvas_top = top
    canvas_bottom = bottom

    if accent in ("caron", "ring"):
        center = (left + right) // 2
        canvas_top = min(canvas_top, top - accent_h - gap)
        canvas_left = min(canvas_left, center - accent_w // 2 - thick)
        canvas_right = max(canvas_right, center + accent_w // 2 + thick)
    elif accent == "apostrophe":
        # Czech ď/ť use an apostrophe-like caron at the upper-right.
        canvas_right = max(canvas_right, right + accent_w // 2 + thick)
        canvas_top = min(canvas_top, top - accent_h // 3)

    w = max(1, canvas_right - canvas_left)
    h = max(1, canvas_bottom - canvas_top)
    img = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(img)

    # Baseline is y=0 in font coordinates, translated to canvas space.
    draw.text((-canvas_left, -canvas_top), base, font=font, fill=255, anchor="ls")

    if accent == "caron":
        center = (left + right) // 2 - canvas_left
        y0 = top - accent_h - gap - canvas_top
        y1 = top - gap - canvas_top
        stroke(draw, [(center - accent_w // 2, y0),
                      (center, y1),
                      (center + accent_w // 2, y0)], thick)
    elif accent == "ring":
        center = (left + right) // 2 - canvas_left
        y0 = top - accent_h - gap - canvas_top
        x0 = center - accent_w // 3
        x1 = center + accent_w // 3
        y1 = y0 + accent_h
        draw.ellipse((x0, y0, x1, y1), outline=255, width=thick)
    elif accent == "apostrophe":
        x0 = right - canvas_left + max(1, thick // 2)
        y0 = top - canvas_top
        stroke(draw, [(x0 + accent_w // 3, y0),
                      (x0, y0 + accent_h)], thick)

    img = img.point(lambda p: 255 if p >= 128 else 0)
    adv = max(1, round(font.getlength(base)))
    if accent == "apostrophe":
        adv = max(adv, canvas_right - left)
    return {
        "img": img,
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
            rec = render_synth(font, base, accent, target_height)
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
