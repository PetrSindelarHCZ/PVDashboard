#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Regular8BitmapBase64[] PROGMEM =
    "YJCQkJBgAEBAQEBAQABgkCAgQPAAYJAgEJBgACBgYKDwIADggOAQkGAAYIDgkJBgAOAgIEBAgABgkGCQkGAAYJCQcBBgAAAAAAAA"
    "gAAAAAAAAICAAABAcEAAAAAAAOAAAADIsNAgWFgAAAAAAACAAICAoMDAoACUlLS0bEgAgIDgoKCgAPCA8ICAgACQkJBgYCAA8IDw"
    "gIDwAPCQkJCQ8AAAAOCgoKAAAABgoMBgAAAAYIBg4AA=";
static const Glyph Regular8Glyphs[] PROGMEM = {
    {32,0,0,2,0,0,0},{48,4,7,4,0,0,0},{49,3,7,3,0,0,7},{50,4,7,4,0,0,14},{51,4,7,4,0,0,21},{52,5,7,4,0,0,28},{53,4,7,4,0,0,35},{54,4,7,4,0,0,42},{55,4,7,4,0,0,49},{56,4,7,4,0,0,56},{57,4,7,4,0,0,63},{46,2,7,2,0,0,70},{44,2,7,2,0,0,77},{43,4,7,4,0,0,84},{45,3,7,3,0,0,91},{37,6,7,6,0,0,98},{58,2,7,2,0,0,105},{107,4,7,4,0,0,112},{87,7,7,7,0,0,119},{104,4,7,4,0,0,126},{70,4,7,4,0,0,133},{86,5,7,5,0,0,140},{69,4,7,4,0,0,147},{68,5,7,5,0,0,154},{110,4,7,4,0,0,161},{101,4,7,4,0,0,168},{115,4,7,3,0,0,175},
};
static const Face Regular8Face = {8,11,Regular8Glyphs,27,Regular8BitmapBase64};
} // namespace DashboardSansV2
