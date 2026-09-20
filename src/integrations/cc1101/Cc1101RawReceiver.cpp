#include "Cc1101RawReceiver.h"

#include <Arduino.h>
#include <SPI.h>
#include "../../../include/AppConfig.h"

namespace Cc1101RawReceiver {
namespace {

constexpr uint8_t Sres = 0x30;
constexpr uint8_t Srx = 0x34;

constexpr uint8_t IOCFG2 = 0x00;
constexpr uint8_t IOCFG0 = 0x02;
constexpr uint8_t PKTCTRL1 = 0x07;
constexpr uint8_t PKTCTRL0 = 0x08;
constexpr uint8_t FSCTRL1 = 0x0B;
constexpr uint8_t FREQ2 = 0x0D;
constexpr uint8_t FREQ1 = 0x0E;
constexpr uint8_t FREQ0 = 0x0F;
constexpr uint8_t MDMCFG4 = 0x10;
constexpr uint8_t MDMCFG3 = 0x11;
constexpr uint8_t MDMCFG2 = 0x12;
constexpr uint8_t MDMCFG1 = 0x13;
constexpr uint8_t DEVIATN = 0x15;
constexpr uint8_t MCSM0 = 0x18;
constexpr uint8_t FOCCFG = 0x19;
constexpr uint8_t BSCFG = 0x1A;
constexpr uint8_t AGCCTRL2 = 0x1B;
constexpr uint8_t AGCCTRL1 = 0x1C;
constexpr uint8_t AGCCTRL0 = 0x1D;
constexpr uint8_t FREND1 = 0x21;
constexpr uint8_t FSCAL3 = 0x23;
constexpr uint8_t FSCAL2 = 0x24;
constexpr uint8_t FSCAL1 = 0x25;
constexpr uint8_t FSCAL0 = 0x26;
constexpr uint8_t TEST2 = 0x2C;
constexpr uint8_t TEST1 = 0x2D;
constexpr uint8_t TEST0 = 0x2E;

constexpr uint32_t SpiFrequencyHz = 4000000;
constexpr uint32_t ChipReadyTimeoutUs = 10000;

// Pulse filtering / burst framing.
constexpr uint32_t MinimumPulseUs = 70;
constexpr uint32_t BurstGapUs = 18000;
constexpr uint32_t CarrierHoldUs = 16000;
constexpr uint16_t MinimumBurstPulses = 8;
constexpr uint16_t MaximumPulseCount = 768;

volatile int32_t pulses[MaximumPulseCount];
volatile uint16_t pulseCount = 0;
volatile uint32_t lastEdgeUs = 0;
volatile uint32_t lastActivityUs = 0;
volatile uint32_t lastCarrierSeenUs = 0;
volatile uint16_t carrierHighEdges = 0;
volatile uint16_t carrierHoldEdges = 0;
volatile bool overflowed = false;
volatile bool receiverReady = false;
volatile bool captureSuppressed = false;

SensorObservationCallback sensorObservationCallback;

void publishSensorObservation(const RfSensorObservation& observation) {
    if (sensorObservationCallback) sensorObservationCallback(observation);
}

constexpr uint8_t Ft017ThSensorCapacity = 8;

struct Ft017ThSensorEntry {
    bool used = false;
    uint16_t candidateId = 0;
    uint16_t unknownA = 0;
    uint8_t unknownB = 0;
    float temperatureC = 0.0f;
    float humidityPercent = 0.0f;
    uint32_t packetCount = 0;
    uint32_t firstSeenMs = 0;
    uint32_t lastSeenMs = 0;
};

Ft017ThSensorEntry ft017ThSensors[Ft017ThSensorCapacity];

Ft017ThSensorEntry& sensorEntryFor(uint16_t candidateId) {
    Ft017ThSensorEntry* freeEntry = nullptr;
    Ft017ThSensorEntry* oldestEntry = &ft017ThSensors[0];

    for (auto& entry : ft017ThSensors) {
        if (entry.used && entry.candidateId == candidateId) {
            return entry;
        }
        if (!entry.used && freeEntry == nullptr) {
            freeEntry = &entry;
        }
        if (!entry.used || entry.lastSeenMs < oldestEntry->lastSeenMs) {
            oldestEntry = &entry;
        }
    }

    Ft017ThSensorEntry& selected =
        freeEntry != nullptr ? *freeEntry : *oldestEntry;
    selected = Ft017ThSensorEntry{};
    selected.used = true;
    selected.candidateId = candidateId;
    selected.firstSeenMs = millis();
    return selected;
}

bool waitForChipReady() {
    const uint32_t started = micros();
    while (digitalRead(CC1101_MISO_PIN) == HIGH) {
        if (static_cast<uint32_t>(micros() - started) >= ChipReadyTimeoutUs) {
            return false;
        }
        delayMicroseconds(1);
    }
    return true;
}

bool strobe(uint8_t command, const SPISettings& settings) {
    SPI.beginTransaction(settings);
    digitalWrite(CC1101_CS_PIN, LOW);
    if (!waitForChipReady()) {
        digitalWrite(CC1101_CS_PIN, HIGH);
        SPI.endTransaction();
        return false;
    }
    SPI.transfer(command);
    digitalWrite(CC1101_CS_PIN, HIGH);
    SPI.endTransaction();
    return true;
}

bool writeRegister(uint8_t address, uint8_t value, const SPISettings& settings) {
    SPI.beginTransaction(settings);
    digitalWrite(CC1101_CS_PIN, LOW);
    if (!waitForChipReady()) {
        digitalWrite(CC1101_CS_PIN, HIGH);
        SPI.endTransaction();
        return false;
    }
    SPI.transfer(address);
    SPI.transfer(value);
    digitalWrite(CC1101_CS_PIN, HIGH);
    SPI.endTransaction();
    return true;
}

bool resetChip(const SPISettings& settings) {
    digitalWrite(CC1101_CS_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(CC1101_CS_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(CC1101_CS_PIN, HIGH);
    delayMicroseconds(45);

    SPI.beginTransaction(settings);
    digitalWrite(CC1101_CS_PIN, LOW);
    if (!waitForChipReady()) {
        digitalWrite(CC1101_CS_PIN, HIGH);
        SPI.endTransaction();
        return false;
    }

    SPI.transfer(Sres);
    if (!waitForChipReady()) {
        digitalWrite(CC1101_CS_PIN, HIGH);
        SPI.endTransaction();
        return false;
    }

    digitalWrite(CC1101_CS_PIN, HIGH);
    SPI.endTransaction();
    delay(1);
    return true;
}

bool configureReceiver(const SPISettings& settings) {
    struct RegisterValue {
        uint8_t address;
        uint8_t value;
    };

    // 433.92 MHz, ASK/OOK, asynchronous serial RX.
    // Data rate ~4.8 kBaud and RX bandwidth ~203 kHz are deliberately broad
    // first-pass values for common low-rate 433 MHz weather/remote sensors.
    static constexpr RegisterValue settingsTable[] = {
        {IOCFG2, 0x0E},   // GDO2 = carrier sense
        {IOCFG0, 0x0D},   // GDO0 = asynchronous serial data
        {PKTCTRL1, 0x00},
        {PKTCTRL0, 0x32}, // async serial, CRC off
        {FSCTRL1, 0x06},
        {FREQ2, 0x10},    // 433.92 MHz @ 26 MHz crystal
        {FREQ1, 0xB0},
        {FREQ0, 0x71},
        {MDMCFG4, 0x87},  // 203 kHz RX BW, DRATE_E=7
        {MDMCFG3, 0x83},  // ~4.798 kBaud
        {MDMCFG2, 0x30},  // ASK/OOK, no sync qualification
        {MDMCFG1, 0x22},
        {DEVIATN, 0x15},
        {MCSM0, 0x18},
        {FOCCFG, 0x16},
        {BSCFG, 0x6C},
        {AGCCTRL2, 0x06},
        {AGCCTRL1, 0x00},
        {AGCCTRL0, 0x92},
        {FREND1, 0x56},
        {FSCAL3, 0xE9},
        {FSCAL2, 0x2A},
        {FSCAL1, 0x00},
        {FSCAL0, 0x1F},
        {TEST2, 0x81},
        {TEST1, 0x35},
        {TEST0, 0x09},
    };

    for (const auto& item : settingsTable) {
        if (!writeRegister(item.address, item.value, settings)) {
            return false;
        }
    }
    return true;
}

void IRAM_ATTR onRawEdge() {
    if (captureSuppressed) {
        return;
    }

    const uint32_t now = micros();
    const bool carrierHigh = digitalRead(CC1101_GDO2_PIN) == HIGH;

    if (carrierHigh) {
        lastCarrierSeenUs = now;
        if (carrierHighEdges != 0xFFFF) {
            ++carrierHighEdges;
        }
    } else {
        // Some weather protocols contain intentional long OOK gaps inside
        // one packet. GDO2 carrier-sense can fall during those gaps, so keep
        // accepting GDO0 edges for a short grace period after the last
        // confirmed carrier instead of cutting the frame immediately.
        const uint32_t carrierAge =
            lastCarrierSeenUs == 0
                ? 0xFFFFFFFFUL
                : static_cast<uint32_t>(now - lastCarrierSeenUs);
        if (carrierAge > CarrierHoldUs) {
            lastEdgeUs = 0;
            return;
        }
        if (carrierHoldEdges != 0xFFFF) {
            ++carrierHoldEdges;
        }
    }

    lastActivityUs = now;
    const uint32_t previous = lastEdgeUs;
    lastEdgeUs = now;

    if (previous == 0) {
        return;
    }

    const uint32_t duration = now - previous;
    if (duration < MinimumPulseUs) {
        return;
    }

    if (pulseCount >= MaximumPulseCount) {
        overflowed = true;
        return;
    }

    // Interrupt fires after the level has changed. Therefore the duration
    // belongs to the previous level.
    const bool currentHigh = digitalRead(CC1101_GDO0_PIN) == HIGH;
    pulses[pulseCount++] =
        currentHigh
            ? -static_cast<int32_t>(duration) // previous level was LOW
            : static_cast<int32_t>(duration); // previous level was HIGH
}


bool tryPrintTfaTwinPlus(const int32_t* data, uint16_t count) {
    // TFA Twin Plus 30.3049 / Conrad KW9010 / Ea2 BL999 family.
    //
    // rtl_433 documents a 36-bit OOK/PPM frame:
    //   short LOW gap ~2 ms => 0
    //   long  LOW gap ~4 ms => 1
    //   inter-row gap ~6-10 ms
    //
    // Carrin Electronics manufactured/rebranded weather sensors in the same
    // product family as the Hyundai WS Senzor 77 TH, so this is currently a
    // protocol candidate for the physical Hyundai unit. Do not label it as
    // Hyundai until values are confirmed against the sensor display.
    constexpr uint32_t PulseMinUs = 100;
    constexpr uint32_t PulseMaxUs = 1400;
    constexpr uint32_t ZeroGapMinUs = 1300;
    constexpr uint32_t ZeroGapMaxUs = 2900;
    constexpr uint32_t OneGapMinUs = 3000;
    constexpr uint32_t OneGapMaxUs = 5600;
    constexpr uint32_t RowGapMinUs = 6000;
    constexpr uint32_t RowGapMaxUs = 10000;
    constexpr uint8_t FrameBits = 36;
    constexpr uint8_t MinimumRepeats = 2;

    auto duration = [](int32_t pulse) -> uint32_t {
        return static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);
    };

    auto reverse8Local = [](uint8_t v) -> uint8_t {
        v = static_cast<uint8_t>((v >> 4) | (v << 4));
        v = static_cast<uint8_t>(((v & 0xCC) >> 2) | ((v & 0x33) << 2));
        v = static_cast<uint8_t>(((v & 0xAA) >> 1) | ((v & 0x55) << 1));
        return v;
    };

    uint64_t rows[12] = {};
    uint8_t rowBits[12] = {};
    uint8_t rowCount = 0;
    uint64_t current = 0;
    uint8_t currentBits = 0;

    for (uint16_t i = 0; i + 1 < count; ++i) {
        if (data[i] <= 0 || data[i + 1] >= 0) {
            continue;
        }

        const uint32_t pulseUs = duration(data[i]);
        if (pulseUs < PulseMinUs || pulseUs > PulseMaxUs) {
            continue;
        }

        const uint32_t gapUs = duration(data[i + 1]);

        if (gapUs >= ZeroGapMinUs && gapUs <= ZeroGapMaxUs) {
            if (currentBits < 63) {
                current <<= 1;
                ++currentBits;
            } else {
                current = 0;
                currentBits = 0;
            }
            ++i;
            continue;
        }

        if (gapUs >= OneGapMinUs && gapUs <= OneGapMaxUs) {
            if (currentBits < 63) {
                current = (current << 1) | 1ULL;
                ++currentBits;
            } else {
                current = 0;
                currentBits = 0;
            }
            ++i;
            continue;
        }

        if (gapUs >= RowGapMinUs && gapUs < RowGapMaxUs) {
            if (rowCount < 12 && currentBits > 0) {
                rows[rowCount] = current;
                rowBits[rowCount] = currentBits;
                ++rowCount;
            }
            current = 0;
            currentBits = 0;
            ++i;
            continue;
        }

        if (gapUs >= RowGapMaxUs) {
            if (rowCount < 12 && currentBits > 0) {
                rows[rowCount] = current;
                rowBits[rowCount] = currentBits;
                ++rowCount;
            }
            current = 0;
            currentBits = 0;
            ++i;
        }
    }

    if (rowCount < MinimumRepeats) return false;

    for (uint8_t r = 0; r < rowCount; ++r) {
        if (rowBits[r] != FrameBits) continue;

        uint8_t repeats = 1;
        for (uint8_t s = static_cast<uint8_t>(r + 1); s < rowCount; ++s) {
            if (rowBits[s] == FrameBits && rows[s] == rows[r]) {
                ++repeats;
            }
        }
        if (repeats < MinimumRepeats) continue;

        const uint64_t frame = rows[r];
        const uint8_t b0 = static_cast<uint8_t>((frame >> 28) & 0xFF);
        const uint8_t b1 = static_cast<uint8_t>((frame >> 20) & 0xFF);
        const uint8_t b2 = static_cast<uint8_t>((frame >> 12) & 0xFF);
        const uint8_t b3 = static_cast<uint8_t>((frame >> 4) & 0xFF);
        const uint8_t b4 = static_cast<uint8_t>((frame & 0x0F) << 4);

        const uint8_t rb0 = reverse8Local(b0);
        const uint8_t rb1 = reverse8Local(b1);
        const uint8_t rb2 = reverse8Local(b2);
        const uint8_t rb3 = reverse8Local(b3);
        const uint8_t rb4 = reverse8Local(b4);

        const uint8_t sumNibbles = static_cast<uint8_t>(
            (rb0 >> 4) + (rb0 & 0x0F) +
            (rb1 >> 4) + (rb1 & 0x0F) +
            (rb2 >> 4) + (rb2 & 0x0F) +
            (rb3 >> 4) + (rb3 & 0x0F));
        const uint8_t checksum = static_cast<uint8_t>(rb4 & 0x0F);
        if (checksum != (sumNibbles & 0x0F)) continue;

        const bool negative = (b2 & 0x07) != 0;
        const int tempRaw =
            ((static_cast<int>(rb2) & 0x1F) << 4) |
            (static_cast<int>(rb1) >> 4);
        const float temperatureC =
            (negative ? -((1 << 9) - tempRaw) : tempRaw) * 0.1f;

        const int humidity =
            (static_cast<int>(rb3) & 0x7F) - 28;
        const uint8_t sensorId = static_cast<uint8_t>(
            (rb0 & 0x0F) | ((rb0 & 0xC0) >> 2));
        const bool batteryLow = (b1 & 0x80) != 0;
        const uint8_t channel =
            static_cast<uint8_t>((b0 >> 2) & 0x03);

        if (channel < 1 || channel > 3 ||
            humidity < 0 || humidity > 100 ||
            temperatureC < -60.0f || temperatureC > 80.0f) {
            continue;
        }

        static uint32_t packetCount = 0;
        static uint32_t lastSeenMs = 0;
        const uint32_t nowMs = millis();
        const uint32_t intervalMs =
            lastSeenMs == 0 ? 0 : nowMs - lastSeenMs;
        lastSeenMs = nowMs;
        ++packetCount;

        Serial.printf(
            "[CC1101][TFA-TWIN] id=0x%02X temp=%.1f C humidity=%d %% "
            "channel=%u battery=%s repeats=%u checksum=OK | packets=%lu",
            static_cast<unsigned>(sensorId),
            temperatureC,
            humidity,
            static_cast<unsigned>(channel),
            batteryLow ? "LOW" : "OK",
            static_cast<unsigned>(repeats),
            static_cast<unsigned long>(packetCount));

        if (intervalMs > 0) {
            Serial.printf(
                " interval=%.1f s",
                static_cast<double>(intervalMs) / 1000.0);
        }

        Serial.printf(
            " | raw=%09llX\n",
            static_cast<unsigned long long>(frame));

        RfSensorObservation observation;
        observation.protocol = "tfa-twin";
        observation.sensorId = sensorId;
        observation.channel = channel;
        observation.hasTemperature = true;
        observation.temperatureC = temperatureC;
        observation.hasHumidity = true;
        observation.humidityPercent = humidity;
        observation.hasBattery = true;
        observation.batteryOk = !batteryLow;
        publishSensorObservation(observation);
        return true;
    }

    return false;
}

bool tryPrintNexusTh(const int32_t* data, uint16_t count) {
    // Nexus temperature/humidity protocol family.
    //
    // Observed from an unidentified Nexus-TH compatible sensor in RF range:
    //   HIGH pulse ~0.45-0.56 ms
    //   LOW gap  ~1.0 ms => 0
    //   LOW gap  ~2.0 ms => 1
    //   LOW sync ~4.0 ms between repeated 36-bit frames
    //
    // Payload (9 nibbles / 36 bits):
    //   [id0][id1][flags][temp0][temp1][temp2][0xF][humi0][humi1]
    // flags = B T C C, temperature = signed 12-bit / 10.
    //
    // The protocol has no checksum, so require repeated identical rows plus
    // the constant 0xF nibble and sane channel/humidity values.
    constexpr uint32_t PulseMinUs = 350;
    constexpr uint32_t PulseMaxUs = 700;
    constexpr uint32_t ZeroGapMinUs = 700;
    constexpr uint32_t ZeroGapMaxUs = 1300;
    constexpr uint32_t OneGapMinUs = 1600;
    constexpr uint32_t OneGapMaxUs = 2400;
    constexpr uint32_t SyncGapMinUs = 3000;
    constexpr uint32_t SyncGapMaxUs = 5000;
    constexpr uint8_t FrameBits = 36;
    constexpr uint8_t MinimumRepeats = 3;

    auto duration = [](int32_t pulse) -> uint32_t {
        return static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);
    };

    uint64_t rows[16] = {};
    uint8_t rowBits[16] = {};
    uint8_t rowCount = 0;
    uint64_t current = 0;
    uint8_t currentBits = 0;

    for (uint16_t i = 0; i + 1 < count; ++i) {
        if (data[i] <= 0 || data[i + 1] >= 0) {
            continue;
        }

        const uint32_t pulseUs = duration(data[i]);
        if (pulseUs < PulseMinUs || pulseUs > PulseMaxUs) {
            continue;
        }

        const uint32_t gapUs = duration(data[i + 1]);

        if (gapUs >= ZeroGapMinUs && gapUs <= ZeroGapMaxUs) {
            if (currentBits < 63) {
                current <<= 1;
                ++currentBits;
            } else {
                current = 0;
                currentBits = 0;
            }
            ++i;
            continue;
        }

        if (gapUs >= OneGapMinUs && gapUs <= OneGapMaxUs) {
            if (currentBits < 63) {
                current = (current << 1) | 1ULL;
                ++currentBits;
            } else {
                current = 0;
                currentBits = 0;
            }
            ++i;
            continue;
        }

        if (gapUs >= SyncGapMinUs && gapUs <= SyncGapMaxUs) {
            if (rowCount < 16 && currentBits > 0) {
                rows[rowCount] = current;
                rowBits[rowCount] = currentBits;
                ++rowCount;
            }
            current = 0;
            currentBits = 0;
            ++i;
            continue;
        }
    }

    if (rowCount < MinimumRepeats) {
        return false;
    }

    for (uint8_t r = 0; r < rowCount; ++r) {
        if (rowBits[r] != FrameBits) continue;

        uint8_t repeats = 1;
        for (uint8_t s = static_cast<uint8_t>(r + 1); s < rowCount; ++s) {
            if (rowBits[s] == FrameBits && rows[s] == rows[r]) {
                ++repeats;
            }
        }
        if (repeats < MinimumRepeats) continue;

        const uint64_t frame = rows[r];
        const uint8_t id =
            static_cast<uint8_t>((frame >> 28) & 0xFF);
        const uint8_t flags =
            static_cast<uint8_t>((frame >> 24) & 0x0F);
        const uint16_t tempRaw12 =
            static_cast<uint16_t>((frame >> 12) & 0x0FFF);
        const uint8_t constantNibble =
            static_cast<uint8_t>((frame >> 8) & 0x0F);
        const uint8_t humidity =
            static_cast<uint8_t>(frame & 0xFF);

        if (constantNibble != 0x0F) continue;

        const uint8_t channel =
            static_cast<uint8_t>((flags & 0x03) + 1);
        if (channel > 3 || humidity > 100) continue;

        int16_t signedTemp = static_cast<int16_t>(tempRaw12);
        if ((signedTemp & 0x0800) != 0) {
            signedTemp = static_cast<int16_t>(signedTemp | 0xF000);
        }
        const float temperatureC =
            static_cast<float>(signedTemp) * 0.1f;

        if (temperatureC < -60.0f || temperatureC > 80.0f) {
            continue;
        }

        const bool batteryOk = (flags & 0x08) != 0;
        const bool testMode = (flags & 0x04) != 0;

        static uint32_t packetCount = 0;
        static uint32_t lastSeenMs = 0;
        const uint32_t nowMs = millis();
        const uint32_t intervalMs =
            lastSeenMs == 0 ? 0 : nowMs - lastSeenMs;
        lastSeenMs = nowMs;
        ++packetCount;

        Serial.printf(
            "[CC1101][NEXUS-TH] id=0x%02X temp=%.1f C humidity=%u %% "
            "channel=%u battery=%s test=%s repeats=%u | packets=%lu",
            static_cast<unsigned>(id),
            temperatureC,
            static_cast<unsigned>(humidity),
            static_cast<unsigned>(channel),
            batteryOk ? "OK" : "LOW",
            testMode ? "ON" : "OFF",
            static_cast<unsigned>(repeats),
            static_cast<unsigned long>(packetCount));

        if (intervalMs > 0) {
            Serial.printf(
                " interval=%.1f s",
                static_cast<double>(intervalMs) / 1000.0);
        }

        Serial.printf(
            " | raw=%09llX\n",
            static_cast<unsigned long long>(frame));

        RfSensorObservation observation;
        observation.protocol = "nexus-th";
        observation.sensorId = id;
        observation.channel = channel;
        observation.hasTemperature = true;
        observation.temperatureC = temperatureC;
        observation.hasHumidity = true;
        observation.humidityPercent = humidity;
        observation.hasBattery = true;
        observation.batteryOk = batteryOk;
        publishSensorObservation(observation);
        return true;
    }

    return false;
}

bool tryPrintHyundaiWs(const int32_t* data, uint16_t count) {
    // Hyundai WS SENZOR Remote Temperature Sensor, based on the protocol
    // documented by rtl_433.
    //
    // OOK/PPM framing:
    //   HIGH pulse: ~224 us (fixed)
    //   following LOW gap ~1032 us => 0
    //   following LOW gap ~1992 us => 1
    //   after 24 data bits an extra short HIGH is followed by an
    //   inter-packet LOW gap around 4016 us.
    // The same 24-bit payload is transmitted repeatedly (typically ~23x).
    //
    // There is no checksum, so require several identical repeated frames
    // before accepting a packet. This also lets us decode the first part of
    // a long capture even when the shared raw buffer later overflows.
    constexpr uint32_t PulseMinUs = 120;
    constexpr uint32_t PulseMaxUs = 420;
    constexpr uint32_t ZeroGapMinUs = 650;
    constexpr uint32_t ZeroGapMaxUs = 1450;
    constexpr uint32_t OneGapMinUs = 1450;
    constexpr uint32_t OneGapMaxUs = 2550;
    constexpr uint32_t PacketGapMinUs = 2800;
    constexpr uint32_t PacketGapMaxUs = 5500;
    constexpr uint8_t FrameBits = 24;
    constexpr uint8_t MinimumRepeats = 4;

    auto duration = [](int32_t pulse) -> uint32_t {
        return static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);
    };

