#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Bold8BitmapBase64[] PROGMEM =
    "OEzExMTETDhg4KAgICAgIHhIzAgYMGD8eEwMMAzMTHgYGChoSPwICHhAwPjMDMh4OEzA+MzMTDj4CBgQMCBgQHhMSDBMzMx4eEjM"
    "zHwMSHhAwEDAgBAQfBAQAHAAAADAQABAwAByAFIAVABsAAsAFIAUgCMAwMDY0PDw2MjMwMzATIBegHeAc4AzADMAwMD4yMjIyMh8"
    "QEB4QEBAQMZGRGxsKDg4fEBAeEBAQHx4TEZGRkZMePjIyMjIyHBI+MBIcHDYYDjYcAA=";
static const Glyph Bold8Glyphs[] PROGMEM = {
    {32,0,0,2,0,8,0},{48,7,8,7,0,0,0},{49,4,8,7,0,0,8},{50,6,8,7,0,0,16},{51,6,8,7,0,0,24},
    {52,7,8,7,0,0,32},{53,6,8,7,0,0,40},{54,6,8,7,0,0,48},{55,6,8,7,0,0,56},{56,6,8,7,0,0,64},
    {57,6,8,7,0,0,72},{46,3,2,2,0,6,80},{44,2,3,2,0,7,82},{43,7,6,7,0,2,85},{45,4,4,4,0,4,91},
    {58,3,6,2,0,2,95},{37,9,8,9,0,0,101},{107,6,8,6,0,0,117},{87,10,8,10,0,0,125},{104,6,8,6,0,0,141},
    {70,6,8,6,0,0,149},{86,7,8,7,0,0,157},{69,6,8,6,0,0,165},{68,7,8,7,0,0,173},{110,6,6,6,0,2,181},
    {101,6,6,6,0,2,187},{115,5,7,5,0,2,193},
};
static const Face Bold8Face = {8,11,Bold8Glyphs,27,Bold8BitmapBase64};
} // namespace DashboardSansV2
