#pragma once

namespace Cc1101RawReceiver {

// Configures CC1101 for 433.92 MHz ASK/OOK asynchronous receive.
// GDO0 carries raw demodulated data, GDO2 is carrier sense.
bool begin();

// Call frequently from DashboardApp::loop(). Completed bursts are printed
// as H/L pulse lengths in microseconds.
void loop();

bool isReady();

}