    auto decodeFrameAt = [&](uint16_t start,
                             uint32_t& frame,
                             uint16_t& nextPos) -> bool {
        frame = 0;
        uint16_t pos = start;

        for (uint8_t bit = 0; bit < FrameBits; ++bit) {
            if (pos + 1 >= count ||
                data[pos] <= 0 ||
                data[pos + 1] >= 0) {
                return false;
            }

            const uint32_t pulseUs = duration(data[pos]);
            const uint32_t gapUs = duration(data[pos + 1]);

            if (pulseUs < PulseMinUs || pulseUs > PulseMaxUs) {
                return false;
            }

            frame <<= 1;
            if (gapUs >= ZeroGapMinUs && gapUs < ZeroGapMaxUs) {
                // logical 0
            } else if (gapUs >= OneGapMinUs && gapUs <= OneGapMaxUs) {
                frame |= 1U;
            } else {
                return false;
            }

            pos += 2;
        }

        // rtl_433's PPM slicer ends a row on a gap above gap_limit.
        // In the raw waveform this is another normal short HIGH followed by
        // the ~4 ms packet gap.
        if (pos + 1 >= count ||
            data[pos] <= 0 ||
            data[pos + 1] >= 0) {
            return false;
        }

        const uint32_t separatorPulseUs = duration(data[pos]);
        const uint32_t packetGapUs = duration(data[pos + 1]);
        if (separatorPulseUs < PulseMinUs ||
            separatorPulseUs > PulseMaxUs ||
            packetGapUs < PacketGapMinUs ||
            packetGapUs > PacketGapMaxUs) {
            return false;
        }

        nextPos = static_cast<uint16_t>(pos + 2);
        return true;
    };

