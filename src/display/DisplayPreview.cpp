#include "DisplayPreview.h"
#include <ArduinoJson.h>
#include <new>
#include <stdarg.h>
#include <string.h>

DisplayPreview::~DisplayPreview() {
    releaseCanvas();
    delete[] _packed;
    _packed = nullptr;
    _packedBytes = 0;

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

    Serial.printf(
        "[DISPLAY-PREVIEW] Lazy preview pripraven: canvas %u B se alokuje jen pri capture.\n",
        static_cast<unsigned>(BitmapBytes));
}

bool DisplayPreview::available() const {
    return _mutex != nullptr;
}

bool DisplayPreview::ready() {
    if (!available()) return false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;

    const bool result =
        _hasCapture &&
        _packed != nullptr &&
        _packedBytes > 0;

    xSemaphoreGive(_mutex);
    return result;
}

bool DisplayPreview::ensureCanvas() {
    if (_canvas != nullptr && _canvas->getBuffer() != nullptr) return true;

    releaseCanvas();

    _canvas = new (std::nothrow) GFXcanvas1(Width, Height);
    if (_canvas == nullptr || _canvas->getBuffer() == nullptr) {
        Serial.printf(
            "[DISPLAY-PREVIEW] Nelze docasne alokovat %u B canvas. free=%u maxBlock=%u\n",
            static_cast<unsigned>(BitmapBytes),
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());
        releaseCanvas();
        return false;
    }

    _u8g2.begin(*_canvas);
    _u8g2.setFontMode(1);
    _u8g2.setFontDirection(0);
    _u8g2.setForegroundColor(0);
    _u8g2.setBackgroundColor(1);
    return true;
}

void DisplayPreview::releaseCanvas() {
    delete _canvas;
    _canvas = nullptr;
}

size_t DisplayPreview::packedSize(
    const uint8_t* input,
    size_t length) {
    if (input == nullptr || length == 0) return 0;

    size_t encoded = 0;
    size_t i = 0;

    while (i < length) {
        size_t run = 1;
        while (i + run < length &&
               input[i + run] == input[i] &&
               run < 130) {
            ++run;
        }

        if (run >= 3) {
            encoded += 2;
            i += run;
            continue;
        }

        size_t literalLength = 0;
        while (i < length && literalLength < 128) {
            run = 1;
            while (i + run < length &&
                   input[i + run] == input[i] &&
                   run < 130) {
                ++run;
            }

            if (run >= 3 && literalLength > 0) break;
            if (run >= 3) break;

            ++i;
            ++literalLength;
        }

        if (literalLength == 0) {
            // Sem se běžně nedostaneme, ale chráníme se proti nekonečné smyčce.
            ++i;
            literalLength = 1;
        }

        encoded += 1 + literalLength;
    }

    return encoded;
}

bool DisplayPreview::pack(
    const uint8_t* input,
    size_t length,
    uint8_t* output,
    size_t outputSize) {
    if (input == nullptr || output == nullptr) return false;

    size_t i = 0;
    size_t out = 0;

    while (i < length) {
        size_t run = 1;
        while (i + run < length &&
               input[i + run] == input[i] &&
               run < 130) {
            ++run;
        }

        if (run >= 3) {
            if (out + 2 > outputSize) return false;
            output[out++] =
                static_cast<uint8_t>(0x80 | (run - 3));
            output[out++] = input[i];
            i += run;
            continue;
        }

        const size_t literalStart = i;
        size_t literalLength = 0;

        while (i < length && literalLength < 128) {
            run = 1;
            while (i + run < length &&
                   input[i + run] == input[i] &&
                   run < 130) {
                ++run;
            }

            if (run >= 3 && literalLength > 0) break;
            if (run >= 3) break;

            ++i;
            ++literalLength;
        }

        if (literalLength == 0) {
            ++i;
            literalLength = 1;
        }

        if (out + 1 + literalLength > outputSize) return false;

        output[out++] =
            static_cast<uint8_t>(literalLength - 1);
        memcpy(
            output + out,
            input + literalStart,
            literalLength);
        out += literalLength;
    }

    return out == outputSize;
}

