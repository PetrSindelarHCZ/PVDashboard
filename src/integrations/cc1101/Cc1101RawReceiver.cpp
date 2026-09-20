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
constexpr uint32_t BurstGapUs = 12000;
constexpr uint16_t MinimumBurstPulses = 8;
constexpr uint16_t MaximumPulseCount = 768;

volatile int32_t pulses[MaximumPulseCount];
volatile uint16_t pulseCount = 0;
volatile uint32_t lastEdgeUs = 0;
volatile uint32_t lastActivityUs = 0;
volatile bool overflowed = false;
volatile bool receiverReady = false;
volatile bool captureSuppressed = false;

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

    // Ignore raw slicer chatter until CC1101 declares a carrier.
    if (digitalRead(CC1101_GDO2_PIN) == LOW) {
        // Break pulse timing across periods without carrier, but keep the
        // timestamp of the last real burst activity so loop() can close it.
        lastEdgeUs = 0;
        return;
    }

    const uint32_t now = micros();
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

    const uint16_t temperatureRaw12 = readBits(33, 12);
    const uint16_t humidityRaw12 = readBits(45, 12);

    const float temperatureC =
        (static_cast<float>(temperatureRaw12 << 4) / 576.077364f) - 40.0f;
    const float humidityPercent =
        (static_cast<float>(humidityRaw12 << 4) / 51451.432435f) * 100.0f;

    if (temperatureC < -45.0f || temperatureC > 65.0f ||
        humidityPercent < 0.0f || humidityPercent > 105.0f) {
        return false;
    }

    Serial.printf(
        "[CC1101][FT017TH] temp=%.1f C humidity=%.1f %% | rawT=%u rawH=%u\n",
        temperatureC,
        humidityPercent,
        static_cast<unsigned>(temperatureRaw12),
        static_cast<unsigned>(humidityRaw12));
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

void printBurst(const int32_t* data, uint16_t count, bool wasOverflowed) {
    Serial.printf(
        "[CC1101][RX] burst pulses=%u%s | ",
        static_cast<unsigned>(count),
        wasOverflowed ? " OVERFLOW" : "");

    for (uint16_t i = 0; i < count; ++i) {
        const int32_t pulse = data[i];
        Serial.print(pulse >= 0 ? 'H' : 'L');
        Serial.print(static_cast<unsigned long>(pulse >= 0 ? pulse : -pulse));
        if (i + 1 < count) {
            Serial.print(' ');
        }

        // Keep serial lines reasonably sized and readable.
        if ((i + 1) % 32 == 0 && i + 1 < count) {
            Serial.println();
            Serial.print("[CC1101][RX]   ");
        }
    }
    Serial.println();
}

} // namespace

bool begin() {
    receiverReady = false;
    captureSuppressed = false;
    pulseCount = 0;
    lastEdgeUs = 0;
    lastActivityUs = 0;
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

    noInterrupts();
    snapshotCount = pulseCount;
    if (snapshotCount > MaximumPulseCount) {
        snapshotCount = MaximumPulseCount;
    }
    for (uint16_t i = 0; i < snapshotCount; ++i) {
        snapshot[i] = pulses[i];
    }
    snapshotOverflow = overflowed;
    pulseCount = 0;
    lastEdgeUs = 0;
    lastActivityUs = 0;
    overflowed = false;
    interrupts();

    if (!tryPrintRepeatedManchester(snapshot, snapshotCount)) {
        printBurst(snapshot, snapshotCount, snapshotOverflow);
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