    for (uint16_t start = 0; start + 49 < count; ++start) {
        uint32_t firstFrame = 0;
        uint16_t nextPos = 0;
        if (!decodeFrameAt(start, firstFrame, nextPos)) {
            continue;
        }

        uint8_t repeats = 1;
        uint16_t scanPos = nextPos;

        while (scanPos + 49 < count && repeats < 32) {
            uint32_t repeatedFrame = 0;
            uint16_t repeatedNext = 0;
            if (decodeFrameAt(scanPos, repeatedFrame, repeatedNext) &&
                repeatedFrame == firstFrame) {
                ++repeats;
                scanPos = repeatedNext;
                continue;
            }

            // Allow a few stray pulses between repeats, but do not search so
            // far that an unrelated transmission can be counted as a repeat.
            bool recovered = false;
            const uint16_t recoveryEnd = min<uint16_t>(
                count,
                static_cast<uint16_t>(scanPos + 8));
            for (uint16_t candidate = static_cast<uint16_t>(scanPos + 1);
                 candidate + 49 < recoveryEnd;
                 ++candidate) {
                if (decodeFrameAt(candidate, repeatedFrame, repeatedNext) &&
                    repeatedFrame == firstFrame) {
                    ++repeats;
                    scanPos = repeatedNext;
                    recovered = true;
                    break;
                }
            }
            if (!recovered) break;
        }

        if (repeats < MinimumRepeats) {
            continue;
        }

        const uint8_t b0 = static_cast<uint8_t>((firstFrame >> 16) & 0xFF);
        const uint8_t b1 = static_cast<uint8_t>((firstFrame >> 8) & 0xFF);
        const uint8_t b2 = static_cast<uint8_t>(firstFrame & 0xFF);

        if ((b0 == 0x00 && b1 == 0x00 && b2 == 0x00) ||
            (b0 == 0xFF && b1 == 0xFF && b2 == 0xFF)) {
            continue;
        }

        const int16_t packedTemperature = static_cast<int16_t>(
            (static_cast<uint16_t>(b0) << 8) |
            static_cast<uint16_t>(b1 & 0xF0));
        const int16_t temperatureDeciC = packedTemperature >> 4;
        const float temperatureC =
            static_cast<float>(temperatureDeciC) * 0.1f;

        const bool batteryOk = (b1 & 0x08) != 0;
        const bool startup = (b1 & 0x04) != 0;
        const uint8_t channel =
            static_cast<uint8_t>((b1 & 0x03) + 1);
        const uint8_t sensorId = b2;

        if (temperatureC < -60.0f ||
            temperatureC > 80.0f ||
            channel < 1 ||
            channel > 3) {
            continue;
        }

        static uint32_t packetCount = 0;
        static uint32_t lastSeenMs = 0;
        const uint32_t nowMs = millis();
        const uint32_t intervalMs =
            lastSeenMs == 0 ? 0 : nowMs - lastSeenMs;
        lastSeenMs = nowMs;
        ++packetCount;

        Serial.printf(
            "[CC1101][HYUNDAI] id=0x%02X temp=%.1f C channel=%u "
            "battery=%s startup=%s repeats=%u | packets=%lu",
            static_cast<unsigned>(sensorId),
            temperatureC,
            static_cast<unsigned>(channel),
            batteryOk ? "OK" : "LOW",
            startup ? "ON" : "OFF",
            static_cast<unsigned>(repeats),
            static_cast<unsigned long>(packetCount));

        if (intervalMs > 0) {
            Serial.printf(
                " interval=%.1f s",
                static_cast<double>(intervalMs) / 1000.0);
        }

        Serial.printf(
            " | raw=%02X %02X %02X\n",
            b0,
            b1,
            b2);
        return true;
    }

