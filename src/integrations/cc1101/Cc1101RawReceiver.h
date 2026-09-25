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

// Temporarily discard RF captures while the e-paper is electrically active.
// This helps distinguish real 433 MHz traffic from local display interference.
void setSuppressed(bool suppressed);

bool isReady();

}
