#include "DisplayPreview.h"
#include <ArduinoJson.h>
#include <new>
#include <stdarg.h>

DisplayPreview::~DisplayPreview() {
    delete _canvas;
    _canvas = nullptr;
    if (_mutex != nullptr) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

void DisplayPreview::init() {
    if (_initialized) return;
    _initialized = true;

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        Serial.println("[DISPLAY-PREVIEW] Nelze vytvorit mutex.");
        return;
    }

    _canvas = new (std::nothrow) GFXcanvas1(Width, Height);
    if (_canvas == nullptr || _canvas->getBuffer() == nullptr) {
        Serial.printf("[DISPLAY-PREVIEW] Nelze alokovat %u B framebuffer. Nahled bude vypnuty.\n",
                      static_cast<unsigned int>(BitmapBytes));
        delete _canvas;
        _canvas = nullptr;
        return;
    }

    _u8g2.begin(*_canvas);
    _u8g2.setFontMode(1);
    _u8g2.setFontDirection(0);
    _u8g2.setForegroundColor(0);
    _u8g2.setBackgroundColor(1);
    _canvas->fillScreen(1);

    Serial.printf("[DISPLAY-PREVIEW] Framebuffer pripraven: %dx%d, %u B.\n",
                  Width, Height, static_cast<unsigned int>(BitmapBytes));
}

bool DisplayPreview::available() const {
    return _canvas != nullptr && _canvas->getBuffer() != nullptr && _mutex != nullptr;
}

bool DisplayPreview::ready() {
    if (!available()) return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
    const bool result = _hasCapture;
    xSemaphoreGive(_mutex);
    return result;
}

void DisplayPreview::capture(IScreen& screen, const DataModel& dataModel, bool fullRefresh) {
    if (!available()) return;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        Serial.println("[DISPLAY-PREVIEW] Timeout pri publikovani nahledu.");
        return;
    }

    _canvas->fillScreen(1);
    _useUnicodeFont = false;
    screen.render(*this, dataModel);

    _lastScreenId = screen.getId();
    _lastRefreshMs = millis();
    _lastFullRefresh = fullRefresh;
    _lastWifiRssi = dataModel.system.wifiRssi;
    _lastWifiSignalLevel = dataModel.system.wifiSignalLevel;
    _lastWifiConnected = dataModel.system.wifiConnected;
    _lastWifiAccessPoint = dataModel.system.wifiAccessPoint;
    _lastNtpSynced = dataModel.system.ntpSynced;
    _lastGoodweAvailable = dataModel.solar.status.available;
    _lastAzrouterAvailable = dataModel.azrouter.status.available;
    _lastRefreshTime = dataModel.system.ntpSynced
        ? dataModel.system.dateStr + " " + dataModel.system.timeStr
        : String();
    _hasCapture = true;
    ++_generation;

    xSemaphoreGive(_mutex);
    Serial.printf("[DISPLAY-PREVIEW] Publikovan nahled '%s', generace %lu.\n",
                  _lastScreenId.c_str(), static_cast<unsigned long>(_generation));
}

String DisplayPreview::metadataJson() {
    JsonDocument doc;

    if (!available()) {
        doc["ready"] = false;
        doc["reason"] = "framebuffer_unavailable";
        String response;
        serializeJson(doc, response);
        return response;
    }

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(250)) != pdTRUE) {
        doc["ready"] = false;
        doc["reason"] = "busy";
        String response;
        serializeJson(doc, response);
        return response;
    }

    doc["ready"] = _hasCapture;
    doc["width"] = Width;
    doc["height"] = Height;
    doc["generation"] = _generation;
    doc["page"] = _lastScreenId;
    doc["lastRefreshMs"] = _lastRefreshMs;
    doc["lastRefresh"] = _lastRefreshTime;
    doc["refreshType"] = _lastFullRefresh ? "full" : "partial";
    doc["wifiRssi"] = _lastWifiRssi;
    doc["wifiSignalLevel"] = _lastWifiSignalLevel;
    doc["wifiConnected"] = _lastWifiConnected;
    doc["wifiAccessPoint"] = _lastWifiAccessPoint;
    doc["ntpSynced"] = _lastNtpSynced;
    doc["goodweAvailable"] = _lastGoodweAvailable;
    doc["azrouterAvailable"] = _lastAzrouterAvailable;

    String response;
    serializeJson(doc, response);
    xSemaphoreGive(_mutex);
    return response;
}

