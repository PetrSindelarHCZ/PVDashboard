#include "EpaperDisplay.h"
#include <stdarg.h>

EpaperDisplay::EpaperDisplay(int8_t cs, int8_t dc, int8_t rst, int8_t busy, int8_t sck, int8_t miso, int8_t mosi)
    : _cs(cs), _dc(dc), _rst(rst), _busy(busy), _sck(sck), _miso(miso), _mosi(mosi),
      _epd(GxEPD2_750_T7(cs, dc, rst, busy)) {
}

void EpaperDisplay::init() {
    Serial.printf("[DISPLAY] Piny CS=%d DC=%d RST=%d BUSY=%d SPI=%d/%d/%d\n",
                  _cs, _dc, _rst, _busy, _sck, _miso, _mosi);
    SPI.begin(_sck, _miso, _mosi, _cs);
    _epd.init(115200);
    _epd.setRotation(0);
    Serial.printf("[DISPLAY] Stav BUSY po init: %d\n", digitalRead(_busy));
}

void EpaperDisplay::clear(uint16_t color) {
    _epd.fillScreen(color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
    _epd.drawPixel(x, y, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _epd.drawLine(x0, y0, x1, y1, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _epd.drawRect(x, y, w, h, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _epd.fillRect(x, y, w, h, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    _epd.drawRoundRect(x, y, w, h, r, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    _epd.fillRoundRect(x, y, w, h, r, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _epd.drawCircle(x0, y0, r, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _epd.fillCircle(x0, y0, r, color == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::setFont(const GFXfont* f) {
    _epd.setFont(f);
}

void EpaperDisplay::setTextColor(uint16_t c) {
    _epd.setTextColor(c == 0 ? GxEPD_BLACK : GxEPD_WHITE);
}

void EpaperDisplay::setTextSize(uint8_t s) {
    _epd.setTextSize(s);
}

void EpaperDisplay::setCursor(int16_t x, int16_t y) {
    _epd.setCursor(x, y);
}

void EpaperDisplay::print(const String& text) {
    _epd.print(text);
}

void EpaperDisplay::printf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    _epd.print(buf);
}

int16_t EpaperDisplay::width() const {
    return _epd.width();
}

int16_t EpaperDisplay::height() const {
    return _epd.height();
}

void EpaperDisplay::beginFrame(bool partial) {
    _isPartial = partial;
    if (partial) {
        _epd.setPartialWindow(0, 0, _epd.width(), _epd.height());
    } else {
        _epd.setFullWindow();
    }
    _epd.firstPage();
}

bool EpaperDisplay::nextFrame() {
    return _epd.nextPage();
}

void EpaperDisplay::powerOff() {
    _epd.powerOff();
}
