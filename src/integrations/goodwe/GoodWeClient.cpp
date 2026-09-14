#include "GoodWeClient.h"
#include "../../diagnostics/Performance.h"

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
    _udp.begin(0); // Lokální UDP port pro příjem
    Serial.printf("[GOODWE] Inicializován klient UDP %s:%u (Unit ID: 0xF7, Modbus RTU/UDP)\n", _host.c_str(), _port);
}

bool GoodWeClient::update(SolarData& solarData) {
    Performance::Scope timing(Performance::GoodWe);
    if (_host.isEmpty()) {
        solarData.status.recordError("No Host");
        return false;
    }

    // GoodWe GW10K-ET ověřený dotaz: Unit 0xF7, Read 0x03, Start 35100 (0x891C), Count 125 (0x007D), CRC (0x7AE7)
    const uint8_t request[8] = {0xF7, 0x03, 0x89, 0x1C, 0x00, 0x7D, 0x7A, 0xE7};

    uint8_t buffer[320];
    int len = 0;
    bool success = false;

    // Až 3 pokusy o přečtení (UDP paket se v síti může ztratit)
    for (int attempt = 1; attempt <= 3; attempt++) {
        while (_udp.parsePacket() > 0) {
            _udp.flush(); // Vyprázdnění případných starých dat
        }

        if (!_udp.beginPacket(_host.c_str(), _port)) {
            continue;
        }
        _udp.write(request, sizeof(request));
        if (!_udp.endPacket()) {
            continue;
        }

        // Čekání na odpověď (max 1200 ms)
        unsigned long start = millis();
        int packetSize = 0;
        while (millis() - start < 1200) {
            packetSize = _udp.parsePacket();
            if (packetSize > 0) break;
            delay(15);
        }

        if (packetSize >= 10) {
            len = _udp.read(buffer, sizeof(buffer));
            success = true;
            break;
        }
    }

    if (!success || len < 10) {
        solarData.status.recordError("Timeout (3 attempts)");
        return false;
    }

    // 1. Ošetření prefixu "AA 55", který GoodWe Wi-Fi modul vrací před RTU rámcem
    size_t rtuOffset = 0;
    if (len >= 2 && buffer[0] == 0xAA && buffer[1] == 0x55) {
        rtuOffset = 2;
    }

    const uint8_t* rtu = buffer + rtuOffset;
    size_t rtuLen = len - rtuOffset;

    if (rtuLen < 5) {
        solarData.status.recordError("RTU frame too short");
        return false;
    }

    // 2. Kontrola Modbus CRC16 (CRC se počítá pouze z RTU rámce od bajtu 0xF7, bez AA 55)
    uint16_t receivedCrc = (uint16_t)(rtu[rtuLen - 2] | (rtu[rtuLen - 1] << 8));
    uint16_t calcCrc = calculateCrc(rtu, rtuLen - 2);
    if (receivedCrc != calcCrc) {
        solarData.status.recordError("CRC mismatch");
        Serial.printf("[GOODWE] CRC mismatch: recv=0x%04X, calc=0x%04X\n", receivedCrc, calcCrc);
        return false;
    }

    // 3. Kontrola Unit ID (0xF7) a Modbus funkce (0x03)
    if (rtu[0] != 0xF7) {
        solarData.status.recordError("Invalid Unit ID");
        return false;
    }

    // Ošetření Modbus Exception (např. 0x83)
    if ((rtu[1] & 0x80) != 0) {
        uint8_t exceptionCode = rtu[2];
        solarData.status.recordError("Modbus Exception 0x" + String(exceptionCode, HEX));
        return false;
    }

    if (rtu[1] != 0x03) {
        solarData.status.recordError("Invalid Function Code");
        return false;
    }

    uint8_t byteCount = rtu[2];
    if (rtuLen != (size_t)(3 + byteCount + 2)) {
        solarData.status.recordError("Length mismatch");
        return false;
    }

    // 4. Parsování datových registrů (data začínají za rtu[3])
    // Startovní registr = 35100 (0x891C)
    // Offset registru v payloadu = (RegAddress - 35100) * 2
    const uint8_t* data = rtu + 3;
    auto regOffset = [](uint16_t reg) -> size_t {
        return (reg - 35100) * 2;
    };

    // PV1 & PV2 výkon (W)
    uint32_t ppv1 = readUInt32(data, regOffset(35105));
    uint32_t ppv2 = readUInt32(data, regOffset(35109));
    solarData.productionPowerW = (float)(ppv1 + ppv2);

    // Dnešní výroba FVE (e_day): reg 35193-35194 (uint32 / 10 = kWh)
    uint32_t eDayRaw = readUInt32(data, regOffset(35193));
    solarData.energyTodayKWh = eDayRaw * 0.1f;

    // Baterie:
    // pbattery1: reg 35182-35183 (int32, kladné = vybíjení/dodávka, záporné = nabíjení)
    int32_t pBatt = readInt32(data, regOffset(35182));
    solarData.batteryPowerW = (float)pBatt;

    // Baterie napětí / proud:
    uint16_t vBattRaw = readUInt16(data, regOffset(35180)); // 0.1 V
    float vBatt = vBattRaw * 0.1f;
    // Baterie SoC odhad z napětí, pokud není k dispozici registr SoC
    if (vBatt > 100.0f) {
        // High voltage battery (typicky GoodWe ET 180 - 450V)
        solarData.batterySocPercent = constrain((vBatt - 180.0f) / (260.0f - 180.0f) * 100.0f, 0.0f, 100.0f);
    }

    // Síť (Active Power Meter): reg 35140 (int16, kladné = přetok/export do sítě, záporné = nákup ze sítě)
    int16_t pActive = readInt16(data, regOffset(35140));
    solarData.gridPowerW = (float)pActive;

    // Spotřeba domu (House Consumption):
    // Dle ET dokumentace: P_load = P_pv + P_battery_discharge - P_active_export
    int16_t pLoadRaw = readInt16(data, regOffset(35172)); // load_ptotal
    float calculatedHouseLoad = solarData.productionPowerW + solarData.batteryPowerW - solarData.gridPowerW;
    if (calculatedHouseLoad >= 0) {
        solarData.houseConsumptionW = calculatedHouseLoad;
    } else if (pLoadRaw > 0) {
        solarData.houseConsumptionW = (float)pLoadRaw;
    } else {
        solarData.houseConsumptionW = 0.0f;
    }

    solarData.lastUpdateMs = millis();
    solarData.status.recordSuccess();

    Serial.printf("[GOODWE] FVE: %.0f W (Dnes: %.1f kWh) | Batt: %.0f W | Síť: %+.0f W | Dům: %.0f W\n",
                  solarData.productionPowerW,
                  solarData.energyTodayKWh,
                  solarData.batteryPowerW,
                  solarData.gridPowerW,
                  solarData.houseConsumptionW);

    return true;
}
