#pragma once
#include <Arduino.h>
#include "IDisplay.h"

namespace DashboardSans {

enum class Weight : uint8_t { Regular, Bold };

inline bool isSupportedSize(uint8_t px) {
    switch (px) {
        case 8: case 10: case 12: case 14: case 16: case 18:
        case 20: case 22: case 24: case 28: case 32: case 36:
            return true;
        default: return false;
    }
}

namespace Detail {

struct AccentMap { uint32_t codepoint; char base; uint8_t accent; };

static const AccentMap Accents[] = {
    {0x00E1,'a',1},{0x010D,'c',2},{0x010F,'d',2},{0x00E9,'e',1},
    {0x011B,'e',2},{0x00ED,'i',1},{0x0148,'n',2},{0x00F3,'o',1},
    {0x0159,'r',2},{0x0161,'s',2},{0x0165,'t',2},{0x00FA,'u',1},
    {0x016F,'u',3},{0x00FD,'y',1},{0x017E,'z',2},
    {0x00C1,'A',1},{0x010C,'C',2},{0x010E,'D',2},{0x00C9,'E',1},
    {0x011A,'E',2},{0x00CD,'I',1},{0x0147,'N',2},{0x00D3,'O',1},
    {0x0158,'R',2},{0x0160,'S',2},{0x0164,'T',2},{0x00DA,'U',1},
    {0x016E,'U',3},{0x00DD,'Y',1},{0x017D,'Z',2}
};

static const uint8_t GlyphRows[95][7] = {
    {0,0,0,0,0,0,0},{4,4,4,4,4,0,4},{14,17,1,2,4,0,4},{14,17,1,2,4,0,4},
    {14,17,1,2,4,0,4},{25,26,4,8,22,19,0},{14,17,1,2,4,0,4},{14,17,1,2,4,0,4},
    {2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{14,17,1,2,4,0,4},{0,4,4,31,4,4,0},
    {0,0,0,0,6,6,4},{0,0,0,31,0,0,0},{0,0,0,0,0,6,6},{1,2,2,4,8,8,16},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},{14,17,17,15,1,1,14},{0,6,6,0,6,6,0},{14,17,1,2,4,0,4},
    {14,17,1,2,4,0,4},{0,31,0,31,0,0,0},{14,17,1,2,4,0,4},{14,17,1,2,4,0,4},
    {14,17,1,2,4,0,4},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{15,16,16,16,16,16,15},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{15,16,16,23,17,17,15},
    {17,17,17,31,17,17,17},{31,4,4,4,4,4,31},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},
    {16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},
    {31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},
    {17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,1,2,4,0,4},
    {14,17,1,2,4,0,4},{14,17,1,2,4,0,4},{14,17,1,2,4,0,4},{14,17,1,2,4,0,4},
    {14,17,1,2,4,0,4},{0,0,14,1,15,17,15},{16,16,22,25,17,17,30},{0,0,15,16,16,16,15},
    {1,1,13,19,17,17,15},{0,0,14,17,31,16,15},{6,9,8,28,8,8,8},{0,0,15,17,15,1,14},
    {16,16,22,25,17,17,17},{4,0,12,4,4,4,14},{2,0,6,2,2,18,12},{16,16,18,20,24,20,18},
    {12,4,4,4,4,4,14},{0,0,26,21,21,21,21},{0,0,22,25,17,17,17},{0,0,14,17,17,17,14},
    {0,0,30,17,30,16,16},{0,0,15,17,15,1,1},{0,0,23,24,16,16,16},{0,0,15,16,14,1,30},
    {8,8,28,8,8,9,6},{0,0,17,17,17,19,13},{0,0,17,17,17,10,4},{0,0,17,17,21,21,10},
    {0,0,17,10,4,10,17},{0,0,17,17,15,1,14},{0,0,31,2,4,8,31},{14,17,1,2,4,0,4},
    {4,4,4,4,4,4,4},{14,17,1,2,4,0,4},{14,17,1,2,4,0,4}
};

inline uint32_t decodeUtf8(const String& text, size_t& index) {
    const uint8_t c0 = static_cast<uint8_t>(text[index++]);
    if ((c0 & 0x80u) == 0) return c0;
    if ((c0 & 0xE0u) == 0xC0u && index < text.length()) {
        const uint8_t c1 = static_cast<uint8_t>(text[index++]);
        return ((c0 & 0x1Fu) << 6) | (c1 & 0x3Fu);
    }
    if ((c0 & 0xF0u) == 0xE0u && index + 1 < text.length()) {
        const uint8_t c1 = static_cast<uint8_t>(text[index++]);
        const uint8_t c2 = static_cast<uint8_t>(text[index++]);
        return ((c0 & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
    }
    return '?';
}

inline void resolveGlyph(uint32_t cp, char& base, uint8_t& accent) {
    accent = 0;
    if (cp >= 32 && cp <= 126) { base = static_cast<char>(cp); return; }
    for (const auto& item : Accents) {
        if (item.codepoint == cp) { base = item.base; accent = item.accent; return; }
    }
    base = '?';
}

inline int16_t glyphWidth(uint8_t px) {
    return max<int16_t>(4, static_cast<int16_t>((px * 6 + 5) / 10));
}
inline int16_t advance(uint8_t px) {
    return glyphWidth(px) + max<int16_t>(1, px / 10);
}
inline int16_t sourceYToPixel(int16_t top, uint8_t row, uint8_t px) {
    return top + static_cast<int32_t>(row) * px / 9;
}

inline void drawMasterCell(IDisplay& d, int16_t x, int16_t top,
                           uint8_t col, uint8_t row, uint8_t px,
                           Weight weight, uint16_t color) {
    const int16_t gw = glyphWidth(px);
    const int16_t x0 = x + static_cast<int32_t>(col) * gw / 5;
    const int16_t x1 = x + static_cast<int32_t>(col + 1) * gw / 5;
    const int16_t y0 = sourceYToPixel(top, row, px);
    const int16_t y1 = sourceYToPixel(top, row + 1, px);
    const int16_t w = max<int16_t>(1, x1 - x0);
    const int16_t h = max<int16_t>(1, y1 - y0);
    d.fillRect(x0, y0, w, h, color);
    if (weight == Weight::Bold && px >= 12) d.fillRect(x0 + 1, y0, w, h, color);
}

inline void drawAccent(IDisplay& d, int16_t x, int16_t top, uint8_t px,
                       uint8_t accent, Weight weight, uint16_t color) {
    if (accent == 0) return;
    const int16_t gw = glyphWidth(px);
    auto cell = [&](uint8_t col, uint8_t row) {
        drawMasterCell(d, x, top, col, row, px, weight, color);
    };
    if (accent == 1) { cell(3,0); cell(2,1); }
    else if (accent == 2) { cell(1,0); cell(3,0); cell(2,1); }
    else if (accent == 3) {
        const int16_t cx = x + gw / 2;
        const int16_t r = max<int16_t>(1, px / 12);
        d.drawCircle(cx, top + r + 1, r, color);
    }
}

inline void drawGlyph(IDisplay& d, int16_t x, int16_t top, char base,
                      uint8_t accent, uint8_t px, Weight weight,
                      uint16_t color) {
    uint8_t code = static_cast<uint8_t>(base);
    if (code < 32 || code > 126) code = '?';
    const uint8_t* rows = GlyphRows[code - 32];
    for (uint8_t r = 0; r < 7; ++r) {
        const uint8_t bits = rows[r];
        for (uint8_t c = 0; c < 5; ++c) {
            if ((bits & (1u << (4 - c))) == 0) continue;
            drawMasterCell(d, x, top, c, r + 2, px, weight, color);
        }
    }
    drawAccent(d, x, top, px, accent, weight, color);
}

} // namespace Detail

inline int16_t textWidth(const String& text, uint8_t px) {
    if (!isSupportedSize(px)) return 0;
    int16_t count = 0;
    size_t index = 0;
    while (index < text.length()) { Detail::decodeUtf8(text, index); ++count; }
    return count > 0 ? count * Detail::advance(px) - max<int16_t>(1, px / 10) : 0;
}

inline void drawText(IDisplay& d, int16_t x, int16_t top, const String& text,
                     uint8_t px, Weight weight = Weight::Regular,
                     uint16_t color = 0) {
    if (!isSupportedSize(px)) return;
    int16_t cursorX = x;
    size_t index = 0;
    while (index < text.length()) {
        const uint32_t cp = Detail::decodeUtf8(text, index);
        char base = '?'; uint8_t accent = 0;
        Detail::resolveGlyph(cp, base, accent);
        Detail::drawGlyph(d, cursorX, top, base, accent, px, weight, color);
        cursorX += Detail::advance(px);
    }
}

} // namespace DashboardSans