    return false;
}

bool tryPrintAuriolHg02832(const int32_t* data, uint16_t count) {
    // Lidl/Auriol HG02832 / HG05124A-DCF family.
    //
    // Observed on the user's physical sensor:
    //   preamble: L/H sync pulses around 0.8-0.9 ms,
    //   data: 40 bits, encoded by HIGH pulse width
    //     short HIGH ~0.25-0.30 ms = 0
    //     long  HIGH ~0.60-0.66 ms = 1
    //   LOW time is complementary (~0.55-0.60 ms after short HIGH,
    //   ~0.20-0.25 ms after long HIGH).
    //
    // Payload:
    //   byte0: sensor ID
    //   byte1: humidity
    //   byte2: battery/TX/channel flags + upper temperature nibble
    //   byte3: lower temperature byte
    //   byte4: checksum
    constexpr uint32_t SyncMinUs = 700;
    constexpr uint32_t SyncMaxUs = 1050;
    constexpr uint32_t ShortHighMinUs = 180;
    constexpr uint32_t ShortHighMaxUs = 380;
    constexpr uint32_t LongHighMinUs = 480;
    constexpr uint32_t LongHighMaxUs = 760;
    constexpr uint32_t LowAfterShortMinUs = 430;
    constexpr uint32_t LowAfterShortMaxUs = 780;
    constexpr uint32_t LowAfterLongMinUs = 150;
    constexpr uint32_t LowAfterLongMaxUs = 380;
    constexpr uint8_t FrameBits = 40;

    auto duration = [](int32_t pulse) -> uint32_t {
        return static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);
    };

