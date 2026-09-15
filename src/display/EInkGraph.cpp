#include "EInkGraph.h"
#include <math.h>

EInkGraph::EInkGraph(IDisplay& display, int16_t x, int16_t y, int16_t w, int16_t h)
    : _display(display), _x(x), _y(y), _w(w), _h(h) {
}

void EInkGraph::setXRange(float minValue, float maxValue) {
    if (maxValue <= minValue) maxValue = minValue + 1.0f;
    _xMin = minValue;
    _xMax = maxValue;
}

void EInkGraph::setYRange(float minValue, float maxValue) {
    if (maxValue <= minValue) maxValue = minValue + 1.0f;
    _yMin = minValue;
    _yMax = maxValue;
}

float EInkGraph::clamp(float value, float minValue, float maxValue) const {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

int16_t EInkGraph::mapX(float value) const {
    const float v = clamp(value, _xMin, _xMax);
    const float normalized = (v - _xMin) / (_xMax - _xMin);
    return _x + static_cast<int16_t>(lroundf(normalized * static_cast<float>(_w - 1)));
}

int16_t EInkGraph::mapY(float value) const {
    const float v = clamp(value, _yMin, _yMax);
    const float normalized = (v - _yMin) / (_yMax - _yMin);
    return _y + (_h - 1) - static_cast<int16_t>(lroundf(normalized * static_cast<float>(_h - 1)));
}

void EInkGraph::drawFrame(uint16_t color) const {
    _display.drawRect(_x, _y, _w, _h, color);
}

void EInkGraph::drawHorizontalGrid(uint8_t divisions, uint16_t color) const {
    if (divisions < 2) return;
    for (uint8_t i = 1; i < divisions; ++i) {
        const int16_t y = _y + static_cast<int16_t>((static_cast<int32_t>(_h - 1) * i) / divisions);
        for (int16_t x = _x; x < _x + _w; x += 6) {
            const int16_t x2 = (x + 2 < _x + _w) ? x + 2 : _x + _w - 1;
            _display.drawLine(x, y, x2, y, color);
        }
    }
}

void EInkGraph::drawVerticalGrid(uint8_t divisions, uint16_t color) const {
    if (divisions < 2) return;
    for (uint8_t i = 1; i < divisions; ++i) {
        const int16_t x = _x + static_cast<int16_t>((static_cast<int32_t>(_w - 1) * i) / divisions);
        for (int16_t y = _y; y < _y + _h; y += 6) {
            const int16_t y2 = (y + 2 < _y + _h) ? y + 2 : _y + _h - 1;
            _display.drawLine(x, y, x, y2, color);
        }
    }
}

void EInkGraph::drawLineSeries(const EInkGraphPoint* points, size_t count,
                               bool drawPoints, uint16_t color) const {
    if (points == nullptr || count == 0) return;

    int16_t previousX = mapX(points[0].x);
    int16_t previousY = mapY(points[0].y);
    if (drawPoints) _display.fillCircle(previousX, previousY, 3, color);

    for (size_t i = 1; i < count; ++i) {
        const int16_t x = mapX(points[i].x);
        const int16_t y = mapY(points[i].y);
        _display.drawLine(previousX, previousY, x, y, color);
        _display.drawLine(previousX, previousY + 1, x, y + 1, color);
        if (drawPoints) _display.fillCircle(x, y, 3, color);
        previousX = x;
        previousY = y;
    }
}

void EInkGraph::drawBars(const EInkGraphPoint* points, size_t count,
                         float baseline, uint16_t color) const {
    if (points == nullptr || count == 0) return;

    const int16_t baseY = mapY(baseline);
    int16_t barWidth = 6;
    if (count > 1) {
        const int16_t dx = abs(mapX(points[1].x) - mapX(points[0].x));
        barWidth = (dx - 3 > 3) ? dx - 3 : 3;
    }

    for (size_t i = 0; i < count; ++i) {
        const int16_t centerX = mapX(points[i].x);
        const int16_t valueY = mapY(points[i].y);
        const int16_t top = (valueY < baseY) ? valueY : baseY;
        const int16_t delta = abs(baseY - valueY);
        const int16_t height = delta > 1 ? delta : 1;
        const int16_t left = centerX - barWidth / 2;

        _display.drawRect(left, top, barWidth, height, color);
        for (int16_t y = top + 2; y < top + height; y += 4) {
            _display.drawLine(left + 1, y, left + barWidth - 2, y, color);
        }
    }
}
