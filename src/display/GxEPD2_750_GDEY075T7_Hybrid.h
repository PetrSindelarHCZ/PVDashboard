#pragma once

#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_750_GDEY075T7.h>

// Hybrid update strategy for the Waveshare 7.5" V2 panel identified by
// FPC-8612 (GDEY075T7 / UC8179 family).
//
// Large changes use the panel's OTP partial waveform. On this panel revision
// it is slow, but gives clean black/white output without vertical banding.
//
// Small regions use GxEPD2's register-based differential waveform. It is much
// faster and testing showed only light ghosting for navigation-sized changes.
class GxEPD2_750_GDEY075T7_Hybrid : public GxEPD2_750_GDEY075T7 {
public:
    using GxEPD2_750_GDEY075T7::GxEPD2_750_GDEY075T7;
    using GxEPD2_750_GDEY075T7::refresh;
    using GxEPD2_750_GDEY075T7::writeImageForFullRefresh;
    using GxEPD2_750_GDEY075T7::writeImageToPrevious;

    void writeImage(
        const uint8_t bitmap[],
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert = false, bool mirror_y = false, bool pgm = false) {

        if (useRegisterPartialForRegion(w, h)) {
            ensureRegisterPartialMode();
        } else {
            ensureOtpPartialMode();
        }

        GxEPD2_750_GDEY075T7::writeImage(
            bitmap, x, y, w, h, invert, mirror_y, pgm);
    }

    void powerOff() {
        GxEPD2_750_GDEY075T7::powerOff();
        _hybridMode = HybridMode::Unknown;
    }

private:
    enum class HybridMode : uint8_t {
        Unknown,
        Otp,
        Register
    };

    HybridMode _hybridMode = HybridMode::Unknown;

    static constexpr uint8_t T1 = 30;
    static constexpr uint8_t T2 = 5;
    static constexpr uint8_t T3 = 30;
    static constexpr uint8_t T4 = 5;

    // One quarter of the 800x480 panel. Sidebar, header and focus rectangles
    // stay below this limit; page/body/screen changes use the clean OTP path.
    static constexpr uint32_t RegisterPartialMaxPixels =
        (uint32_t(WIDTH) * uint32_t(HEIGHT)) / 4;

    static bool useRegisterPartialForRegion(int16_t w, int16_t h) {
        if (w <= 0 || h <= 0) return false;
        return uint32_t(w) * uint32_t(h) <= RegisterPartialMaxPixels;
    }

    void writeShortLut(uint8_t command, uint8_t phase) {
        _writeCommand(command);
        _writeData(phase);
        _writeData(T1);
        _writeData(T2);
        _writeData(T3);
        _writeData(T4);
        _writeData(1);
        for (uint8_t i = 6; i < 42; ++i) _writeData(0x00);
    }

    void ensureOtpPartialMode() {
        if (_hybridMode == HybridMode::Otp && _using_partial_mode) return;

        // A register LUT can remain active while _using_partial_mode is true.
        // Explicitly end that mode before asking the upstream driver to load
        // the OTP partial waveform.
        if (_power_is_on || _using_partial_mode) {
            GxEPD2_750_GDEY075T7::powerOff();
        }

        _Init_Part();
        _hybridMode = HybridMode::Otp;
    }

    void ensureRegisterPartialMode() {
        if (_hybridMode == HybridMode::Register && _using_partial_mode) return;

        // Likewise, do not assume that _using_partial_mode means the desired
        // waveform is already loaded: it may currently be the OTP waveform.
        if (_power_is_on || _using_partial_mode) {
            GxEPD2_750_GDEY075T7::powerOff();
        }

        if (_hibernating) _reset();

        // Same controller base setup as GxEPD2_750_GDEY075T7::_InitDisplay().
        _writeCommand(0x00);
        _writeData(0x1F);

        _writeCommand(0x01);
        _writeData(0x07);
        _writeData(0x07);
        _writeData(0x3F);
        _writeData(0x3F);
        _writeData(0x09);

        _writeCommand(0x06);
        _writeData(0x17);
        _writeData(0x17);
        _writeData(0x28);
        _writeData(0x17);

        _writeCommand(0x61);
        _writeData(WIDTH / 256);
        _writeData(WIDTH % 256);
        _writeData(HEIGHT / 256);
        _writeData(HEIGHT % 256);

        _writeCommand(0x15);
        _writeData(0x00);

        _writeCommand(0x50);
        _writeData(0x29);
        _writeData(0x07);

        _writeCommand(0x60);
        _writeData(0x22);

        _writeCommand(0xE3);
        _writeData(0x22);

        // GxEPD2 register-based partial branch for earlier GDEY075T7 batches.
        _writeCommand(0x00);
        _writeData(0x3F);

        _writeCommand(0x82);
        _writeData(0x30);

        _writeCommand(0x50);
        _writeData(0x39);
        _writeData(0x07);

        writeShortLut(0x20, 0x00); // LUTC
        writeShortLut(0x21, 0x00); // white -> white
        writeShortLut(0x22, 0x5A); // black -> white, upstream "more white"
        writeShortLut(0x23, 0x84); // white -> black
        writeShortLut(0x24, 0x00); // black -> black
        writeShortLut(0x25, 0x00); // border

        if (!_power_is_on) {
            _writeCommand(0x04);
            _waitWhileBusy("GDEY_RegisterPartial_PowerOn", 140);
            _power_is_on = true;
        }

        _using_partial_mode = true;
        _hybridMode = HybridMode::Register;
    }
};
