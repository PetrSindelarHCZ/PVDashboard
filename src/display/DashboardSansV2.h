#pragma once

#include "DashboardSansV2Types.h"
#include "fonts/v2/DashboardSansV2_Regular12.h"
#include "fonts/v2/DashboardSansV2_Regular16.h"
#include "fonts/v2/DashboardSansV2_Bold16.h"
#include "fonts/v2/DashboardSansV2_Bold22.h"
#include "fonts/v2/DashboardSansV2_Bold28.h"
#include "fonts/v2/DashboardSansV2_Bold36.h"

namespace DashboardSansV2 {

inline const Face* face(uint8_t px, Weight weight) {
    if (weight == Weight::Regular) {
        switch (px) {
            case 12: return &Regular12Face;
            case 16: return &Regular16Face;
            default: return nullptr;
        }
    }

    switch (px) {
        case 16: return &Bold16Face;
        case 22: return &Bold22Face;
        case 28: return &Bold28Face;
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