void DisplayPreview::buildBmpHeader(uint8_t* header) const {
    memset(header, 0, BmpHeaderBytes);

    auto put16 = [header](size_t offset, uint16_t value) {
        header[offset] = static_cast<uint8_t>(value & 0xFF);
        header[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    };
    auto put32 = [header](size_t offset, uint32_t value) {
        header[offset] = static_cast<uint8_t>(value & 0xFF);
        header[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        header[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        header[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    };

    header[0] = 'B';
    header[1] = 'M';
    put32(2, static_cast<uint32_t>(BmpBytes));
    put32(10, static_cast<uint32_t>(BmpHeaderBytes));

    put32(14, 40);
    put32(18, static_cast<uint32_t>(Width));
    put32(22, static_cast<uint32_t>(-Height)); // top-down bitmap
    put16(26, 1);
    put16(28, 1);
    put32(34, static_cast<uint32_t>(BitmapBytes));
    put32(38, 2835);
    put32(42, 2835);
    put32(46, 2);
    put32(50, 2);

    // 1-bit palette: index 0 = black, index 1 = white (BGRA)
    header[54] = 0;   header[55] = 0;   header[56] = 0;   header[57] = 0;
    header[58] = 255; header[59] = 255; header[60] = 255; header[61] = 0;
}

bool DisplayPreview::writeBmp(WiFiClient& client) {
    if (!available() || !_hasCapture) return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return false;

    uint8_t header[BmpHeaderBytes];
    buildBmpHeader(header);

    bool ok = client.write(header, sizeof(header)) == sizeof(header);
    const uint8_t* buffer = _canvas->getBuffer();

    for (int16_t y = 0; ok && y < Height; ++y) {
        const uint8_t* row = buffer + static_cast<size_t>(y) * RowBytes;
        ok = client.write(row, RowBytes) == RowBytes;
    }

    xSemaphoreGive(_mutex);
    return ok;
}

void DisplayPreview::clear(uint16_t color) {
    if (_canvas) _canvas->fillScreen(normalizeColor(color));
}

void DisplayPreview::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (_canvas) _canvas->drawPixel(x, y, normalizeColor(color));
}

void DisplayPreview::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (_canvas) _canvas->drawLine(x0, y0, x1, y1, normalizeColor(color));
}

void DisplayPreview::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (_canvas) _canvas->drawRect(x, y, w, h, normalizeColor(color));
}

void DisplayPreview::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (_canvas) _canvas->fillRect(x, y, w, h, normalizeColor(color));
}

void DisplayPreview::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (_canvas) _canvas->drawRoundRect(x, y, w, h, r, normalizeColor(color));
}

void DisplayPreview::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (_canvas) _canvas->fillRoundRect(x, y, w, h, r, normalizeColor(color));
}

void DisplayPreview::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (_canvas) _canvas->drawCircle(x0, y0, r, normalizeColor(color));
}

void DisplayPreview::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (_canvas) _canvas->fillCircle(x0, y0, r, normalizeColor(color));
}

void DisplayPreview::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                                int16_t w, int16_t h, uint16_t color) {
    if (_canvas) _canvas->drawBitmap(x, y, bitmap, w, h, normalizeColor(color));
}

void DisplayPreview::drawInvertedBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                                        int16_t w, int16_t h, uint16_t color) {
    if (!_canvas || bitmap == nullptr) return;
    const int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;
    for (int16_t j = 0; j < h; ++j) {
        for (int16_t i = 0; i < w; ++i) {
            if (i & 7) byte <<= 1;
            else byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if (!(byte & 0x80)) _canvas->drawPixel(x + i, y + j, normalizeColor(color));
        }
    }
}

void DisplayPreview::setFont(const GFXfont* f) {
    _useUnicodeFont = false;
    if (_canvas) _canvas->setFont(f);
}

void DisplayPreview::setUnicodeFont(const uint8_t* font) {
    _useUnicodeFont = true;
    _u8g2.setFont(font);
    _u8g2.setFontMode(1);
}

void DisplayPreview::setTextColor(uint16_t c) {
    const uint16_t color = normalizeColor(c);
    if (_canvas) _canvas->setTextColor(color);
    _u8g2.setForegroundColor(color);
}

void DisplayPreview::setTextSize(uint8_t s) {
    if (_canvas) _canvas->setTextSize(s);
}

void DisplayPreview::setCursor(int16_t x, int16_t y) {
    if (_useUnicodeFont) _u8g2.setCursor(x, y);
    else if (_canvas) _canvas->setCursor(x, y);
}

void DisplayPreview::print(const String& text) {
    if (_useUnicodeFont) _u8g2.print(text);
    else if (_canvas) _canvas->print(text);
}

void DisplayPreview::printf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (_useUnicodeFont) _u8g2.print(buf);
    else if (_canvas) _canvas->print(buf);
}

int16_t DisplayPreview::textWidth(const String& text) {
    if (_useUnicodeFont) return _u8g2.getUTF8Width(text.c_str());
    if (!_canvas) return 0;

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    _canvas->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return static_cast<int16_t>(w);
}

void DisplayPreview::beginFrame(bool) {
}

bool DisplayPreview::nextFrame() {
    return false;
}

void DisplayPreview::powerOff() {
}
