#!/usr/bin/env python3
"""Convert the Home energy-flow house PNG to a 1-bit PROGMEM bitmap."""

from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "energy-flow" / "house.png"
OUTPUT = ROOT / "src" / "display" / "assets" / "EnergyFlowAssets.h"

TARGET_WIDTH = 220
THRESHOLD = 175


def main() -> None:
    if not SOURCE.exists():
        raise SystemExit(f"Source image not found: {SOURCE}")

    image = Image.open(SOURCE).convert("RGBA")
    alpha = image.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        raise SystemExit("Source image is fully transparent")

    image = image.crop(bbox)
    white = Image.new("RGBA", image.size, (255, 255, 255, 255))
    gray = Image.alpha_composite(white, image).convert("L")

    target_height = round(gray.height * TARGET_WIDTH / gray.width)
    gray = gray.resize((TARGET_WIDTH, target_height), Image.Resampling.LANCZOS)
    bitmap = gray.point(lambda p: 255 if p >= THRESHOLD else 0, mode="1")

    row_bytes = (TARGET_WIDTH + 7) // 8
    data: list[int] = []
    for y in range(target_height):
        byte = 0
        bits = 0
        for x in range(TARGET_WIDTH):
            # Adafruit/GxEPD drawBitmap uses a set bit for foreground pixels.
            bit = 1 if bitmap.getpixel((x, y)) == 0 else 0
            byte = (byte << 1) | bit
            bits += 1
            if bits == 8:
                data.append(byte)
                byte = 0
                bits = 0
        if bits:
            byte <<= 8 - bits
            data.append(byte)

    rows = []
    for offset in range(0, len(data), 16):
        chunk = data[offset:offset + 16]
        rows.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",")

    header = f"""#pragma once
#include <Arduino.h>

namespace EnergyFlowAssets {{

constexpr int16_t HouseWidth = {TARGET_WIDTH};
constexpr int16_t HouseHeight = {target_height};
constexpr int16_t HouseRowBytes = {row_bytes};

static const uint8_t HouseBitmap[] PROGMEM = {{
{chr(10).join(rows)}
}};

}} // namespace EnergyFlowAssets
"""

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(header, encoding="utf-8")
    print(f"Generated {OUTPUT}")
    print(
        f"{TARGET_WIDTH}x{target_height}px, 1-bit, "
        f"{len(data)} bytes ({row_bytes} bytes/row)"
    )


if __name__ == "__main__":
    main()
