#pragma once
#include <stdint.h>

// Owned by the application loop; the renderer only reads the resulting level.
class WifiSignalLevel {
public:
    uint8_t update(bool connected, int16_t rssi, uint32_t now) {
        if (!connected) {
            _level = _candidate = 0;
            return 0;
        }
        if (_level == 0) {
            _level = _candidate = rssi >= -67 ? 3 : (rssi >= -75 ? 2 : 1);
            return _level;
        }
        uint8_t next = _level;
        // 3 dB hysteresis on each boundary, plus 5 seconds of stability.
        if (_level == 3 && rssi < -70) next = rssi < -78 ? 1 : 2;
        if (_level == 2) {
            if (rssi >= -64) next = 3;
            else if (rssi < -78) next = 1;
        }
        if (_level == 1 && rssi >= -72) next = rssi >= -64 ? 3 : 2;
        if (next == _level) {
            _candidate = _level;
        } else if (next != _candidate) {
            _candidate = next;
            _candidateSince = now;
        } else if (static_cast<uint32_t>(now - _candidateSince) >= 5000U) {
            _level = next;
        }
        return _level;
    }
private:
    uint8_t _level = 0;
    uint8_t _candidate = 0;
    uint32_t _candidateSince = 0;
};
