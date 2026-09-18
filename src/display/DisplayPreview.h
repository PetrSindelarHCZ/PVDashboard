#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WiFiClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "IDisplay.h"
#include "../data/DataModel.h"
#include "../screens/IScreen.h"

class DisplayPreview : public IDisplay {
public:
    static constexpr int16_t Width = 800;
    static constexpr int16_t Height = 480;
    static constexpr size_t RowBytes = Width / 8;
    static constexpr size_t BitmapBytes = RowBytes * Height;
    static constexpr int16_t TileHeight = 60;
    static constexpr size_t TileBytes = RowBytes * TileHeight;
    static constexpr size_t BmpHeaderBytes = 62;
    static constexpr size_t BmpBytes = BmpHeaderBytes + BitmapBytes;

    // Preview nesmí kvůli TLS trvale držet celý 48 kB framebuffer.
    // Pokud by se konkrétní snímek nezkomprimoval pod tento limit,
    // preview pro daný frame raději nebude dostupné.
    static constexpr size_t MaxStoredBytes = 32768;

    DisplayPreview() = default;
    ~DisplayPreview() override;

    void init() override;
    bool available() const;
    bool ready();
    void capture(IScreen& screen, const DataModel& dataModel, bool fullRefresh);
    String metadataJson();
    bool writeBmp(WiFiClient& client);
    size_t bmpSize() const { return BmpBytes; }

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
    void drawInvertedBitmap(int16_t x, int16_t y, const uint8_t* bitmap,
                            int16_t w, int16_t h, uint16_t color) override;

    void setFont(const GFXfont* f = nullptr) override;
    void setUnicodeFont(const uint8_t* font) override;
    void setTextColor(uint16_t c) override;
    void setTextSize(uint8_t s) override;
    void setCursor(int16_t x, int16_t y) override;
    void print(const String& text) override;
    void printf(const char* format, ...) override;
    int16_t textWidth(const String& text) override;

    int16_t width() const override { return Width; }
    int16_t height() const override { return Height; }

    void beginFrame(bool partial = false) override;
    bool nextFrame() override;
    void powerOff() override;

private:
    static uint16_t normalizeColor(uint16_t color) { return color == 0 ? 0 : 1; }
    void buildBmpHeader(uint8_t* header) const;

    bool ensureCanvas();
    void releaseCanvas();
    static size_t packedSize(const uint8_t* input, size_t length);
    static bool pack(const uint8_t* input, size_t length, uint8_t* output, size_t outputSize);
    bool writeUnpacked(WiFiClient& client) const;

    // Preview renderujeme po vodorovnych pruzich, aby nikdy nebyl potreba
    // souvisly 48kB framebuffer. 800 x 60 px = pouze 6000 B.
    GFXcanvas1* _canvas = nullptr;
    int16_t _tileY = 0;
    U8G2_FOR_ADAFRUIT_GFX _u8g2;
    SemaphoreHandle_t _mutex = nullptr;

    uint8_t* _packed = nullptr;
    size_t _packedBytes = 0;

    bool _useUnicodeFont = false;
    bool _initialized = false;
    bool _hasCapture = false;

    uint32_t _generation = 0;
    uint32_t _lastRefreshMs = 0;
    bool _lastFullRefresh = false;
    String _lastScreenId;
    String _lastRefreshTime;
    int32_t _lastWifiRssi = 0;
    uint8_t _lastWifiSignalLevel = 0;
    bool _lastWifiConnected = false;
    bool _lastWifiAccessPoint = false;
    bool _lastNtpSynced = false;
    bool _lastGoodweAvailable = false;
    bool _lastAzrouterAvailable = false;
};
