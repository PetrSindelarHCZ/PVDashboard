// Run: g++ -std=c++11 tests/native/wifi_signal_level.cpp -o /tmp/wifi-test && /tmp/wifi-test
#include "../../src/network/WifiSignalLevel.h"
#include <cassert>
int main() {
    WifiSignalLevel s;
    assert(s.update(false, -60, 0) == 0);
    assert(s.update(true, -67, 1) == 3);
    assert(s.update(true, -70, 100) == 3); // hysteresis dead band
    assert(s.update(true, -71, 200) == 3);
    assert(s.update(true, -71, 5199) == 3);
    assert(s.update(true, -71, 5200) == 2);
    assert(s.update(true, -67, 5300) == 2);
    assert(s.update(true, -64, 5400) == 2);
    assert(s.update(true, -65, 5500) == 2); // cancelled candidate
    assert(s.update(true, -64, 11000) == 2);
    assert(s.update(true, -64, 16000) == 3);
    assert(s.update(true, -85, 17000) == 3);
    assert(s.update(true, -85, 22000) == 1); // direct multi-level change
    assert(s.update(true, -73, 23000) == 1);
    assert(s.update(true, -72, 24000) == 1);
    assert(s.update(true, -72, 29000) == 2);
    assert(s.update(true, -79, 30000) == 2);
    assert(s.update(false, -79, 30001) == 0); // no disconnect debounce
    assert(s.update(true, -80, 30002) == 1); // fresh reconnect classification
    assert(s.update(false, 0, 30003) == 0);
    assert(s.update(true, -75, 30004) == 2);
    assert(s.update(false, 0, 30005) == 0);
    assert(s.update(true, -76, 30006) == 1);
    assert(s.update(true, -60, UINT32_MAX - 1000) == 1);
    assert(s.update(true, -60, 3998) == 1);
    assert(s.update(true, -60, 3999) == 3); // millis rollover
}