    auto crc8OneByte = [](uint8_t value) -> uint8_t {
        uint8_t crc = 0x53;
        crc ^= value;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80)
                ? static_cast<uint8_t>((crc << 1) ^ 0x31)
                : static_cast<uint8_t>(crc << 1);
        }
        return crc;
    };

    // Seven alternating sync-level pulses precede the first data HIGH in
    // captures from the physical sensor: L H L H L H L.
    for (uint16_t start = 7; start < count; ++start) {
        if (data[start] <= 0) continue;

        bool preambleOk = true;
        for (uint8_t j = 0; j < 7; ++j) {
            const uint16_t pos = static_cast<uint16_t>(start - 7 + j);
            const bool expectLow = (j % 2) == 0;
            if ((expectLow && data[pos] >= 0) ||
                (!expectLow && data[pos] <= 0)) {
                preambleOk = false;
                break;
            }
            const uint32_t us = duration(data[pos]);
            if (us < SyncMinUs || us > SyncMaxUs) {
                preambleOk = false;
                break;
            }
        }
        if (!preambleOk) continue;

        uint8_t bytes[5] = {0, 0, 0, 0, 0};
        uint16_t pos = start;
        bool frameOk = true;

        for (uint8_t bit = 0; bit < FrameBits; ++bit) {
            if (pos >= count || data[pos] <= 0) {
                frameOk = false;
                break;
            }

            const uint32_t highUs = duration(data[pos]);
            bool value = false;
            bool shortHigh = false;

            if (highUs >= ShortHighMinUs && highUs <= ShortHighMaxUs) {
                value = false;
                shortHigh = true;
            } else if (highUs >= LongHighMinUs && highUs <= LongHighMaxUs) {
                value = true;
            } else {
                frameOk = false;
                break;
            }

            bytes[bit / 8] <<= 1;
            if (value) bytes[bit / 8] |= 1;

            // The final HIGH may be the last pulse in the captured burst.
            if (bit == FrameBits - 1) {
                ++pos;
                break;
            }

            if (pos + 1 >= count || data[pos + 1] >= 0) {
                frameOk = false;
                break;
            }

            const uint32_t lowUs = duration(data[pos + 1]);
            if (shortHigh) {
                if (lowUs < LowAfterShortMinUs ||
                    lowUs > LowAfterShortMaxUs) {
                    frameOk = false;
                    break;
                }
            } else {
                if (lowUs < LowAfterLongMinUs ||
                    lowUs > LowAfterLongMaxUs) {
                    frameOk = false;
                    break;
                }
            }

            pos += 2;
        }

        if (!frameOk) continue;

        const uint8_t folded =
            static_cast<uint8_t>(bytes[0] ^ bytes[1] ^ bytes[2] ^ bytes[3]);
        const uint8_t expectedChecksum = crc8OneByte(folded);
        if (expectedChecksum != bytes[4]) continue;

        const uint8_t id = bytes[0];
        const uint8_t humidity = bytes[1];
        const bool batteryLow = (bytes[2] & 0x80) != 0;
        const bool txButton = (bytes[2] & 0x40) != 0;
        const uint8_t channel =
            static_cast<uint8_t>(((bytes[2] & 0x30) >> 4) + 1);

        const int16_t packedTemperature = static_cast<int16_t>(
            (static_cast<uint16_t>(bytes[2] & 0x0F) << 12) |
            (static_cast<uint16_t>(bytes[3]) << 4));
        const int16_t temperatureDeciC = packedTemperature >> 4;
        const float temperatureC =
            static_cast<float>(temperatureDeciC) * 0.1f;

        if (humidity > 100 ||
            temperatureC < -60.0f ||
            temperatureC > 80.0f) {
            continue;
        }

        static uint32_t packetCount = 0;
        static uint32_t lastSeenMs = 0;
        const uint32_t nowMs = millis();
        const uint32_t intervalMs =
            lastSeenMs == 0 ? 0 : nowMs - lastSeenMs;
        lastSeenMs = nowMs;
        ++packetCount;

        Serial.printf(
            "[CC1101][AURIOL] id=0x%02X temp=%.1f C humidity=%u %% "
            "channel=%u battery=%s tx=%s | checksum=OK packets=%lu",
            static_cast<unsigned>(id),
            temperatureC,
            static_cast<unsigned>(humidity),
            static_cast<unsigned>(channel),
            batteryLow ? "LOW" : "OK",
            txButton ? "ON" : "OFF",
            static_cast<unsigned long>(packetCount));

        if (intervalMs > 0) {
            Serial.printf(
                " interval=%.1f s",
                static_cast<double>(intervalMs) / 1000.0);
        }

        Serial.printf(
            " | raw=%02X %02X %02X %02X %02X\n",
            bytes[0],
            bytes[1],
            bytes[2],
            bytes[3],
            bytes[4]);

        RfSensorObservation observation;
        observation.protocol = "auriol";
        observation.sensorId = id;
        observation.channel = channel;
        observation.hasTemperature = true;
        observation.temperatureC = temperatureC;
        observation.hasHumidity = true;
        observation.humidityPercent = humidity;
        observation.hasBattery = true;
        observation.batteryOk = !batteryLow;
        publishSensorObservation(observation);
        return true;
    }

    return false;
}

bool tryPrintPwm67Candidate(const int32_t* data, uint16_t count) {
    constexpr uint32_t SyncLeadLowMin = 1400;
    constexpr uint32_t SyncLeadLowMax = 2400;
    constexpr uint32_t SyncHighMin = 6000;
    constexpr uint32_t SyncHighMax = 8500;
    constexpr uint32_t SyncLowMin = 8500;
    constexpr uint32_t SyncLowMax = 12000;

    auto duration = [](int32_t pulse) -> uint32_t {
        return static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);
    };

    for (uint16_t i = 0; i + 2 < count; ++i) {
        if (data[i] >= 0 || data[i + 1] <= 0 || data[i + 2] >= 0) {
            continue;
        }

        const uint32_t leadLow = duration(data[i]);
        const uint32_t high = duration(data[i + 1]);
        const uint32_t low = duration(data[i + 2]);

        if (leadLow < SyncLeadLowMin || leadLow > SyncLeadLowMax ||
            high < SyncHighMin || high > SyncHighMax ||
            low < SyncLowMin || low > SyncLowMax) {
            continue;
        }

        uint8_t preambleLows = 1;
        int32_t p = static_cast<int32_t>(i) - 2;
        while (p >= 0) {
            const int32_t lowPulse = data[p];
            const int32_t highPulse = data[p + 1];
            const uint32_t lowUs = duration(lowPulse);
            const uint32_t highUs = duration(highPulse);

            if (lowPulse >= 0 || highPulse <= 0 ||
                lowUs < 1500 || lowUs > 2300 ||
                highUs < 450 || highUs > 1050) {
                break;
            }

            ++preambleLows;
            p -= 2;
        }

        Serial.printf(
            "[CC1101][PWM67?] sync candidate burst=%u at=%u preamble=%u "
            "L%lu H%lu L%lu | undecoded",
            static_cast<unsigned>(count),
            static_cast<unsigned>(i),
            static_cast<unsigned>(preambleLows),
            static_cast<unsigned long>(leadLow),
            static_cast<unsigned long>(high),
            static_cast<unsigned long>(low));

        const uint16_t dataStart = static_cast<uint16_t>(i + 3);
        if (dataStart < count) {
            Serial.print(" | next=");
            const uint16_t end = min<uint16_t>(
                count,
                static_cast<uint16_t>(dataStart + 16));
            for (uint16_t j = dataStart; j < end; ++j) {
                if (j > dataStart) Serial.print(' ');
                const int32_t pulse = data[j];
                Serial.print(pulse >= 0 ? 'H' : 'L');
                Serial.print(static_cast<unsigned long>(
                    pulse >= 0 ? pulse : -pulse));
            }
        }

        Serial.println();
        return true;
    }

    return false;
}

