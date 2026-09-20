#pragma once

namespace Cc1101Diagnostics {

// Initializes the CC1101 only long enough to verify SPI communication,
// prints the result to Serial, then releases the SPI bus again.
// Returns true when the expected CC1101 identification registers are readable.
bool probe();

}