void DisplayPreview::capture(
    IScreen& screen,
    const DataModel& dataModel,
    bool fullRefresh) {
    if (!available()) return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        Serial.println("[DISPLAY-PREVIEW] Timeout pri publikovani nahledu.");
        return;
    }

    if (!ensureCanvas()) {
        // Posledni uspesny snapshot ponechame dostupny. Selhani noveho
        // capture nesmi zneplatnit nahled, ktery uz v pameti mame.
        xSemaphoreGive(_mutex);
        return;
    }

    _canvas->fillScreen(1);
    _useUnicodeFont = false;
    screen.render(*this, dataModel);

    const uint8_t* bitmap = _canvas->getBuffer();
    const size_t encodedBytes =
        packedSize(bitmap, BitmapBytes);

    uint8_t* nextPacked = nullptr;
    bool stored = false;

    if (encodedBytes > 0 &&
        encodedBytes <= MaxStoredBytes) {
        nextPacked =
            new (std::nothrow) uint8_t[encodedBytes];

        if (nextPacked != nullptr) {
            stored = pack(
                bitmap,
                BitmapBytes,
                nextPacked,
                encodedBytes);
        }
    }

    if (stored) {
        delete[] _packed;
        _packed = nextPacked;
        _packedBytes = encodedBytes;

        _lastScreenId = screen.getId();
        _lastRefreshMs = millis();
        _lastFullRefresh = fullRefresh;
        _lastWifiRssi = dataModel.system.wifiRssi;
        _lastWifiSignalLevel =
            dataModel.system.wifiSignalLevel;
        _lastWifiConnected =
            dataModel.system.wifiConnected;
        _lastWifiAccessPoint =
            dataModel.system.wifiAccessPoint;
        _lastNtpSynced =
            dataModel.system.ntpSynced;
        _lastGoodweAvailable =
            dataModel.solar.status.available;
        _lastAzrouterAvailable =
            dataModel.azrouter.status.available;
        _lastRefreshTime =
            dataModel.system.ntpSynced
                ? dataModel.system.dateStr +
                    " " +
                    dataModel.system.timeStr
                : String();

        _hasCapture = true;
        ++_generation;
    } else {
        delete[] nextPacked;

        Serial.printf(
            "[DISPLAY-PREVIEW] Novy snapshot nelze ulozit: pack=%u B, limit=%u B, free=%u, maxBlock=%u%s\n",
            static_cast<unsigned>(encodedBytes),
            static_cast<unsigned>(MaxStoredBytes),
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap(),
            _hasCapture ? " | ponechavam predchozi nahled" : "");
    }

    releaseCanvas();

    if (stored) {
        Serial.printf(
            "[DISPLAY-PREVIEW] Publikovan nahled '%s', generace %lu, %u B (%.1f%% raw). free=%u maxBlock=%u\n",
            _lastScreenId.c_str(),
            static_cast<unsigned long>(_generation),
            static_cast<unsigned>(_packedBytes),
            100.0f *
                static_cast<float>(_packedBytes) /
                static_cast<float>(BitmapBytes),
            ESP.getFreeHeap(),
            ESP.getMaxAllocHeap());
    }

    xSemaphoreGive(_mutex);
}

