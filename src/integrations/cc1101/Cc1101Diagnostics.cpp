#include "Cc1101Diagnostics.h"

#include <Arduino.h>
#include <SPI.h>
#include "../../../include/AppConfig.h"

namespace Cc1101Diagnostics {
namespace {

constexpr uint8_t SresStrobe = 0x30;
constexpr uint8_t PartnumRegister = 0x30;
constexpr uint8_t VersionRegister = 0x31;
constexpr uint8_t ReadBurst = 0xC0;
constexpr uint32_t ChipReadyTimeoutUs = 10000;
constexpr uint32_t SpiFrequencyHz = 4000000;

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

bool resetChip(const SPISettings& settings) {
    // CC1101 reset sequence from the datasheet. SO/MISO goes LOW once the
    // crystal oscillator is stable and the chip is ready for an SPI command.
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

    SPI.transfer(SresStrobe);

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

bool readStatusRegister(uint8_t address, uint8_t& value, const SPISettings& settings) {
    SPI.beginTransaction(settings);
    digitalWrite(CC1101_CS_PIN, LOW);

    if (!waitForChipReady()) {
        digitalWrite(CC1101_CS_PIN, HIGH);
        SPI.endTransaction();
        return false;
    }

    // Status registers 0x30-0x3D require both READ and BURST bits set.
    SPI.transfer(address | ReadBurst);
    value = SPI.transfer(0x00);

    digitalWrite(CC1101_CS_PIN, HIGH);
    SPI.endTransaction();
    return true;
}

} // namespace

bool probe() {
    Serial.printf(
        "[CC1101] SPI SCK=%u MISO=%u MOSI=%u CS=%u | GDO0=%u GDO2=%u\n",
        static_cast<unsigned>(CC1101_SCK_PIN),
        static_cast<unsigned>(CC1101_MISO_PIN),
        static_cast<unsigned>(CC1101_MOSI_PIN),
        static_cast<unsigned>(CC1101_CS_PIN),
        static_cast<unsigned>(CC1101_GDO0_PIN),
        static_cast<unsigned>(CC1101_GDO2_PIN));

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

    uint8_t partnum = 0xFF;
    uint8_t version = 0xFF;

    const bool resetOk = resetChip(settings);
    const bool partnumOk = resetOk &&
        readStatusRegister(PartnumRegister, partnum, settings);
    const bool versionOk = partnumOk &&
        readStatusRegister(VersionRegister, version, settings);

    // The startup probe is intentionally temporary. The display later
    // reinitializes the shared global SPI object with its own MISO pin.
    SPI.end();

    if (!resetOk) {
        Serial.println("[CC1101] CHYBA: modul po resetu nepotvrdil chip-ready na MISO.");
        return false;
    }

    if (!partnumOk || !versionOk) {
        Serial.println("[CC1101] CHYBA: identifikacni registry nelze precist.");
        return false;
    }

    Serial.printf(
        "[CC1101] PARTNUM=0x%02X VERSION=0x%02X\n",
        static_cast<unsigned>(partnum),
        static_cast<unsigned>(version));

    const bool detected =
        partnum == 0x00 &&
        version != 0x00 &&
        version != 0xFF;

    if (detected) {
        Serial.println("[CC1101] OK: CC1101 detekovan pres SPI.");
    } else {
        Serial.println("[CC1101] CHYBA: odpoved neodpovida platnemu CC1101.");
    }

    return detected;
}

} // namespace Cc1101Diagnostics