bool tryPrintPwm67(const int32_t* data, uint16_t count) {
    // Unknown periodic 433 MHz source seen roughly every 67 seconds.
    // Signature observed repeatedly:
    //   7x (L ~1.9 ms, H ~0.7 ms)
    //   L ~1.9 ms, H ~7.2 ms, L ~10.3 ms
    // followed by pulse-width encoded bits:
    //   0 = H short (~0.7 ms), L long (~1.9 ms)
    //   1 = H long  (~1.8 ms), L short (~0.8 ms)
    constexpr uint32_t LongLowMin = 1500;
    constexpr uint32_t LongLowMax = 2300;
    constexpr uint32_t ShortHighMin = 450;
    constexpr uint32_t ShortHighMax = 1050;
    constexpr uint32_t LongHighMin = 1500;
    constexpr uint32_t LongHighMax = 2300;
    constexpr uint32_t ShortLowMin = 450;
    constexpr uint32_t ShortLowMax = 1100;
    constexpr uint32_t FooterLowMin = 2500;
    constexpr uint32_t FooterLowMax = 16000;
    constexpr uint32_t SyncHighMin = 6000;
    constexpr uint32_t SyncHighMax = 8500;
    constexpr uint32_t SyncLowMin = 8500;
    constexpr uint32_t SyncLowMax = 11500;
    constexpr uint8_t MinimumPreambleLows = 6;
    constexpr uint8_t MinimumDecodedBits = 6;

    auto level = [](int32_t pulse) -> char {
        return pulse >= 0 ? 'H' : 'L';
    };
    auto duration = [](int32_t pulse) -> uint32_t {
        return static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);
    };

    for (uint16_t syncStart = 0; syncStart + 2 < count; ++syncStart) {
        const int32_t syncLead = data[syncStart];
        const int32_t syncHigh = data[syncStart + 1];
        const int32_t syncLow = data[syncStart + 2];

        if (level(syncLead) != 'L' ||
            duration(syncLead) < LongLowMin ||
            duration(syncLead) > LongLowMax ||
            level(syncHigh) != 'H' ||
            duration(syncHigh) < SyncHighMin ||
            duration(syncHigh) > SyncHighMax ||
            level(syncLow) != 'L' ||
            duration(syncLow) < SyncLowMin ||
            duration(syncLow) > SyncLowMax) {
            continue;
        }

        uint8_t preambleLows = 1; // syncLead is the final ~1.9 ms preamble low.
        int32_t i = static_cast<int32_t>(syncStart) - 2;
        while (i >= 0) {
            const int32_t lowPulse = data[i];
            const int32_t highPulse = data[i + 1];
            if (level(lowPulse) != 'L' ||
                duration(lowPulse) < LongLowMin ||
                duration(lowPulse) > LongLowMax ||
                level(highPulse) != 'H' ||
                duration(highPulse) < ShortHighMin ||
                duration(highPulse) > ShortHighMax) {
                break;
            }
            ++preambleLows;
            i -= 2;
        }

        if (preambleLows < MinimumPreambleLows) continue;

        char bits[64];
        uint8_t bitCount = 0;
        uint16_t pos = syncStart + 3;

        while (pos + 1 < count && bitCount < sizeof(bits) - 1) {
            const int32_t highPulse = data[pos];
            const int32_t lowPulse = data[pos + 1];
            if (level(highPulse) != 'H' || level(lowPulse) != 'L') break;

            const uint32_t highUs = duration(highPulse);
            const uint32_t lowUs = duration(lowPulse);

            if (highUs >= ShortHighMin && highUs <= ShortHighMax &&
                lowUs >= LongLowMin && lowUs <= LongLowMax) {
                bits[bitCount++] = '0';
            } else if (highUs >= LongHighMin && highUs <= LongHighMax &&
                       lowUs >= ShortLowMin && lowUs <= ShortLowMax) {
                bits[bitCount++] = '1';
            } else {
                break;
            }

            pos += 2;
        }

        bool footerTerminatedBit = false;
        uint32_t footerLowUs = 0;

        // Repeated captures end the data section with a normal long HIGH
        // (~1.8 ms) followed by a much longer LOW. The LOW varied from
        // ~3.1 ms to ~12.1 ms, so interpret this as the final logical 1
        // followed by an end-of-frame/inter-frame gap.
        if (pos + 1 < count &&
            level(data[pos]) == 'H' &&
            level(data[pos + 1]) == 'L') {
            const uint32_t highUs = duration(data[pos]);
            const uint32_t lowUs = duration(data[pos + 1]);
            if (highUs >= LongHighMin && highUs <= LongHighMax &&
                lowUs >= FooterLowMin && lowUs <= FooterLowMax &&
                bitCount < sizeof(bits) - 1) {
                bits[bitCount++] = '1';
                footerTerminatedBit = true;
                footerLowUs = lowUs;
                pos += 2;
            }
        }

        bits[bitCount] = '\0';
        if (bitCount < MinimumDecodedBits) continue;

        bool truncatedFooter = false;
        bool partialBit = false;
        char partialValue = '?';
        uint32_t partialHighUs = 0;

        // If the capture ends immediately after a valid long HIGH, repeated
        // observations show this is the final logical 1 and the LOW footer
        // was simply not captured.
        if (!footerTerminatedBit &&
            pos + 1 == count &&
            level(data[pos]) == 'H') {
            const uint32_t highUs = duration(data[pos]);
            if (highUs >= LongHighMin &&
                highUs <= LongHighMax &&
                bitCount < sizeof(bits) - 1) {
                bits[bitCount++] = '1';
                bits[bitCount] = '\0';
                truncatedFooter = true;
                partialHighUs = highUs;
                ++pos;
            }
        }

        if (!footerTerminatedBit &&
            !truncatedFooter &&
            pos < count &&
            level(data[pos]) == 'H') {
            partialHighUs = duration(data[pos]);
            if (partialHighUs >= ShortHighMin &&
                partialHighUs <= ShortHighMax) {
                partialBit = true;
                partialValue = '0';
            } else if (partialHighUs >= LongHighMin &&
                       partialHighUs <= LongHighMax) {
                partialBit = true;
                partialValue = '1';
            }
        }

        static uint32_t packetCount = 0;
        static uint32_t lastSeenMs = 0;
        const uint32_t nowMs = millis();
        const uint32_t intervalMs =
            lastSeenMs == 0 ? 0 : nowMs - lastSeenMs;
        lastSeenMs = nowMs;
        ++packetCount;

        const uint16_t remainingPulses =
            pos < count ? static_cast<uint16_t>(count - pos) : 0;

        Serial.printf(
            "[CC1101][PWM67] burst=%u syncAt=%u preamble=%u bits=%u data=%s",
            static_cast<unsigned>(count),
            static_cast<unsigned>(syncStart),
            static_cast<unsigned>(preambleLows),
            static_cast<unsigned>(bitCount),
            bits);

        if (footerTerminatedBit) {
            Serial.printf(
                " footer=%lu us",
                static_cast<unsigned long>(footerLowUs));
        } else if (truncatedFooter) {
            Serial.printf(
                " truncated-footer(H%lu)",
                static_cast<unsigned long>(partialHighUs));
        } else if (partialBit) {
            Serial.printf(
                " +%c?(H%lu)",
                partialValue,
                static_cast<unsigned long>(partialHighUs));
        }

        Serial.printf(
            " | stopAt=%u remain=%u | sync=L%lu H%lu L%lu | packets=%lu",
            static_cast<unsigned>(pos),
            static_cast<unsigned>(remainingPulses),
            static_cast<unsigned long>(duration(syncLead)),
            static_cast<unsigned long>(duration(syncHigh)),
            static_cast<unsigned long>(duration(syncLow)),
            static_cast<unsigned long>(packetCount));

        if (intervalMs > 0) {
            Serial.printf(
                " interval=%.1f s",
                static_cast<double>(intervalMs) / 1000.0);
        }

        if (remainingPulses > 0) {
            Serial.print(" | next=");
            const uint16_t end = min<uint16_t>(
                count,
                static_cast<uint16_t>(pos + 12));
            for (uint16_t j = pos; j < end; ++j) {
                if (j > pos) Serial.print(' ');
                const int32_t pulse = data[j];
                Serial.print(pulse >= 0 ? 'H' : 'L');
                Serial.print(static_cast<unsigned long>(
                    pulse >= 0 ? pulse : -pulse));
            }
        }

        Serial.println();
        return true;
    }

    return false;
}

bool tryPrintFt017Th(const char* decoded, uint16_t frameBits) {
    if (frameBits != 65) return false;

    for (uint8_t i = 0; i < 9; ++i) {
        if (decoded[i] != '1') return false;
    }

    auto readBits = [decoded](uint16_t start, uint8_t count) -> uint16_t {
        uint16_t value = 0;
        for (uint8_t i = 0; i < count; ++i) {
            value <<= 1;
            if (decoded[start + i] == '1') value |= 1;
        }
        return value;
    };

    const uint16_t candidateId = readBits(9, 9);
    const uint16_t unknownA = readBits(18, 15);
    const uint16_t temperatureRaw12 = readBits(33, 12);
    const uint16_t humidityRaw12 = readBits(45, 12);
    const uint8_t unknownB = static_cast<uint8_t>(readBits(57, 8));

    const float temperatureC =
        (static_cast<float>(temperatureRaw12 << 4) / 576.077364f) - 40.0f;
    const float humidityPercent =
        (static_cast<float>(humidityRaw12 << 4) / 51451.432435f) * 100.0f;

    if (temperatureC < -45.0f || temperatureC > 65.0f ||
        humidityPercent < 0.0f || humidityPercent > 105.0f) {
        return false;
    }

    Ft017ThSensorEntry& sensor = sensorEntryFor(candidateId);
    const uint32_t nowMs = millis();
    sensor.used = true;
    sensor.candidateId = candidateId;
    sensor.unknownA = unknownA;
    sensor.unknownB = unknownB;
    sensor.temperatureC = temperatureC;
    sensor.humidityPercent = humidityPercent;
    ++sensor.packetCount;
    sensor.lastSeenMs = nowMs;

    Serial.printf(
        "[CC1101][FT017TH] id=0x%03X temp=%.1f C humidity=%.1f %% "
        "| packets=%lu age=%lu s | rawT=%u rawH=%u unkA=0x%04X unkB=0x%02X\n",
        static_cast<unsigned>(candidateId),
        temperatureC,
        humidityPercent,
        static_cast<unsigned long>(sensor.packetCount),
        static_cast<unsigned long>((nowMs - sensor.firstSeenMs) / 1000UL),
        static_cast<unsigned>(temperatureRaw12),
        static_cast<unsigned>(humidityRaw12),
        static_cast<unsigned>(unknownA),
        static_cast<unsigned>(unknownB));

    RfSensorObservation observation;
    observation.protocol = "ft017th";
    observation.sensorId = candidateId;
    observation.channel = 0;
    observation.hasTemperature = true;
    observation.temperatureC = temperatureC;
    observation.hasHumidity = true;
    observation.humidityPercent =
        static_cast<int>(humidityPercent + 0.5f);
    observation.hasBattery = false;
    publishSensorObservation(observation);
    return true;
}

