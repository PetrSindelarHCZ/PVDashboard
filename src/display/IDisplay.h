#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

class IDisplay {
public:
    virtual ~IDisplay() = default;

    virtual void init() = 0;
    virtual void clear(uint16_t color = 1) = 0;
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) = 0;
    virtual void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) = 0;
    virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) = 0;
    virtual void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) = 0;
    
    virtual void setFont(const GFXfont* f = nullptr) = 0;
    virtual void setTextColor(uint16_t c) = 0;
    virtual void setTextSize(uint8_t s) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void print(const String& text) = 0;
    virtual void printf(const char* format, ...) = 0;

    virtual int16_t width() const = 0;
    virtual int16_t height() const = 0;

    virtual void beginFrame(bool partial = false) = 0;
    virtual bool nextFrame() = 0;
    virtual void powerOff() = 0;
};
