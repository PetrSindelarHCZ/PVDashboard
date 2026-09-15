#pragma once
#include "IDisplay.h"
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>

class EpaperDisplay : public IDisplay {
public:
    EpaperDisplay(int8_t cs, int8_t dc, int8_t rst, int8_t busy, int8_t sck, int8_t miso, int8_t mosi);
    ~EpaperDisplay() override = default;

    void init() override;
    void clear(uint16_t color = 1) override;
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override;
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) override;
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) override;
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) override;
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) override;
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                    int16_t w, int16_t h, uint16_t color) override;

    void setFont(const GFXfont* f = nullptr) override;
    void setUnicodeFont(const uint8_t* font) override;
    void setTextColor(uint16_t c) override;
    void setTextSize(uint8_t s) override;
    void setCursor(int16_t x, int16_t y) override;
    void print(const String& text) override;
    void printf(const char* format, ...) override;
    int16_t textWidth(const String& text) override;

    int16_t width() const override;
    int16_t height() const override;

    void beginFrame(bool partial = false) override;
    bool nextFrame() override;
    void powerOff() override;

private:
    uint16_t mapColor(uint16_t color) const;

    int8_t _cs, _dc, _rst, _busy, _sck, _miso, _mosi;
    GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> _epd;
    U8G2_FOR_ADAFRUIT_GFX _u8g2;
    bool _useUnicodeFont = false;
    bool _isPartial = false;
};
