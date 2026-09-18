#pragma once

#include <Arduino.h>
#include <math.h>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../display/IDisplay.h"
#include "../screens/ScreenStyle.h"

namespace CustomWidgetRenderer {

inline bool resolveValue(const DataModel& dm, const String& source, float& value) {
    if (source.startsWith("solar.")) {
        if (!dm.solar.enabled || !dm.solar.status.available) return false;
        if (source == "solar.productionPowerW") value = dm.solar.productionPowerW;
        else if (source == "solar.houseConsumptionW") value = dm.solar.houseConsumptionW;
        else if (source == "solar.gridPowerW") value = dm.solar.gridPowerW;
        else if (source == "solar.energyTodayKWh") value = dm.solar.energyTodayKWh;
        else if (source == "solar.batterySocPercent") value = dm.solar.batterySocPercent;
        else if (source == "solar.batteryPowerW") value = dm.solar.batteryPowerW;
        else return false;
        return true;
    }

    if (source.startsWith("azrouter.")) {
        if (!dm.azrouter.enabled || !dm.azrouter.status.available) return false;
        if (source == "azrouter.gridPowerW") value = dm.azrouter.gridPowerW;
        else if (source == "azrouter.routedPowerW") value = dm.azrouter.routedPowerW;
        else if (source == "azrouter.routedEnergyTodayKWh") value = dm.azrouter.routedEnergyTodayKWh;
        else if (source == "azrouter.boilerTempC") value = dm.azrouter.boilerTempC;
        else return false;
        return true;
    }

    if (source.startsWith("weather.")) {
        if (!dm.weather.enabled || !dm.weather.status.available) return false;
        if (source == "weather.outdoorTempC") value = dm.weather.outdoorTempC;
        else if (source == "weather.outdoorHumidityPercent") value = dm.weather.outdoorHumidityPercent;
        else if (source == "weather.surfacePressureHpa") value = dm.weather.surfacePressureHpa;
        else if (source == "weather.windSpeedKmh") value = dm.weather.windSpeedKmh;
        else return false;
        return true;
    }

    if (source == "inside.livingRoomTempC") value = dm.inside.livingRoomTempC;
    else if (source == "inside.bedroomTempC") value = dm.inside.bedroomTempC;
    else if (source == "inside.poolTempC") value = dm.inside.poolTempC;
    else if (source.startsWith("pool.")) {
        if (!dm.pool.enabled) return false;
        if (source == "pool.waterTempC") value = dm.pool.waterTempC;
        else if (source == "pool.targetTempC") value = dm.pool.targetTempC;
        else if (source == "pool.ph") value = dm.pool.ph;
        else if (source == "pool.freeChlorineMgL") value = dm.pool.freeChlorineMgL;
        else if (source == "pool.airTempC") value = dm.pool.airTempC;
        else if (source == "pool.airHumidityPercent") value = dm.pool.airHumidityPercent;
        else return false;
    } else if (source == "system.wifiRssi") value = dm.system.wifiRssi;
    else if (source == "system.uptimeSeconds") value = dm.system.uptimeSeconds;
    else return false;

    return true;
}

inline String formatValue(float value, uint8_t decimals, const String& unit) {
    String text(value, static_cast<unsigned int>(decimals));
    if (!unit.isEmpty()) {
        text += " ";
        text += unit;
    }
    return text;
}

inline const uint8_t* fontFor(const CustomWidgetElementConfig& element, bool valueFont = false) {
    if (element.fontSize == "small") return DisplayFonts::sectionTitle();
    if (element.fontSize == "normal") return DisplayFonts::body();
    if (element.fontSize == "large") return DisplayFonts::value();
    return valueFont ? DisplayFonts::value() : DisplayFonts::body();
}

inline int16_t baselineOffset(const CustomWidgetElementConfig& element, bool valueFont = false) {
    if (element.fontSize == "small") return 16;
    if (element.fontSize == "normal") return 18;
    if (element.fontSize == "large") return 24;
    return valueFont ? 24 : 18;
}

inline int16_t alignedX(IDisplay& display, int16_t x, int16_t width,
                        const String& text, const String& align) {
    if (align == "center") return x + (width - display.textWidth(text)) / 2;
    if (align == "right") return x + width - display.textWidth(text);
    return x;
}

inline void useElementFont(IDisplay& display, const CustomWidgetElementConfig& element,
                           bool valueFont = false) {
    display.setTextColor(0);
    display.setUnicodeFont(fontFor(element, valueFont));
}

inline void drawText(IDisplay& display, int16_t x, int16_t y,
                     const CustomWidgetElementConfig& element) {
    useElementFont(display, element, false);
    const int16_t textX = alignedX(display, x, element.width, element.text, element.align);
    display.setCursor(textX, y + baselineOffset(element, false));
    display.print(element.text);
}

inline void drawKpi(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                    const CustomWidgetElementConfig& element) {
    int16_t valueY = y;
    if (element.showLabel && !element.label.isEmpty()) {
        ScreenStyle::useBody(display);
        const int16_t labelX = alignedX(display, x, element.width, element.label, element.align);
        display.setCursor(labelX, y + 16);
        display.print(element.label);
        valueY += 20;
    }

    float value = 0.0f;
    const bool available = resolveValue(dm, element.source, value);
    const String valueText = available
        ? formatValue(value, element.decimals, element.unit)
        : String("--");
    useElementFont(display, element, true);
    const int16_t valueX = alignedX(display, x, element.width, valueText, element.align);
    display.setCursor(valueX, valueY + baselineOffset(element, true));
    display.print(valueText);
}

inline void drawProgress(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                         const CustomWidgetElementConfig& element) {
    float value = 0.0f;
    const bool available = resolveValue(dm, element.source, value);

    int16_t barY = y + 4;
    if (element.showLabel) {
        const String labelText = !element.label.isEmpty() ? element.label : element.source;
        ScreenStyle::useBody(display);
        const int16_t labelX = alignedX(display, x, element.width, labelText, element.align);
        display.setCursor(labelX, y + 15);
        display.print(labelText);
        barY = y + 21;
    }
    int16_t barH = element.height - 22;
    if (barH < 10) barH = 10;
    if (barH > 18) barH = 18;
    display.drawRect(x, barY, element.width, barH, 0);

    if (!available) return;

    float ratio = (value - element.minValue) / (element.maxValue - element.minValue);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    const int16_t fillWidth = static_cast<int16_t>((element.width - 4) * ratio);
    if (fillWidth > 0) display.fillRect(x + 2, barY + 2, fillWidth, barH - 4, 0);
}

inline bool historyValue(const SolarHistorySample& sample, const String& source, float& value) {
    if (source == "solar.productionPowerW") value = sample.productionPowerW;
    else if (source == "solar.houseConsumptionW") value = sample.houseConsumptionW;
    else return false;
    return true;
}

inline void drawSparkline(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                          const CustomWidgetElementConfig& element) {
    int16_t graphY = y + 2;
    if (element.showLabel && !element.label.isEmpty()) {
        ScreenStyle::useBody(display);
        const int16_t labelX = alignedX(display, x, element.width, element.label, element.align);
        display.setCursor(labelX, y + 15);
        display.print(element.label);
        graphY = y + 20;
    }

    int16_t graphH = element.height - (graphY - y) - 2;
    if (graphH < 20) graphH = 20;
    display.drawRect(x, graphY, element.width, graphH, 0);

    const uint8_t count = dm.solar.historyCount;
    if (count < 2) return;

    float maxValue = 1.0f;
    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValue(dm.solar.history[i], element.source, value)) return;
        if (value > maxValue) maxValue = value;
    }

    const int16_t left = x + 2;
    const int16_t top = graphY + 2;
    int16_t plotW = element.width - 4;
    int16_t plotH = graphH - 4;
    if (plotW < 1) plotW = 1;
    if (plotH < 1) plotH = 1;

    if (element.graphStyle == "bars") {
        const int16_t barWidth = count > 0 ? max<int16_t>(1, plotW / count) : 1;
        for (uint8_t i = 0; i < count; ++i) {
            float value = 0.0f;
            if (!historyValue(dm.solar.history[i], element.source, value)) return;
            float normalized = value / maxValue;
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            const int16_t barH = static_cast<int16_t>(normalized * (plotH - 1));
            const int16_t px = left + static_cast<int16_t>((static_cast<uint32_t>(i) * plotW) / count);
            if (barH > 0) {
                const int16_t w = barWidth > 1 ? barWidth - 1 : 1;
                display.fillRect(px, top + plotH - barH, w, barH, 0);
            }
        }
        return;
    }

    int16_t previousX = left;
    int16_t previousY = top + plotH - 1;
    bool previousValid = false;

    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValue(dm.solar.history[i], element.source, value)) return;
        const int16_t px = left + static_cast<int16_t>((static_cast<uint32_t>(i) * (plotW - 1)) / (count - 1));
        float normalized = value / maxValue;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        const int16_t py = top + plotH - 1 - static_cast<int16_t>(normalized * (plotH - 1));
        if (previousValid) display.drawLine(previousX, previousY, px, py, 0);
        previousX = px;
        previousY = py;
        previousValid = true;
    }
}

inline void draw(IDisplay& display, const DataModel& dm, const HomeLayoutWidgetConfig& widget) {
    ScreenStyle::drawCard(display, widget.x, widget.y, widget.width, widget.height,
                          widget.title.isEmpty() ? "VLASTNÍ" : widget.title.c_str());

    for (uint8_t i = 0; i < widget.elements.size() && i < MaxCustomWidgetElements; ++i) {
        const CustomWidgetElementConfig& element = widget.elements[i];
        const int16_t x = widget.x + element.x;
        const int16_t y = widget.y + element.y;

        if (element.type == "text") drawText(display, x, y, element);
        else if (element.type == "kpi") drawKpi(display, dm, x, y, element);
        else if (element.type == "progress") drawProgress(display, dm, x, y, element);
        else if (element.type == "sparkline") drawSparkline(display, dm, x, y, element);
    }
}

} // namespace CustomWidgetRenderer