String DisplayPreview::metadataJson() {
    JsonDocument doc;

    if (!available()) {
        doc["ready"] = false;
        doc["reason"] = "preview_unavailable";
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
    doc["refreshType"] =
        _lastFullRefresh ? "full" : "partial";
    doc["wifiRssi"] = _lastWifiRssi;
    doc["wifiSignalLevel"] = _lastWifiSignalLevel;
    doc["wifiConnected"] = _lastWifiConnected;
    doc["wifiAccessPoint"] = _lastWifiAccessPoint;
    doc["ntpSynced"] = _lastNtpSynced;
    doc["goodweAvailable"] = _lastGoodweAvailable;
    doc["azrouterAvailable"] = _lastAzrouterAvailable;
    doc["storedBytes"] = _packedBytes;
    doc["rawBytes"] = BitmapBytes;

    String response;
    serializeJson(doc, response);
    xSemaphoreGive(_mutex);
    return response;
}

void DisplayPreview::buildBmpHeader(uint8_t* header) const {
    memset(header, 0, BmpHeaderBytes);

    auto put16 = [header](size_t offset, uint16_t value) {
        header[offset] =
            static_cast<uint8_t>(value & 0xFF);
        header[offset + 1] =
            static_cast<uint8_t>((value >> 8) & 0xFF);
    };

    auto put32 = [header](size_t offset, uint32_t value) {
        header[offset] =
            static_cast<uint8_t>(value & 0xFF);
        header[offset + 1] =
            static_cast<uint8_t>((value >> 8) & 0xFF);
        header[offset + 2] =
            static_cast<uint8_t>((value >> 16) & 0xFF);
        header[offset + 3] =
            static_cast<uint8_t>((value >> 24) & 0xFF);
    };

    header[0] = 'B';
    header[1] = 'M';
    put32(2, static_cast<uint32_t>(BmpBytes));
    put32(10, static_cast<uint32_t>(BmpHeaderBytes));

    put32(14, 40);
    put32(18, static_cast<uint32_t>(Width));
    put32(
        22,
        static_cast<uint32_t>(-Height));
    put16(26, 1);
    put16(28, 1);
    put32(34, static_cast<uint32_t>(BitmapBytes));
    put32(38, 2835);
    put32(42, 2835);
    put32(46, 2);
    put32(50, 2);

    header[54] = 0;
    header[55] = 0;
    header[56] = 0;
    header[57] = 0;

    header[58] = 255;
    header[59] = 255;
    header[60] = 255;
    header[61] = 0;
}

bool DisplayPreview::writeUnpacked(
    WiFiClient& client) const {
    if (_packed == nullptr || _packedBytes == 0) return false;

    // Posilat jednotlive RLE bloky znamenalo stovky velmi malych TCP write()
    // volani. Na Wi-Fi pak 48kB BMP dokazal blokovat WebServer pres 10 s.
    // Data proto skládáme do vetsiho vystupniho bufferu a posilame po blocich.
    constexpr size_t OutputBufferBytes = 1024;
    uint8_t outputBuffer[OutputBufferBytes];
    size_t buffered = 0;

    auto flush = [&]() -> bool {
        size_t written = 0;
        while (written < buffered) {
            const size_t chunk =
                client.write(
                    outputBuffer + written,
                    buffered - written);
            if (chunk == 0) return false;
            written += chunk;
        }
        buffered = 0;
        return true;
    };

    auto emit = [&](const uint8_t* data, size_t length) -> bool {
        while (length > 0) {
            const size_t space = OutputBufferBytes - buffered;
            const size_t chunk = min(space, length);
            memcpy(outputBuffer + buffered, data, chunk);
            buffered += chunk;
            data += chunk;
            length -= chunk;

            if (buffered == OutputBufferBytes && !flush()) {
                return false;
            }
        }
        return true;
    };

    auto emitRun = [&](uint8_t value, size_t length) -> bool {
        while (length > 0) {
            const size_t space = OutputBufferBytes - buffered;
            const size_t chunk = min(space, length);
            memset(outputBuffer + buffered, value, chunk);
            buffered += chunk;
            length -= chunk;

            if (buffered == OutputBufferBytes && !flush()) {
                return false;
            }
        }
        return true;
    };

    size_t input = 0;
    size_t produced = 0;

    while (input < _packedBytes &&
           produced < BitmapBytes) {
        const uint8_t header = _packed[input++];

        if ((header & 0x80) != 0) {
            const size_t runLength =
                static_cast<size_t>(
                    header & 0x7F) +
                3;

            if (input >= _packedBytes ||
                produced + runLength > BitmapBytes) {
                return false;
            }

            const uint8_t value = _packed[input++];
            if (!emitRun(value, runLength)) {
                return false;
            }
            produced += runLength;
        } else {
            const size_t literalLength =
                static_cast<size_t>(header) + 1;

            if (input + literalLength > _packedBytes ||
                produced + literalLength > BitmapBytes) {
                return false;
            }

            if (!emit(
                    _packed + input,
                    literalLength)) {
                return false;
            }

            input += literalLength;
            produced += literalLength;
        }
    }

    if (produced != BitmapBytes ||
        input != _packedBytes) {
        return false;
    }

    return buffered == 0 || flush();
}

bool DisplayPreview::writeBmp(WiFiClient& client) {
    if (!available()) return false;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    if (!_hasCapture ||
        _packed == nullptr ||
        _packedBytes == 0) {
        xSemaphoreGive(_mutex);
        return false;
    }

    uint8_t header[BmpHeaderBytes];
    buildBmpHeader(header);

    bool ok =
        client.write(header, sizeof(header)) ==
        sizeof(header);

    if (ok) ok = writeUnpacked(client);

    xSemaphoreGive(_mutex);
    return ok;
}

void DisplayPreview::clear(uint16_t color) {
    if (_canvas) {
        _canvas->fillScreen(normalizeColor(color));
    }
}

void DisplayPreview::drawPixel(
    int16_t x,
    int16_t y,
    uint16_t color) {
    if (_canvas) {
        _canvas->drawPixel(
            x,
            y,
            normalizeColor(color));
    }
}

void DisplayPreview::drawLine(
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    uint16_t color) {
    if (_canvas) {
        _canvas->drawLine(
            x0,
            y0,
            x1,
            y1,
            normalizeColor(color));
    }
}

void DisplayPreview::drawRect(
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    uint16_t color) {
    if (_canvas) {
        _canvas->drawRect(
            x,
            y,
            w,
            h,
            normalizeColor(color));
    }
}

void DisplayPreview::fillRect(
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    uint16_t color) {
    if (_canvas) {
        _canvas->fillRect(
            x,
            y,
            w,
            h,
            normalizeColor(color));
    }
}

void DisplayPreview::drawRoundRect(
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    int16_t r,
    uint16_t color) {
    if (_canvas) {
        _canvas->drawRoundRect(
            x,
            y,
            w,
            h,
            r,
            normalizeColor(color));
    }
}

void DisplayPreview::fillRoundRect(
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    int16_t r,
    uint16_t color) {
    if (_canvas) {
        _canvas->fillRoundRect(
            x,
            y,
            w,
            h,
            r,
            normalizeColor(color));
    }
}

void DisplayPreview::drawCircle(
    int16_t x,
    int16_t y,
    int16_t r,
    uint16_t color) {
    if (_canvas) {
        _canvas->drawCircle(
            x,
            y,
            r,
            normalizeColor(color));
    }
}

void DisplayPreview::fillCircle(
    int16_t x,
    int16_t y,
    int16_t r,
    uint16_t color) {
    if (_canvas) {
        _canvas->fillCircle(
            x,
            y,
            r,
            normalizeColor(color));
    }
}

void DisplayPreview::drawBitmap(
    int16_t x,
    int16_t y,
    const uint8_t* bitmap,
    int16_t w,
    int16_t h,
    uint16_t color) {
    if (_canvas) {
        _canvas->drawBitmap(
            x,
            y,
            bitmap,
            w,
            h,
            normalizeColor(color));
    }
}

void DisplayPreview::drawInvertedBitmap(
    int16_t x,
    int16_t y,
    const uint8_t* bitmap,
    int16_t w,
    int16_t h,
    uint16_t color) {
    if (!_canvas || bitmap == nullptr) return;

    const int16_t byteWidth = (w + 7) / 8;
    uint8_t byte = 0;

    for (int16_t j = 0; j < h; ++j) {
        for (int16_t i = 0; i < w; ++i) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte =
                    pgm_read_byte(
                        &bitmap[
                            j * byteWidth +
                            i / 8]);
            }

            if (!(byte & 0x80)) {
                _canvas->drawPixel(
                    x + i,
                    y + j,
                    normalizeColor(color));
            }
        }
    }
}