bool tryPrintRepeatedManchester(const int32_t* data, uint16_t count) {
    // Look for the longest clean run matching the ~500 us Manchester signal
    // seen during RF discovery. A logical bit consists of two half-bits;
    // equal adjacent half-bits are represented by a ~1000 us raw pulse.
    uint16_t bestStart = 0;
    uint16_t bestLength = 0;
    uint16_t runStart = 0;
    uint16_t runLength = 0;

    for (uint16_t i = 0; i < count; ++i) {
        const uint32_t duration =
            static_cast<uint32_t>(data[i] >= 0 ? data[i] : -data[i]);
        const bool plausible = duration >= 300 && duration <= 1250;

        if (plausible) {
            if (runLength == 0) runStart = i;
            ++runLength;
            if (runLength > bestLength) {
                bestStart = runStart;
                bestLength = runLength;
            }
        } else {
            runLength = 0;
        }
    }

    if (bestLength < 40) return false;

    static char halfBits[MaximumPulseCount * 2 + 2];
    uint16_t halfCount = 0;
    uint32_t shortTotal = 0;
    uint16_t shortCount = 0;
    uint32_t longTotal = 0;
    uint16_t longCount = 0;

    for (uint16_t i = bestStart;
         i < static_cast<uint16_t>(bestStart + bestLength);
         ++i) {
        const int32_t pulse = data[i];
        const char level = pulse >= 0 ? 'H' : 'L';
        const uint32_t duration =
            static_cast<uint32_t>(pulse >= 0 ? pulse : -pulse);

        uint8_t halfBitCount = 0;
        if (duration < 750) {
            halfBitCount = 1;
            shortTotal += duration;
            ++shortCount;
        } else {
            halfBitCount = 2;
            longTotal += duration;
            ++longCount;
        }

        for (uint8_t j = 0; j < halfBitCount; ++j) {
            if (halfCount >= sizeof(halfBits) - 1) return false;
            halfBits[halfCount++] = level;
        }
    }

    static char decoded[(MaximumPulseCount * 2) / 2 + 2];
    static char bestDecoded[(MaximumPulseCount * 2) / 2 + 2];
    uint16_t bestBitCount = 0;
    uint16_t bestInvalid = 0xFFFF;

    for (uint8_t offset = 0; offset <= 1; ++offset) {
        uint16_t bitCount = 0;
        uint16_t invalid = 0;

        for (uint16_t i = offset; i + 1 < halfCount; i += 2) {
            const char first = halfBits[i];
            const char second = halfBits[i + 1];
            if (first == 'H' && second == 'L') {
                decoded[bitCount++] = '1';
            } else if (first == 'L' && second == 'H') {
                decoded[bitCount++] = '0';
            } else {
                ++invalid;
                decoded[bitCount++] = '?';
            }
        }

        if (invalid < bestInvalid) {
            bestInvalid = invalid;
            bestBitCount = bitCount;
            for (uint16_t i = 0; i < bitCount; ++i) {
                bestDecoded[i] = decoded[i];
            }
        }
    }

    if (bestInvalid != 0 || bestBitCount < 32) return false;

    for (uint16_t i = 0; i < bestBitCount; ++i) {
        decoded[i] = bestDecoded[i];
    }
    decoded[bestBitCount] = '\0';

    uint16_t frameBits = 0;
    uint16_t fullRepeats = 0;
    uint16_t partialBits = 0;

    // A real RF burst often loses the first/last half-bit at carrier-sense
    // boundaries. Do not require the decoded length to be an exact multiple
    // of the frame size. Accept two complete identical frames plus an
    // identical partial repeat.
    for (uint16_t period = 16; period <= bestBitCount / 2; ++period) {
        const uint16_t candidateFullRepeats = bestBitCount / period;
        const uint16_t candidatePartialBits = bestBitCount % period;
        if (candidateFullRepeats < 2) continue;

        bool identical = true;
        for (uint16_t i = period; i < bestBitCount && identical; ++i) {
            if (decoded[i] != decoded[i % period]) {
                identical = false;
            }
        }

        if (identical) {
            frameBits = period;
            fullRepeats = candidateFullRepeats;
            partialBits = candidatePartialBits;
            break;
        }
    }

    if (fullRepeats < 2 || frameBits == 0) return false;

    const uint32_t shortAverage =
        shortCount ? shortTotal / shortCount : 0;
    const uint32_t longAverage =
        longCount ? longTotal / longCount : 0;

    Serial.printf(
        "[CC1101][MC] frame=%u bits repeats=%u",
        static_cast<unsigned>(frameBits),
        static_cast<unsigned>(fullRepeats));
    if (partialBits > 0) {
        Serial.printf(
            "+%u/%u",
            static_cast<unsigned>(partialBits),
            static_cast<unsigned>(frameBits));
    }
    Serial.printf(
        " | half~%lu us double~%lu us | bits=",
        static_cast<unsigned long>(shortAverage),
        static_cast<unsigned long>(longAverage));

    for (uint16_t i = 0; i < frameBits; ++i) {
        Serial.print(decoded[i]);
    }

    Serial.print(" | hex=");
    const uint16_t fullBytes = frameBits / 8;
    for (uint16_t byteIndex = 0; byteIndex < fullBytes; ++byteIndex) {
        uint8_t value = 0;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            value <<= 1;
            if (decoded[byteIndex * 8 + bit] == '1') value |= 1;
        }
        if (byteIndex > 0) Serial.print(' ');
        if (value < 0x10) Serial.print('0');
        Serial.print(value, HEX);
    }
    if ((frameBits % 8) != 0) {
        Serial.print(" +");
        for (uint16_t i = fullBytes * 8; i < frameBits; ++i) {
            Serial.print(decoded[i]);
        }
    }

    Serial.println();
    tryPrintFt017Th(decoded, frameBits);
    return true;
}

