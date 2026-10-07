#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Regular16BitmapBase64[] PROGMEM =
"fABCAEEAQQBBAEEAQQBBAEIAfAB4REREREREOERE/kBEOHhIQDgMTHg4REICBAwYMGB+PEZCBhgGAkJGPEBABAwUFCRkRP8EBEBAQERIUHBYSESGEEYQRjBFIEkgKSApYCjAMMAQwEBAQHhEREREREQ=";
static const Glyph Regular16Glyphs[] PROGMEM = {
    {32,0,0,3,0,0,0},{68,9,10,9,0,3,0},{110,7,7,7,0,6,20},{101,7,7,7,0,6,27},
    {115,6,7,6,0,6,34},{50,7,10,7,0,3,41},{51,8,10,8,0,3,51},{46,3,2,3,0,11,61},
    {52,8,10,8,0,3,63},{107,7,10,7,0,3,73},{87,13,10,12,0,3,83},{104,7,10,7,0,3,103},
};
static const Face Regular16Face = {16,17,Regular16Glyphs,12,Regular16BitmapBase64};
} // namespace DashboardSansV2
