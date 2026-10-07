#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Regular8BitmapBase64[] PROGMEM =
    "MEiEhISESDBgoCAgICAgIHBIiAgQIED4eEgIMAiESHgYGChISPwICHiAgLDICEhwOEiAuMiISHD4CBAQICBAQHBISDBIiEh4cEiI"
    "SHgISHDAAICAABAQeBAQAHAAAMAAAADAAGJUVCgSFSVGQEBQUGBgUEiIgIyAVIBUgFUAUwBjACMAQEBwSEhISEh4QEB4QEBAQIRE"
    "REhIKDAQeEBAeEBAQHh4REREREREeHBISEhISHCI+IBIcHCQQDCQcA==";
static const Glyph Regular8Glyphs[] PROGMEM = {
    {32,0,0,2,0,8,0},{48,6,8,6,0,0,0},{49,4,8,6,0,0,8},{50,6,8,6,0,0,16},{51,6,8,6,0,0,24},
    {52,6,8,6,0,0,32},{53,6,8,6,0,0,40},{54,6,8,6,0,0,48},{55,5,8,6,0,0,56},{56,6,8,6,0,0,64},
    {57,6,8,6,0,0,72},{46,2,2,2,0,6,80},{44,2,3,2,0,7,82},{43,6,6,6,0,2,85},{45,4,3,4,0,5,91},
    {58,2,6,2,0,2,94},{37,8,8,8,0,0,100},{107,6,8,5,0,0,108},{87,10,8,10,0,0,116},{104,5,8,5,0,0,132},
    {70,6,8,6,0,0,140},{86,7,8,7,0,0,148},{69,6,8,6,0,0,156},{68,7,8,7,0,0,164},{110,5,6,5,0,2,172},
    {101,6,6,5,0,2,178},{115,5,6,5,0,2,184},
};
static const Face Regular8Face = {8,11,Regular8Glyphs,27,Regular8BitmapBase64};
} // namespace DashboardSansV2
