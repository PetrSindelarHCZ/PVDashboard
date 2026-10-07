#pragma once

#include "DashboardSansV2Types.h"

#include "fonts/v2/DashboardSansV2_Regular8.h"
#include "fonts/v2/DashboardSansV2_Bold8.h"
#include "fonts/v2/DashboardSansV2_Regular10.h"
#include "fonts/v2/DashboardSansV2_Bold10.h"
#include "fonts/v2/DashboardSansV2_Regular12.h"
#include "fonts/v2/DashboardSansV2_Bold12.h"
#include "fonts/v2/DashboardSansV2_Regular14.h"
#include "fonts/v2/DashboardSansV2_Bold14.h"
#include "fonts/v2/DashboardSansV2_Bold16.h"
#include "fonts/v2/DashboardSansV2_Regular18.h"
#include "fonts/v2/DashboardSansV2_Bold18.h"
#include "fonts/DashboardSansV2LargeFaces.h"

namespace DashboardSansV2 {

inline const Face* face(uint8_t px, Weight weight) {
    if (weight == Weight::Regular) {
        switch (px) {
            case 8:  return &Regular8Face;
            case 10: return &Regular10Face;
            case 12: return &Regular12Face;
            case 14: return &Regular14Face;
            case 16: return &Regular16Face;
            case 18: return &Regular18Face;
            case 20: return &Regular20Face;
            case 22: return &Regular22Face;
            case 24: return &Regular24Face;
            case 28: return &Regular28Face;
            case 32: return &Regular32Face;
            case 36: return &Regular36Face;
            default: return nullptr;
        }
    }

    switch (px) {
        case 8:  return &Bold8Face;
        case 10: return &Bold10Face;
        case 12: return &Bold12Face;
        case 14: return &Bold14Face;
        case 16: return &Bold16Face;
        case 18: return &Bold18Face;
        case 20: return &Bold20Face;
        case 22: return &Bold22Face;
        case 24: return &Bold24Face;
        case 28: return &Bold28Face;
        case 32: return &Bold32Face;
        case 36: return &Bold36Face;
        default: return nullptr;
    }
}

inline bool hasFace(uint8_t px, Weight weight) {
    return face(px, weight) != nullptr;
}

inline int16_t textWidth(
    const String& text,
    uint8_t px,
    Weight weight) {

    const Face* selected = face(px, weight);
    return selected != nullptr
        ? DashboardSansV2::textWidth(*selected, text)
        : 0;
}

inline void drawText(
    IDisplay& display,
    int16_t x,
    int16_t top,
    const String& text,
    uint8_t px,
    Weight weight,
    uint16_t color = 0) {

    const Face* selected = face(px, weight);
    if (selected == nullptr) return;

    DashboardSansV2::drawText(
        display,
        x,
        top,
        *selected,
        text,
        color);
}

} // namespace DashboardSansV2
