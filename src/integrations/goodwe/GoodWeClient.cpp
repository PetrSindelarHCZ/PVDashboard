#include "GoodWeClient.h"
#include "../../diagnostics/Performance.h"
#include <WiFi.h>

namespace {
constexpr int MaxAttempts = 2;
constexpr uint32_t ResponseTimeoutMs = 700;
}

// Modbus RTU CRC16 (polynomial 0xA001, init 0xFFFF)
uint16_t GoodWeClient::calculateCrc(const uint8_t* buffer, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

int16_t GoodWeClient::readInt16(const uint8_t* buffer, size_t offset) {
    return (int16_t)((buffer[offset] << 8) | buffer[offset + 1]);
}

uint16_t GoodWeClient::readUInt16(const uint8_t* buffer, size_t offset) {
    return (uint16_t)((buffer[offset] << 8) | buffer[offset + 1]);
}

int32_t GoodWeClient::readInt32(const uint8_t* buffer, size_t offset) {
    return (int32_t)(((uint32_t)buffer[offset] << 24) |
                     ((uint32_t)buffer[offset + 1] << 16) |
                     ((uint32_t)buffer[offset + 2] << 8) |
                     ((uint32_t)buffer[offset + 3]));
}

uint32_t GoodWeClient::readUInt32(const uint8_t* buffer, size_t offset) {
    return ((uint32_t)buffer[offset] << 24) |
           ((uint32_t)buffer[offset + 1] << 16) |
           ((uint32_t)buffer[offset + 2] << 8) |
           ((uint32_t)buffer[offset + 3]);
}

GoodWeClient::GoodWeClient() {
}

void GoodWeClient::begin(const String& host, uint16_t port) {
    _host = host;
    _port = port;
    _udp.begin(0);
    _remoteIpKnown = _remoteIp.fromString(_host);
    if (!_remoteIpKnown) {
        _remoteIpKnown = WiFi.hostByName(_host.c_str(), _remoteIp) == 1;
    }
    Serial.printf("[GOODWE] Inicializován klient UDP %s:%u (Unit ID: 0xF7, Modbus RTU/UDP)\n", _host.c_str(), _port);
}

bool GoodWeClient::readHoldingRegisters(uint16_t startRegister, uint16_t count,
                                        uint8_t* dataOut, size_t dataCapacity,
                                        size_t& dataLength) {
    dataLength = 0;
    if (count == 0 || count > 125 || dataCapacity < static_cast<size_t>(count) * 2) {
        return false;
    }

    uint8_t request[8] = {
        0xF7, 0x03,
        static_cast<uint8_t>((startRegister >> 8) & 0xFF),
        static_cast<uint8_t>(startRegister & 0xFF),
        static_cast<uint8_t>((count >> 8) & 0xFF),
        static_cast<uint8_t>(count & 0xFF),
        0, 0
    };
    const uint16_t requestCrc = calculateCrc(request, 6);
    request[6] = static_cast<uint8_t>(requestCrc & 0xFF);
    request[7] = static_cast<uint8_t>((requestCrc >> 8) & 0xFF);

    uint8_t buffer[320];

    for (int attempt = 1; attempt <= MaxAttempts; ++attempt) {
        while (_udp.parsePacket() > 0) {
            _udp.flush();
        }

        if (!_udp.beginPacket(_host.c_str(), _port)) continue;
        _udp.write(request, sizeof(request));
        if (!_udp.endPacket()) continue;

        const unsigned long started = millis();
        int packetSize = 0;
        while (millis() - started < ResponseTimeoutMs) {
            packetSize = _udp.parsePacket();
            if (packetSize <= 0) {
                delay(10);
                continue;
            }

            const bool validPort = _udp.remotePort() == _port;
            const bool validHost = !_remoteIpKnown || _udp.remoteIP() == _remoteIp;
            if (validPort && validHost) break;

            _udp.flush();
            packetSize = 0;
        }

        if (packetSize < 5) {
            if (packetSize > 0) _udp.flush();
            continue;
        }

        const int len = _udp.read(buffer, sizeof(buffer));
        if (len < 5) continue;

        size_t rtuOffset = 0;
        if (len >= 2 && buffer[0] == 0xAA && buffer[1] == 0x55) {
            rtuOffset = 2;
        }

        const uint8_t* rtu = buffer + rtuOffset;
        const size_t rtuLen = static_cast<size_t>(len) - rtuOffset;
        if (rtuLen < 5) continue;

        const uint16_t receivedCrc = static_cast<uint16_t>(rtu[rtuLen - 2] | (rtu[rtuLen - 1] << 8));
        const uint16_t calcCrc = calculateCrc(rtu, rtuLen - 2);
        if (receivedCrc != calcCrc) {
            Serial.printf("[GOODWE] CRC mismatch %u/%u: recv=0x%04X calc=0x%04X\n",
                          startRegister, count, receivedCrc, calcCrc);
            continue;
        }

        if (rtu[0] != 0xF7) continue;
        if ((rtu[1] & 0x80) != 0) {
            Serial.printf("[GOODWE] Modbus exception %u/%u: 0x%02X\n",
                          startRegister, count, rtu[2]);
            return false;
        }
        if (rtu[1] != 0x03) continue;

        const uint8_t byteCount = rtu[2];
        if (byteCount != count * 2 || rtuLen != static_cast<size_t>(3 + byteCount + 2)) {
            continue;
        }

        memcpy(dataOut, rtu + 3, byteCount);
        dataLength = byteCount;
        return true;
    }

    return false;
}

bool GoodWeClient::update(SolarData& solarData) {
    Performance::Scope timing(Performance::GoodWe);
    if (_host.isEmpty()) {
        solarData.status.recordError("No Host");
        return false;
    }

    uint8_t data[250];
    size_t dataLength = 0;
    if (!readHoldingRegisters(35100, 125, data, sizeof(data), dataLength)) {
        solarData.status.recordError("Runtime read failed");
        return false;
    }

    auto regOffset = [](uint16_t reg) -> size_t {
        return (reg - 35100) * 2;
    };

    const uint32_t ppv1 = readUInt32(data, regOffset(35105));
    const uint32_t ppv2 = readUInt32(data, regOffset(35109));
    solarData.productionPowerW = static_cast<float>(ppv1 + ppv2);

    const uint32_t eDayRaw = readUInt32(data, regOffset(35193));
    solarData.energyTodayKWh = eDayRaw * 0.1f;

    const uint16_t vBattRaw = readUInt16(data, regOffset(35180));
    const int16_t iBattRaw = readInt16(data, regOffset(35181));
    const int32_t pBatt = readInt32(data, regOffset(35182));
    const float vBatt = vBattRaw * 0.1f;
    const float iBatt = iBattRaw * 0.1f;
    solarData.batteryPowerW = static_cast<float>(pBatt);

    // BMS block is optional. It remains useful even when no battery is installed:
    // the tested inverter returns valid zero values in that state.
    uint8_t bmsData[20];
    size_t bmsLength = 0;
    bool bmsAvailable = readHoldingRegisters(37000, 10, bmsData, sizeof(bmsData), bmsLength);
    float bmsSoc = 0.0f;
    float bmsSoh = 0.0f;
    if (bmsAvailable && bmsLength >= 18) {
        bmsSoc = static_cast<float>(readUInt16(bmsData, (37007 - 37000) * 2));
        bmsSoh = static_cast<float>(readUInt16(bmsData, (37008 - 37000) * 2));
    }

    solarData.batteryPresent =
        vBatt > 1.0f ||
        fabsf(iBatt) > 0.1f ||
        pBatt != 0 ||
        bmsSoc > 0.0f ||
        bmsSoh > 0.0f;

    if (solarData.batteryPresent) {
        if (bmsSoc > 0.0f && bmsSoc <= 100.0f) {
            solarData.batterySocPercent = bmsSoc;
        } else if (vBatt > 100.0f) {
            solarData.batterySocPercent =
                constrain((vBatt - 180.0f) / (260.0f - 180.0f) * 100.0f, 0.0f, 100.0f);
        }
    } else {
        solarData.batterySocPercent = 0.0f;
        solarData.batteryPowerW = 0.0f;
    }

    const int16_t pActiveInternal = readInt16(data, regOffset(35140));
    solarData.gridPowerW = static_cast<float>(pActiveInternal);

    // Prefer the external meter when available. If this optional request fails,
    // retain the runtime register 35140 as a fallback.
    uint8_t meterData[18];
    size_t meterLength = 0;
    if (readHoldingRegisters(36000, 9, meterData, sizeof(meterData), meterLength) && meterLength >= 18) {
        const int16_t meterTotal = readInt16(meterData, (36008 - 36000) * 2);
        solarData.gridPowerW = static_cast<float>(meterTotal);
    }

    const int16_t pLoadRaw = readInt16(data, regOffset(35172));
    // Prefer the inverter's measured load register. The calculated balance is
    // retained only as a fallback for invalid/negative measured values.
    if (pLoadRaw >= 0) {
        solarData.houseConsumptionW = static_cast<float>(pLoadRaw);
    } else {
        const float calculatedHouseLoad =
            solarData.productionPowerW + solarData.batteryPowerW - solarData.gridPowerW;
        solarData.houseConsumptionW = calculatedHouseLoad > 0.0f ? calculatedHouseLoad : 0.0f;
    }

    solarData.lastUpdateMs = millis();
    solarData.status.recordSuccess();

    Serial.printf("[GOODWE] FVE: %.0f W (Dnes: %.1f kWh) | Batt: %s %.0f W %.0f%% | Síť: %+.0f W | Dům: %.0f W\n",
                  solarData.productionPowerW,
                  solarData.energyTodayKWh,
                  solarData.batteryPresent ? "ano" : "ne",
                  solarData.batteryPowerW,
                  solarData.batterySocPercent,
                  solarData.gridPowerW,
                  solarData.houseConsumptionW);

    return true;
}