void DisplayPreview::setFont(const GFXfont* f) {
    _useUnicodeFont = false;
    if (_canvas) _canvas->setFont(f);
}

void DisplayPreview::setUnicodeFont(
    const uint8_t* font) {
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

void DisplayPreview::setCursor(
    int16_t x,
    int16_t y) {
    if (_useUnicodeFont) {
        _u8g2.setCursor(x, y);
    } else if (_canvas) {
        _canvas->setCursor(x, y);
    }
}

void DisplayPreview::print(const String& text) {
    if (_useUnicodeFont) {
        _u8g2.print(text);
    } else if (_canvas) {
        _canvas->print(text);
    }
}

void DisplayPreview::printf(
    const char* format,
    ...) {
    char buf[256];

    va_list args;
    va_start(args, format);
    vsnprintf(
        buf,
        sizeof(buf),
        format,
        args);
    va_end(args);

    if (_useUnicodeFont) {
        _u8g2.print(buf);
    } else if (_canvas) {
        _canvas->print(buf);
    }
}

int16_t DisplayPreview::textWidth(
    const String& text) {
    if (_useUnicodeFont) {
        return _u8g2.getUTF8Width(
            text.c_str());
    }

    if (!_canvas) return 0;

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    _canvas->getTextBounds(
        text,
        0,
        0,
        &x1,
        &y1,
        &w,
        &h);

    return static_cast<int16_t>(w);
}

void DisplayPreview::beginFrame(bool) {
}

bool DisplayPreview::nextFrame() {
    return false;
}

void DisplayPreview::powerOff() {
}
