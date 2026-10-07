#!/usr/bin/env python3
"""Generate all 1-bit Home energy-flow PROGMEM assets."""

from pathlib import Path
from PIL import Image, ImageDraw
import math

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "energy-flow" / "house.png"
OUTPUT = ROOT / "src" / "display" / "assets" / "EnergyFlowAssets.h"

HOUSE_WIDTH = 242
THRESHOLD = 175


def source_house() -> Image.Image:
    if not SOURCE.exists():
        raise SystemExit(f"Source image not found: {SOURCE}")
    image = Image.open(SOURCE).convert("RGBA")
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise SystemExit("Source image is fully transparent")
    image = image.crop(bbox)
    white = Image.new("RGBA", image.size, (255, 255, 255, 255))
    gray = Image.alpha_composite(white, image).convert("L")
    height = round(gray.height * HOUSE_WIDTH / gray.width)
    gray = gray.resize((HOUSE_WIDTH, height), Image.Resampling.LANCZOS)
    return gray.point(lambda p: 255 if p >= THRESHOLD else 0, mode="1")


def canvas(width: int, height: int) -> Image.Image:
    return Image.new("1", (width, height), 1)


def pv_icon() -> Image.Image:
    im = canvas(58, 38)
    d = ImageDraw.Draw(im)
    d.ellipse((3, 2, 19, 18), outline=0, width=2)
    for angle in range(0, 360, 45):
        a = math.radians(angle)
        d.line((11 + math.cos(a) * 11, 10 + math.sin(a) * 11,
                11 + math.cos(a) * 15, 10 + math.sin(a) * 15),
               fill=0, width=2)
    panel = [(18, 15), (53, 15), (48, 35), (12, 35)]
    d.line(panel + [panel[0]], fill=0, width=2)
    for t in (1 / 3, 2 / 3):
        d.line((18 + (53 - 18) * t, 15, 12 + (48 - 12) * t, 35), fill=0)
    d.line((15, 25, 50, 25), fill=0)
    return im


def grid_icon() -> Image.Image:
    im = canvas(40, 72)
    d = ImageDraw.Draw(im)
    d.line((20, 2, 5, 68), fill=0, width=3)
    d.line((20, 2, 35, 68), fill=0, width=3)
    d.line((20, 2, 20, 68), fill=0, width=2)
    for y, span in ((18, 13), (31, 17), (45, 21)):
        d.line((20 - span, y, 20 + span, y), fill=0, width=3)
    d.line((5, 68, 35, 68), fill=0, width=3)
    return im


def azrouter_icon() -> Image.Image:
    im = canvas(44, 60)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((4, 3, 39, 55), radius=5, outline=0, width=3)
    for x, top, bottom in ((14, 18, 41), (22, 14, 45), (30, 18, 41)):
        d.line((x, top, x, bottom), fill=0, width=3)
    d.line((10, 58, 34, 58), fill=0, width=2)
    return im


def horizontal_arrow(direction: str) -> Image.Image:
    im = canvas(50, 18)
    d = ImageDraw.Draw(im)
    if direction == "right":
        d.line((2, 9, 37, 9), fill=0, width=4)
        d.polygon(((37, 2), (49, 9), (37, 16)), fill=0)
    else:
        d.line((13, 9, 48, 9), fill=0, width=4)
        d.polygon(((13, 2), (1, 9), (13, 16)), fill=0)
    return im


def down_arrow() -> Image.Image:
    im = canvas(18, 42)
    d = ImageDraw.Draw(im)
    d.line((9, 2, 9, 29), fill=0, width=4)
    d.polygon(((2, 29), (9, 41), (16, 29)), fill=0)
    return im


def bitmap_bytes(image: Image.Image) -> tuple[int, list[int]]:
    width, height = image.size
    row_bytes = (width + 7) // 8
    data: list[int] = []
    for y in range(height):
        for byte_index in range(row_bytes):
            value = 0
            for bit in range(8):
                x = byte_index * 8 + bit
                if x < width and image.getpixel((x, y)) == 0:
                    value |= 1 << (7 - bit)
            data.append(value)
    return row_bytes, data


def emit_asset(name: str, image: Image.Image) -> str:
    width, height = image.size
    row_bytes, data = bitmap_bytes(image)
    rows = []
    for offset in range(0, len(data), 16):
        rows.append("    " + ", ".join(f"0x{v:02X}" for v in data[offset:offset + 16]) + ",")
    return f"""constexpr int16_t {name}Width = {width};
constexpr int16_t {name}Height = {height};
constexpr int16_t {name}RowBytes = {row_bytes};
static const uint8_t {name}Bitmap[] PROGMEM = {{
{chr(10).join(rows)}
}};
"""


def main() -> None:
    assets = [
        ("House", source_house()),
        ("PvIcon", pv_icon()),
        ("GridIcon", grid_icon()),
        ("AzrouterIcon", azrouter_icon()),
        ("ArrowPvDown", down_arrow()),
        ("ArrowGridIn", horizontal_arrow("right")),
        ("ArrowGridOut", horizontal_arrow("left")),
        ("ArrowAzOut", horizontal_arrow("right")),
    ]
    header = "#pragma once\n#include <Arduino.h>\n\nnamespace EnergyFlowAssets {\n\n"
    header += "\n".join(emit_asset(name, image) for name, image in assets)
    header += "\n} // namespace EnergyFlowAssets\n"
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(header, encoding="utf-8")
    print(f"Generated {OUTPUT}")
    for name, image in assets:
        rb, data = bitmap_bytes(image)
        print(f"{name}: {image.width}x{image.height}px, {len(data)} bytes")


if __name__ == "__main__":
    main()