void printBurst(
    const int32_t* data,
    uint16_t count,
    bool wasOverflowed,
    uint16_t csHighEdges,
    uint16_t csHoldEdges) {
    constexpr uint16_t FullRawPrintLimit = 160;

    if (wasOverflowed || count > FullRawPrintLimit) {
        uint32_t minUs = 0xFFFFFFFFUL;
        uint32_t maxUs = 0;
        uint32_t totalUs = 0;
        uint16_t shortCount = 0;
        for (uint16_t i = 0; i < count; ++i) {
            const uint32_t us = static_cast<uint32_t>(
                data[i] >= 0 ? data[i] : -data[i]);
            if (us < minUs) minUs = us;
            if (us > maxUs) maxUs = us;
            totalUs += us;
            if (us < 400) ++shortCount;
        }

        uint16_t tfaPairs = 0;
        uint16_t nexusPairs = 0;
        uint16_t plausiblePairs = 0;
        for (uint16_t i = 0; i + 1 < count; ++i) {
            if (data[i] <= 0 || data[i + 1] >= 0) continue;

            const uint32_t highUs = static_cast<uint32_t>(data[i]);
            const uint32_t lowUs = static_cast<uint32_t>(-data[i + 1]);
            if (highUs < 300 || highUs > 800) continue;

            ++plausiblePairs;
            if ((lowUs >= 1500 && lowUs <= 2800) ||
                (lowUs >= 3200 && lowUs <= 5000)) {
                ++tfaPairs;
            }
            if ((lowUs >= 700 && lowUs <= 1400) ||
                (lowUs >= 1600 && lowUs <= 2500)) {
                ++nexusPairs;
            }
        }

        const uint32_t csTotal =
            static_cast<uint32_t>(csHighEdges) +
            static_cast<uint32_t>(csHoldEdges);
        const double csPercent =
            csTotal
                ? 100.0 * static_cast<double>(csHighEdges) /
                      static_cast<double>(csTotal)
                : 0.0;
        const double tfaScore =
            plausiblePairs
                ? 100.0 * static_cast<double>(tfaPairs) /
                      static_cast<double>(plausiblePairs)
                : 0.0;
        const double nexusScore =
            plausiblePairs
                ? 100.0 * static_cast<double>(nexusPairs) /
                      static_cast<double>(plausiblePairs)
                : 0.0;

        const char* shape = "mixed";
        double shapeScore = 0.0;
        if (plausiblePairs >= 24 && tfaScore >= 70.0) {
            shape = "TFA-2/4ms?";
            shapeScore = tfaScore;
        } else if (plausiblePairs >= 24 && nexusScore >= 70.0) {
            shape = "NEXUS-1/2ms?";
            shapeScore = nexusScore;
        }

        Serial.printf(
            "[CC1101][RX][LONG] pulses=%u%s min=%lu us max=%lu us avg=%lu us "
            "short<400=%u (%.0f%%) cs=%u/%lu (%.0f%%) shape=%s",
            static_cast<unsigned>(count),
            wasOverflowed ? " OVERFLOW" : "",
            static_cast<unsigned long>(minUs == 0xFFFFFFFFUL ? 0 : minUs),
            static_cast<unsigned long>(maxUs),
            static_cast<unsigned long>(count ? totalUs / count : 0),
            static_cast<unsigned>(shortCount),
            count
                ? 100.0 * static_cast<double>(shortCount) /
                      static_cast<double>(count)
                : 0.0,
            static_cast<unsigned>(csHighEdges),
            static_cast<unsigned long>(csTotal),
            csPercent,
            shape);

        if (shapeScore > 0.0) {
            Serial.printf(" score=%.0f%%", shapeScore);
        }

        // For protocol discovery keep a bounded raw prefix even for long
        // captures. Repeating weather-sensor packets normally expose their
        // timing signature within the first few dozen edges, while limiting
        // the dump avoids multi-kilobyte serial spam.
        constexpr uint16_t FingerprintPulseCount = 96;
        const uint16_t fingerprintCount =
            min<uint16_t>(count, FingerprintPulseCount);
        Serial.print(" | head=");
        for (uint16_t i = 0; i < fingerprintCount; ++i) {
            if (i > 0) Serial.print(' ');
            const int32_t pulse = data[i];
            Serial.print(pulse >= 0 ? 'H' : 'L');
            Serial.print(static_cast<unsigned long>(
                pulse >= 0 ? pulse : -pulse));
        }

        if (count > fingerprintCount) {
            Serial.printf(
                " | omitted=%u",
                static_cast<unsigned>(count - fingerprintCount));
        }
        Serial.println();
        return;
    }

    Serial.printf(
        "[CC1101][RX] burst pulses=%u | ",
        static_cast<unsigned>(count));

    for (uint16_t i = 0; i < count; ++i) {
        const int32_t pulse = data[i];
        Serial.print(pulse >= 0 ? 'H' : 'L');
        Serial.print(static_cast<unsigned long>(pulse >= 0 ? pulse : -pulse));
        if (i + 1 < count) {
            Serial.print(' ');
        }

        if ((i + 1) % 32 == 0 && i + 1 < count) {
            Serial.println();
            Serial.print("[CC1101][RX]   ");
        }
    }
    Serial.println();
}

} // namespace

void onSensorObservation(SensorObservationCallback callback) {
    sensorObservationCallback = callback;
}

bool begin() {
    receiverReady = false;
    captureSuppressed = false;
    for (auto& sensor : ft017ThSensors) {
        sensor = Ft017ThSensorEntry{};
    }
    pulseCount = 0;
    lastEdgeUs = 0;
    lastActivityUs = 0;
    lastCarrierSeenUs = 0;
    carrierHighEdges = 0;
    carrierHoldEdges = 0;
    overflowed = false;

    pinMode(CC1101_CS_PIN, OUTPUT);
    digitalWrite(CC1101_CS_PIN, HIGH);
    pinMode(CC1101_GDO0_PIN, INPUT);
    pinMode(CC1101_GDO2_PIN, INPUT);

    SPI.begin(
        CC1101_SCK_PIN,
        CC1101_MISO_PIN,
        CC1101_MOSI_PIN,
        CC1101_CS_PIN);

    const SPISettings settings(SpiFrequencyHz, MSBFIRST, SPI_MODE0);

    const bool ok =
        resetChip(settings) &&
        configureReceiver(settings) &&
        strobe(Srx, settings);

    // CC1101 now stays autonomously in RX. SPI is released so the e-paper
    // driver can remap the global SPI object's MISO pin later.
    SPI.end();

    if (!ok) {
        Serial.println("[CC1101][RX] CHYBA: konfigurace prijimace selhala.");
        return false;
    }

    attachInterrupt(
        digitalPinToInterrupt(CC1101_GDO0_PIN),
        onRawEdge,
        CHANGE);

    receiverReady = true;
    Serial.println(
        "[CC1101][RX] RAW prijem aktivni: 433.92 MHz ASK/OOK, "
        "GDO0=data, GDO2=carrier-sense.");
    Serial.println(
        "[CC1101][RX] Cekam na RF bursty; vypis bude H/L + delka pulzu v us.");
    return true;
}

void loop() {
    if (!receiverReady) {
        return;
    }

    const uint32_t now = micros();

    noInterrupts();
    const uint16_t count = pulseCount;
    const uint32_t last = lastActivityUs;
    interrupts();

    if (count < MinimumBurstPulses ||
        last == 0 ||
        static_cast<uint32_t>(now - last) < BurstGapUs) {
        return;
    }

    static int32_t snapshot[MaximumPulseCount];
    uint16_t snapshotCount = 0;
    bool snapshotOverflow = false;
    uint16_t snapshotCarrierHighEdges = 0;
    uint16_t snapshotCarrierHoldEdges = 0;

    noInterrupts();
    snapshotCount = pulseCount;
    if (snapshotCount > MaximumPulseCount) {
        snapshotCount = MaximumPulseCount;
    }
    for (uint16_t i = 0; i < snapshotCount; ++i) {
        snapshot[i] = pulses[i];
    }
    snapshotOverflow = overflowed;
    snapshotCarrierHighEdges = carrierHighEdges;
    snapshotCarrierHoldEdges = carrierHoldEdges;
    pulseCount = 0;
    lastEdgeUs = 0;
    lastActivityUs = 0;
    carrierHighEdges = 0;
    carrierHoldEdges = 0;
    overflowed = false;
    interrupts();

    if (!tryPrintTfaTwinPlus(snapshot, snapshotCount) &&
        !tryPrintNexusTh(snapshot, snapshotCount) &&
        !tryPrintHyundaiWs(snapshot, snapshotCount) &&
        !tryPrintAuriolHg02832(snapshot, snapshotCount) &&
        !tryPrintRepeatedManchester(snapshot, snapshotCount) &&
        !tryPrintPwm67(snapshot, snapshotCount) &&
        !tryPrintPwm67Candidate(snapshot, snapshotCount)) {
        printBurst(
            snapshot,
            snapshotCount,
            snapshotOverflow,
            snapshotCarrierHighEdges,
            snapshotCarrierHoldEdges);
    }
}

void setSuppressed(bool suppressed) {
    bool changed = false;

    noInterrupts();
    if (captureSuppressed != suppressed) {
        captureSuppressed = suppressed;
        pulseCount = 0;
        lastEdgeUs = 0;
        lastActivityUs = 0;
        carrierHighEdges = 0;
        carrierHoldEdges = 0;
        overflowed = false;
        changed = true;
    }
    interrupts();

    if (changed) {
        Serial.printf(
            "[CC1101][RX] capture %s kvuli aktivite e-paperu.\n",
            suppressed ? "PAUSED" : "RESUMED");
    }
}

bool isReady() {
    return receiverReady;
}

} // namespace Cc1101RawReceiver
