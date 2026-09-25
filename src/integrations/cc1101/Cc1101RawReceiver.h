#pragma once

#include <functional>
#include "RfSensorTypes.h"

namespace Cc1101RawReceiver {

using SensorObservationCallback =
    std::function<void(const RfSensorObservation& observation)>;

void onSensorObservation(SensorObservationCallback callback);

// Configures CC1101 for 433.92 MHz ASK/OOK asynchronous receive.
// GDO0 carries raw demodulated data, GDO2 is carrier sense.
bool begin();

// Call frequently from DashboardApp::loop(). Completed bursts are printed
// as H/L pulse lengths in microseconds.
void loop();

// Temporarily suspend RF capture while the e-paper is electrically active.
// The GDO0 interrupt is detached so RF/display chatter cannot pre-empt SPI.
void setSuppressed(bool suppressed);

bool isReady();

}
