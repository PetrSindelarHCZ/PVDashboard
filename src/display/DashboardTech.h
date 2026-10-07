#pragma once

#include <Arduino.h>
#include "IDisplay.h"

namespace DashboardTech {

// Compact geometric KPI font designed specifically for the 800x480 1-bit panel.
// It is intentionally not a general text font. The supported character set is
// focused on power/energy/temperature style values and short technical labels.
// Native target sizes: 24, 28, 32 and 36 px.

enum Segment : uint8_t {
    SegA = 1u << 0,
    SegB = 1u << 1,
    SegC = 1u << 2,
    SegD = 1u << 3,
    SegE = 1u << 4,
    SegF = 1u << 5,
    SegG = 1u << 6
};

inline bool supportedSize(uint8_t px) {
    return px == 24 || px == 28 || px == 32 || px == 36;
}

inline int16_t digitWidth(uint8_t px) {
    return max<int16_t>(13, static_cast<int16_t>((px * 17 + 12) / 24));
}

inline int16_t strokeWidth(uint8_t px) {
    return max<int16_t>(3, static_cast<int16_t>((px + 5) / 6));
}

inline int16_t spacing(uint8_t px) {
    return max<int16_t>(2, static_cast<int16_t>((px + 11) / 12));
}

inline uint8_t digitMask(char ch) {
    switch (ch) {
        case '0': return SegA | SegB | SegC | SegD | SegE | SegF;
        case '1': return SegB | SegC;
        case '2': return SegA | SegB | SegG | SegE | SegD;
        case '3': return SegA | SegB | SegG | SegC | SegD;
        case '4': return SegF | SegG | SegB | SegC;
        case '5': return SegA | SegF | SegG | SegC | SegD;
        case '6': return SegA | SegF | SegG | SegE | SegC | SegD;
        case '7': return SegA | SegB | SegC;
        case '8': return SegA | SegB | SegC | SegD | SegE | SegF | SegG;
        case '9': return SegA | SegB | SegC | SegD | SegF | SegG;
        default: return 0;
    }
}

inline void drawHSegment(
    IDisplay& d,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t t,
    uint16_t color) {

    const int16_t cut = max<int16_t>(1, t / 2);
    for (int16_t yy = 0; yy < t; ++yy) {
        const int16_t edge = min<int16_t>(yy, t - 1 - yy);
        const int16_t inset = max<int16_t>(0, cut - edge);
        const int16_t rowW = w - inset * 2;
        if (rowW > 0) d.fillRect(x + inset, y + yy, rowW, 1, color);
    }
}

inline void drawVSegment(
    IDisplay& d,
    int16_t x,
    int16_t y,
    int16_t h,
    int16_t t,
    uint16_t color) {

    const int16_t cut = max<int16_t>(1, t / 2);
    for (int16_t yy = 0; yy < h; ++yy) {
        const int16_t edge = min<int16_t>(yy, h - 1 - yy);
        const int16_t inset = max<int16_t>(0, cut - edge);
        const int16_t rowW = t - inset;
        if (rowW > 0) d.fillRect(x + inset / 2, y + yy, rowW, 1, color);
    }
}

inline void drawDiag(
    IDisplay& d,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    int16_t t,
    uint16_t color) {

    const int16_t half = max<int16_t>(1, t / 2);
    for (int16_t o = -half; o <= half; ++o) {
        d.drawLine(x0 + o, y0, x1 + o, y1, color);
    }
}

inline void drawDigit(
    IDisplay& d,
    int16_t x,
    int16_t top,
    char ch,
    uint8_t px,
    uint16_t color) {

    const int16_t w = digitWidth(px);
    const int16_t t = strokeWidth(px);
    const int16_t halfH = px / 2;
    const int16_t innerH = max<int16_t>(4, halfH - t / 2);

    const uint8_t mask = digitMask(ch);
    if (mask & SegA) drawHSegment(d, x, top, w, t, color);
    if (mask & SegG) drawHSegment(d, x, top + halfH - t / 2, w, t, color);
    if (mask & SegD) drawHSegment(d, x, top + px - t, w, t, color);

    if (mask & SegF) drawVSegment(d, x, top + t / 2, innerH, t, color);
    if (mask & SegB) drawVSegment(d, x + w - t, top + t / 2, innerH, t, color);
    if (mask & SegE) drawVSegment(d, x, top + halfH, innerH, t, color);
    if (mask & SegC) drawVSegment(d, x + w - t, top + halfH, innerH, t, color);
}

inline int16_t glyphWidth(char ch, uint8_t px) {
    const int16_t w = digitWidth(px);
    const int16_t t = strokeWidth(px);
    if (ch >= '0' && ch <= '9') return w;
    switch (ch) {
        case ' ': return max<int16_t>(5, px / 4);
        case '.':
        case ',':
        case ':': return t + 2;
        case '-': return max<int16_t>(9, (w * 3) / 4);
        case '+': return w;
        case '/': return max<int16_t>(10, (w * 3) / 4);
        case '%': return w + t + 2;
        case 'W': return w + t + 3;
        case 'V':
        case 'A':
        case 'D':
        case 'C': return w + 2;
        case 'F':
        case 'E':
        case 'k':
        case 'h':
        case 'n':
        case 's':
        case 'e': return w;
        default: return w;
    }
}

inline void drawLetter(
    IDisplay& d,
    int16_t x,
    int16_t top,
    char ch,
    uint8_t px,
    uint16_t color) {

    const int16_t w = digitWidth(px);
    const int16_t t = strokeWidth(px);
    const int16_t mid = top + px / 2;
    const int16_t bottom = top + px - 1;
    const int16_t right = x + glyphWidth(ch, px) - 1;

    switch (ch) {
        case 'F':
            drawVSegment(d, x, top, px, t, color);
            drawHSegment(d, x, top, w, t, color);
            drawHSegment(d, x, mid - t / 2, w - t / 2, t, color);
            break;
        case 'E':
            drawVSegment(d, x, top, px, t, color);
            drawHSegment(d, x, top, w, t, color);
            drawHSegment(d, x, mid - t / 2, w - t / 3, t, color);
            drawHSegment(d, x, top + px - t, w, t, color);
            break;
        case 'V':
            drawDiag(d, x + t / 2, top, x + (right - x) / 2, bottom, t, color);
            drawDiag(d, right - t / 2, top, x + (right - x) / 2, bottom, t, color);
            break;
        case 'W': {
            const int16_t q1 = x + (right - x) / 4;
            const int16_t q3 = x + ((right - x) * 3) / 4;
            drawDiag(d, x + t / 2, top, q1, bottom, t, color);
            drawDiag(d, q1, bottom, x + (right - x) / 2, mid + t, t, color);
            drawDiag(d, x + (right - x) / 2, mid + t, q3, bottom, t, color);
            drawDiag(d, q3, bottom, right - t / 2, top, t, color);
            break;
        }
        case 'k':
            drawVSegment(d, x, top, px, t, color);
            drawDiag(d, x + t / 2, mid, right, top + px / 4, t, color);
            drawDiag(d, x + t / 2, mid, right, bottom, t, color);
            break;
        case 'h':
            drawVSegment(d, x, top, px, t, color);
            drawHSegment(d, x, mid - t / 2, w, t, color);
            drawVSegment(d, x + w - t, mid - t / 2, px / 2 + t / 2, t, color);
            break;
        case 'A':
            drawDiag(d, x + t / 2, bottom, x + w / 2, top, t, color);
            drawDiag(d, x + w / 2, top, right - t / 2, bottom, t, color);
            drawHSegment(d, x + t, mid, max<int16_t>(4, w - 2 * t), t, color);
            break;
        case 'D':
            drawVSegment(d, x, top, px, t, color);
            drawHSegment(d, x, top, w - t / 2, t, color);
            drawHSegment(d, x, top + px - t, w - t / 2, t, color);
            drawVSegment(d, right - t + 1, top + t / 2, px - t, t, color);
            break;
        case 'C':
            drawVSegment(d, x, top + t / 2, px - t, t, color);
            drawHSegment(d, x, top, w, t, color);
            drawHSegment(d, x, top + px - t, w, t, color);
            break;
        case 'n':
            drawVSegment(d, x, mid - t / 2, px / 2 + t / 2, t, color);
            drawHSegment(d, x, mid - t / 2, w, t, color);
            drawVSegment(d, x + w - t, mid - t / 2, px / 2 + t / 2, t, color);
            break;
        case 's':
            drawDigit(d, x, top, '5', px, color);
            break;
        case 'e':
            drawDigit(d, x, top, '6', px, color);
            break;
        default:
            break;
    }
}

inline void drawSymbol(
    IDisplay& d,
    int16_t x,
    int16_t top,
    char ch,
    uint8_t px,
    uint16_t color) {

    const int16_t w = glyphWidth(ch, px);
    const int16_t t = strokeWidth(px);
    const int16_t mid = top + px / 2;

    switch (ch) {
        case '.':
            d.fillRect(x, top + px - t, t, t, color);
            break;
        case ',':
            d.fillRect(x, top + px - t - 1, t, t, color);
            d.drawLine(x + t / 2, top + px - 2, x, top + px + 2, color);
            break;
        case ':':
            d.fillRect(x, top + px / 3 - t / 2, t, t, color);
            d.fillRect(x, top + (px * 2) / 3 - t / 2, t, t, color);
            break;
        case '-':
            drawHSegment(d, x, mid - t / 2, w, t, color);
            break;
        case '+':
            drawHSegment(d, x, mid - t / 2, w, t, color);
            drawVSegment(d, x + w / 2 - t / 2, top + px / 4, px / 2, t, color);
            break;
        case '/':
            drawDiag(d, x, top + px - 1, x + w - 1, top, t, color);
            break;
        case '%': {
            const int16_t dot = max<int16_t>(3, t);
            d.fillRect(x, top + 2, dot, dot, color);
            d.fillRect(x + w - dot, top + px - dot - 2, dot, dot, color);
            drawDiag(d, x + dot, top + px - 2, x + w - dot, top + 2, max<int16_t>(2, t / 2), color);
            break;
        }
        default:
            break;
    }
}

inline bool isLetter(char ch) {
    switch (ch) {
        case 'F': case 'E': case 'V': case 'W':
        case 'A': case 'D': case 'C':
        case 'k': case 'h': case 'n': case 's': case 'e':
            return true;
        default:
            return false;
    }
}

inline bool isSymbol(char ch) {
    switch (ch) {
        case '.': case ',': case ':': case '-': case '+': case '/': case '%':
            return true;
        default:
            return false;
    }
}

inline int16_t textWidth(const String& text, uint8_t px) {
    if (!supportedSize(px)) return 0;
    int16_t width = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        width += glyphWidth(text[i], px);
        if (i + 1 < text.length()) width += spacing(px);
    }
    return width;
}

inline void drawText(
    IDisplay& d,
    int16_t x,
    int16_t top,
    const String& text,
    uint8_t px,
    uint16_t color = 0) {

    if (!supportedSize(px)) return;

    for (size_t i = 0; i < text.length(); ++i) {
        const char ch = text[i];
        if (ch >= '0' && ch <= '9') {
            drawDigit(d, x, top, ch, px, color);
        } else if (isLetter(ch)) {
            drawLetter(d, x, top, ch, px, color);
        } else if (isSymbol(ch)) {
            drawSymbol(d, x, top, ch, px, color);
        }

        x += glyphWidth(ch, px);
        if (i + 1 < text.length()) x += spacing(px);
    }
}

} // namespace DashboardTech
