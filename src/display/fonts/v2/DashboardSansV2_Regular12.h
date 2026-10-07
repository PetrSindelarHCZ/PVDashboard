#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Regular12BitmapBase64[] PROGMEM =
"eERERERERHhwSEhISEhwiPiASHBwkEAwkHBwSIgIECBA+HhICDAIhEh4wAAYGChISPwICEBAUFBgYFBIiICMgFSAVIBVAFMAYwAjAEBAcEhISEhI";
static const Glyph Regular12Glyphs[] PROGMEM = {
    {32,0,0,2,0,0,0},{68,7,8,7,0,2,0},{110,5,6,5,0,4,8},{101,6,6,5,0,4,14},
    {115,5,6,5,0,4,20},{50,6,8,6,0,2,26},{51,6,8,6,0,2,34},{46,2,2,2,0,8,42},
    {52,6,8,6,0,2,44},{107,6,8,5,0,2,52},{87,10,8,10,0,2,60},{104,5,8,5,0,2,76},
};
static const Face Regular12Face = {12,13,Regular12Glyphs,12,Regular12BitmapBase64};
} // namespace DashboardSansV2
