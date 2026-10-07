#pragma once
#include "../../DashboardSansV2Types.h"
namespace DashboardSansV2 {
static const char Bold16BitmapBase64[] PROGMEM =
"f/B/8H/wcABwAHAAcAB/4H/gf+BwAHAAcABwAHAAcADwHnAccBx4PDg4ODg8eBxwHHAc8A7gDuAO4AfAB8AHwH/wf/B/8HAAcABwAH/wf/B/8HAAcABwAHAAf/B/8H/wf+B/4H/gcABwAHeAf8B54HDwAHAAcHBwcPB94D/AD4BwcHAAAeAD4APgB+AO4A7gHOA44DjgcOB/+H/4AOAA4ADgAOBwAHAAcABwAHAAceBxwHOAdwB+AH8AfwBzgHPAceBw4PBwePDwcHD4cHD4cHD48HjY4Dnc4Dnc4Dnc4B2NwB+PwB+PwB+PwA8HgA8HgA8HgA==";
static const Glyph Bold16Glyphs[] PROGMEM = {
    {32,0,0,4,0,0,0},{70,12,16,12,0,5,0},{86,15,16,15,0,5,32},{69,13,16,13,0,5,64},
    {53,13,16,13,0,5,96},{46,5,4,5,0,17,128},{52,14,16,14,0,5,132},{107,12,16,12,0,5,164},
    {87,21,16,21,0,5,196},
};
static const Face Bold16Face = {16,27,Bold16Glyphs,9,Bold16BitmapBase64};
} // namespace DashboardSansV2
