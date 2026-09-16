#pragma once

#include "IDisplay.h"
#include <stddef.h>

struct EInkGraphPoint {
    float x = 0.0f;
    float y = 0.0f;
};

class EInkGraph {
public:
    EInkGraph(IDisplay& display, int16_t x, int16_t y, int16_t w, int16_t h);

    void setXRange(float minValue, float maxValue);
    void setYRange(float minValue, float maxValue);

    int16_t mapX(float value) const;
    int16_t mapY(float value) const;

    void drawFrame(uint16_t color = 0) const;
    void drawHorizontalGrid(uint8_t divisions, uint16_t color = 0) const;
    void drawVerticalGrid(uint8_t divisions, uint16_t color = 0) const;
    void drawLineSeries(const EInkGraphPoint* points, size_t count,
                        bool drawPoints = true, uint16_t color = 0) const;
    void drawBars(const EInkGraphPoint* points, size_t count,
                  float baseline = 0.0f, uint16_t color = 0) const;

private:
    float clamp(float value, float minValue, float maxValue) const;

    IDisplay& _display;
    int16_t _x;
    int16_t _y;
    int16_t _w;
    int16_t _h;
    float _xMin = 0.0f;
    float _xMax = 1.0f;
    float _yMin = 0.0f;
    float _yMax = 1.0f;
};
