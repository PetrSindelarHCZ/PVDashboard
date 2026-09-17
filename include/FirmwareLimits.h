#pragma once

#include <stddef.h>

namespace FirmwareLimits {
constexpr size_t MaxImageBytes = 0x1E0000; // Velikost app0/app1 v min_spiffs.csv (1.875 MiB).
}
