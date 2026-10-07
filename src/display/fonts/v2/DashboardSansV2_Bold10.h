#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Bold10BitmapBase64[] PROGMEM =
    "PAB+AGMAQwDDAMMAQwBjAH4APAAw8PAwMDAwMDAwPH5GxgYMGDB+/jx+YgYcDgNDfjwOAA4AHgA2ADYAZgD/AP8ABgAGAH5+QFx+"
    "RgLGfjw8fmJA/ubDY348/v4MDAgYGDAwYDx+ZmY8fsLDZjw8fsbDZ38CRn48YEAAYEDAwBgAGAB/AH8AGAAYABgAeAAAAABAYEAA"
    "AGBAMMBIgEkAewACAATgDaAJIBGgMOBgYGBubHh4eGxmxxjHOGcwZTBtsG2gPeA44DjgOMBgYGB8fmZmZmZmfn5gYH5+YGBgYMGA"
    "YYBjAGMAMwA2ADYAFgAcABwAfn5gYH5+YGB+fnwAfwBjAGGAYYBhgGGAYwB/AHwAfH5mZmZmZjhsxv7Abjx4bGB8DGx4";
static const Glyph Bold10Glyphs[] PROGMEM = {
    {32,0,0,3,0,10,0},{48,9,10,9,0,0,0},{49,5,10,9,0,0,20},{50,8,10,9,0,0,30},{51,8,10,9,0,0,40},
    {52,9,10,9,0,0,50},{53,8,10,9,0,0,70},{54,8,10,9,0,0,80},{55,7,10,9,0,0,90},{56,8,10,9,0,0,100},
    {57,8,10,9,0,0,110},{46,3,3,3,0,7,120},{44,3,4,3,0,8,123},{43,9,7,9,0,3,127},{45,6,5,6,0,5,141},
    {58,3,7,3,0,3,146},{37,12,10,12,0,0,153},{107,8,10,7,0,0,173},{87,13,10,13,0,0,183},{104,8,10,8,0,0,203},
    {70,8,10,8,0,0,213},{86,10,10,9,0,0,223},{69,8,10,8,0,0,243},{68,9,10,9,0,0,253},{110,8,7,8,0,3,273},
    {101,7,7,7,0,3,280},{115,7,7,7,0,3,287},
};
static const Face Bold10Face = {10,13,Bold10Glyphs,27,Bold10BitmapBase64};
} // namespace DashboardSansV2
