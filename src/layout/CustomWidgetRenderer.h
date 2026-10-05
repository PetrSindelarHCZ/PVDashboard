#pragma once

#include <Arduino.h>
#include <math.h>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../display/IDisplay.h"
#include "../display/assets/WidgetIcons.h"
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

        if (source == "azrouter.gridPowerW") {
            if (!dm.azrouter.hasGridPower) return false;
            value = dm.azrouter.gridPowerW;
        } else if (source == "azrouter.gridL1PowerW" ||
                   source == "azrouter.gridL2PowerW" ||
                   source == "azrouter.gridL3PowerW") {
            const uint8_t phase = source == "azrouter.gridL1PowerW" ? 0 :
                                  (source == "azrouter.gridL2PowerW" ? 1 : 2);
            if (!dm.azrouter.hasGridPhasePower[phase]) return false;
            value = dm.azrouter.gridPhasePowerW[phase];
        } else if (source == "azrouter.gridL1VoltageV" ||
                   source == "azrouter.gridL2VoltageV" ||
                   source == "azrouter.gridL3VoltageV") {
            const uint8_t phase = source == "azrouter.gridL1VoltageV" ? 0 :
                                  (source == "azrouter.gridL2VoltageV" ? 1 : 2);
            if (!dm.azrouter.hasGridPhaseVoltage[phase]) return false;
            value = dm.azrouter.gridPhaseVoltageV[phase];
        } else if (source == "azrouter.gridL1CurrentA" ||
                   source == "azrouter.gridL2CurrentA" ||
                   source == "azrouter.gridL3CurrentA") {
            const uint8_t phase = source == "azrouter.gridL1CurrentA" ? 0 :
                                  (source == "azrouter.gridL2CurrentA" ? 1 : 2);
            if (!dm.azrouter.hasGridPhaseCurrent[phase]) return false;
            value = dm.azrouter.gridPhaseCurrentA[phase];
        } else if (source == "azrouter.routedPowerW") {
            if (!dm.azrouter.hasRoutedPower) return false;
            value = dm.azrouter.routedPowerW;
        } else if (source == "azrouter.routedL1PowerW" ||
                   source == "azrouter.routedL2PowerW" ||
                   source == "azrouter.routedL3PowerW") {
            const uint8_t phase = source == "azrouter.routedL1PowerW" ? 0 :
                                  (source == "azrouter.routedL2PowerW" ? 1 : 2);
            if (!dm.azrouter.hasRoutedPhasePower[phase]) return false;
            value = dm.azrouter.routedPhasePowerW[phase];
        } else if (source == "azrouter.routedEnergyTodayKWh") {
            if (!dm.azrouter.hasRoutedEnergyToday) return false;
            value = dm.azrouter.routedEnergyTodayKWh;
        } else if (source == "azrouter.routedEnergyWeekKWh") {
            if (!dm.azrouter.hasRoutedEnergyWeek) return false;
            value = dm.azrouter.routedEnergyWeekKWh;
        } else if (source == "azrouter.routedEnergyMonthKWh") {
            if (!dm.azrouter.hasRoutedEnergyMonth) return false;
            value = dm.azrouter.routedEnergyMonthKWh;
        } else if (source == "azrouter.routedEnergyYearKWh") {
            if (!dm.azrouter.hasRoutedEnergyYear) return false;
            value = dm.azrouter.routedEnergyYearKWh;
        } else if (source == "azrouter.routedEnergyTotalKWh") {
            if (!dm.azrouter.hasRoutedEnergyTotal) return false;
            value = dm.azrouter.routedEnergyTotalKWh;
        } else if (source == "azrouter.systemTempC") {
            if (!dm.azrouter.hasSystemTemp) return false;
            value = dm.azrouter.systemTempC;
        } else {
            return false;
        }
        return true;
    }

    if (source.startsWith("rf.")) {
        const int metricSeparator = source.indexOf('.', 3);
        if (metricSeparator < 0) return false;

        const String slotId = source.substring(3, metricSeparator);
        const String metric = source.substring(metricSeparator + 1);

        for (uint8_t i = 0;
             i < dm.rfSensors.sensorCount && i < MaxRfSensors;
             ++i) {
            const RfSensorData& sensor = dm.rfSensors.sensors[i];
            if (!sensor.configured || sensor.slotId != slotId) continue;
            if (!sensor.available) return false;

            if (metric == "temperatureC" && sensor.hasTemperature) {
                value = sensor.temperatureC;
                return true;
            }
            if (metric == "humidityPercent" && sensor.hasHumidity) {
                value = sensor.humidityPercent;
                return true;
            }
            return false;
        }
        return false;
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

    if (source.startsWith("battery.")) {
        if (!dm.battery.status.available) return false;
        if (source == "battery.voltageV") value = dm.battery.voltageV;
        else if (source == "battery.socPercent") value = dm.battery.socPercent;
        else if (source == "battery.changeRatePercentPerHour")
            value = dm.battery.changeRatePercentPerHour;
        else return false;
        return true;
    }

    if (source.startsWith("inside.")) {
        if (source == "inside.temperatureC" ||
            source == "inside.humidityPercent" ||
            source == "inside.pressureHpa" ||
            source == "inside.livingRoomTempC") {
            if (!dm.inside.status.available) return false;
            if (source == "inside.temperatureC") value = dm.inside.temperatureC;
            else if (source == "inside.humidityPercent") value = dm.inside.humidityPercent;
            else if (source == "inside.pressureHpa") value = dm.inside.pressureHpa;
            else value = dm.inside.livingRoomTempC;
        } else if (source == "inside.bedroomTempC") {
            value = dm.inside.bedroomTempC;
        } else if (source == "inside.poolTempC") {
            value = dm.inside.poolTempC;
        } else {
            return false;
        }
        return true;
    } else if (source.startsWith("pool.")) {
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

inline uint8_t requestedFontPx(const CustomWidgetElementConfig& element, bool valueFont = false) {
    if (element.fontSize == "auto") return valueFont ? 22 : 18;
    if (element.fontSize == "small") return 16;
    if (element.fontSize == "normal") return 18;
    if (element.fontSize == "large") return 22;

    const int px = element.fontSize.toInt();
    if (px < 7 || px > 64) return valueFont ? 22 : 18;
    return static_cast<uint8_t>(px);
}

inline const uint8_t* sourceFontFor(uint8_t px, bool bold) {
    if (px <= 11) return bold ? u8g2_font_t0_11b_te : u8g2_font_t0_11_te;
    if (px <= 12) return bold ? u8g2_font_t0_12b_te : u8g2_font_t0_12_te;
    if (px <= 13) return bold ? u8g2_font_t0_13b_te : u8g2_font_t0_13_te;
    if (px <= 14) return bold ? u8g2_font_t0_14b_te : u8g2_font_t0_14_te;
    if (px <= 15) return bold ? u8g2_font_t0_15b_te : u8g2_font_t0_15_te;
    if (px <= 16) return bold ? u8g2_font_t0_16b_te : u8g2_font_t0_16_te;
    if (px <= 17) return bold ? u8g2_font_t0_17b_te : u8g2_font_t0_17_te;
    if (px <= 18) return bold ? u8g2_font_t0_18b_te : u8g2_font_t0_18_te;
    return bold ? u8g2_font_t0_22b_te : u8g2_font_t0_22_te;
}

inline int16_t sourceFontHeight(const uint8_t* font) {
    U8G2_FOR_ADAFRUIT_GFX metrics;
    metrics.setFont(font);
    const int16_t height =
        metrics.u8g2.font_info.ascent_para - metrics.u8g2.font_info.descent_para;
    return height > 0 ? height : 1;
}

inline int16_t sourceFontAscent(const uint8_t* font) {
    U8G2_FOR_ADAFRUIT_GFX metrics;
    metrics.setFont(font);
    return metrics.u8g2.font_info.ascent_para;
}

inline int16_t scaledTextWidth(const String& text, uint8_t targetPx, bool bold) {
    if (text.isEmpty()) return 0;
    const uint8_t* font = sourceFontFor(targetPx, bold);
    U8G2_FOR_ADAFRUIT_GFX metrics;
    metrics.setFont(font);
    const int16_t sourceHeight = sourceFontHeight(font);
    const int16_t sourceWidth = metrics.getUTF8Width(text.c_str());
    if (sourceWidth <= 0) return 0;
    return static_cast<int16_t>(
        (static_cast<int32_t>(sourceWidth) * targetPx + sourceHeight - 1) / sourceHeight);
}

inline String fitScaledText(const String& input, int16_t width, uint8_t targetPx, bool bold) {
    if (width <= 0 || scaledTextWidth(input, targetPx, bold) <= width) return input;

    String text = input;
    const String ellipsis = "...";
    if (scaledTextWidth(ellipsis, targetPx, bold) > width) return String();

    while (text.length() && scaledTextWidth(text + ellipsis, targetPx, bold) > width) {
        unsigned int end = text.length() - 1;
        while (end > 0 && (static_cast<uint8_t>(text[end]) & 0xC0) == 0x80) --end;
        text.remove(end);
    }
    text += ellipsis;
    return text;
}

inline int16_t alignedScaledX(int16_t x, int16_t width, int16_t textWidth, const String& align) {
    if (align == "center") return x + (width - textWidth) / 2;
    if (align == "right") return x + width - textWidth;
    return x;
}

inline bool drawScaledUnicodeText(IDisplay& display, int16_t x, int16_t y,
                                  const String& text, uint8_t targetPx, bool bold,
                                  uint16_t color, int16_t maxHeight) {
    if (text.isEmpty() || targetPx == 0 || maxHeight <= 0) return true;

    const uint8_t* font = sourceFontFor(targetPx, bold);
    U8G2_FOR_ADAFRUIT_GFX metrics;
    metrics.setFont(font);
    const int16_t sourceHeight = sourceFontHeight(font);
    const int16_t sourceWidth = metrics.getUTF8Width(text.c_str());
    if (sourceWidth <= 0) return true;

    GFXcanvas1 canvas(static_cast<uint16_t>(sourceWidth), static_cast<uint16_t>(sourceHeight));
    if (canvas.getBuffer() == nullptr) return false;
    canvas.fillScreen(0);

    U8G2_FOR_ADAFRUIT_GFX painter;
    painter.begin(canvas);
    painter.setFont(font);
    painter.setFontMode(1);
    painter.setForegroundColor(1);
    painter.setCursor(0, sourceFontAscent(font));
    painter.print(text);

    const int16_t targetWidth = static_cast<int16_t>(
        (static_cast<int32_t>(sourceWidth) * targetPx + sourceHeight - 1) / sourceHeight);
    const int16_t drawHeight = targetPx < maxHeight ? targetPx : maxHeight;

    for (int16_t dy = 0; dy < drawHeight; ++dy) {
        const int16_t sy = static_cast<int16_t>(
            (static_cast<int32_t>(dy) * sourceHeight) / targetPx);
        for (int16_t dx = 0; dx < targetWidth; ++dx) {
            const int16_t sx = static_cast<int16_t>(
                (static_cast<int32_t>(dx) * sourceWidth) / targetWidth);
            if (canvas.getPixel(sx, sy)) display.drawPixel(x + dx, y + dy, color);
        }
    }
    return true;
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

inline String fitText(IDisplay& display, const String& input, int16_t width) {
    if (width <= 0 || display.textWidth(input) <= width) return input;

    String text = input;
    const String ellipsis = "...";
    if (display.textWidth(ellipsis) > width) return String();

    while (text.length() && display.textWidth(text + ellipsis) > width) {
        unsigned int end = text.length() - 1;
        while (end > 0 && (static_cast<uint8_t>(text[end]) & 0xC0) == 0x80) --end;
        text.remove(end);
    }
    text += ellipsis;
    return text;
}

inline int16_t alignedX(IDisplay& display, int16_t x, int16_t width,
                        const String& text, const String& align) {
    if (align == "center") return x + (width - display.textWidth(text)) / 2;
    if (align == "right") return x + width - display.textWidth(text);
    return x;
}

inline void useElementFont(IDisplay& display, const CustomWidgetElementConfig& element,
                           bool valueFont = false, uint16_t color = 0) {
    display.setTextColor(color);
    display.setUnicodeFont(fontFor(element, valueFont));
}

inline int16_t verticalOffset(
    const CustomWidgetElementConfig& element,
    int16_t contentHeight) {

    const int16_t freeHeight =
        element.height > contentHeight
            ? element.height - contentHeight
            : 0;

    if (element.verticalAlign == 1) return freeHeight / 2;
    if (element.verticalAlign == 2) return freeHeight;
    return 0;
}

inline int16_t elementFontHeight(
    const CustomWidgetElementConfig& element,
    bool valueFont) {

    if (element.fontSize != "auto")
        return requestedFontPx(element, valueFont);

    return sourceFontHeight(fontFor(element, valueFont));
}

inline void drawText(IDisplay& display, int16_t x, int16_t y,
                     const CustomWidgetElementConfig& element, uint16_t textColor) {
    const int16_t contentHeight =
        elementFontHeight(element, false);
    const int16_t top =
        y + verticalOffset(element, contentHeight);

    if (element.fontSize != "auto") {
        const uint8_t fontPx = requestedFontPx(element, false);
        const String text = fitScaledText(element.text, element.width, fontPx, false);
        const int16_t textWidth = scaledTextWidth(text, fontPx, false);
        const int16_t textX = alignedScaledX(x, element.width, textWidth, element.align);
        if (drawScaledUnicodeText(
                display, textX, top, text, fontPx, false,
                textColor, element.height - (top - y))) {
            return;
        }
    }

    useElementFont(display, element, false, textColor);
    const String text = fitText(display, element.text, element.width);
    const int16_t textX = alignedX(display, x, element.width, text, element.align);
    display.setCursor(textX, top + baselineOffset(element, false));
    display.print(text);
}

inline void drawKpi(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                    const CustomWidgetElementConfig& element, uint16_t textColor) {
    const bool hasLabel =
        element.showLabel && !element.label.isEmpty();
    const int16_t valueHeight =
        elementFontHeight(element, true);
    const int16_t blockHeight =
        valueHeight + (hasLabel ? 20 : 0);
    const int16_t blockTop =
        y + verticalOffset(element, blockHeight);

    int16_t valueY = blockTop;
    if (hasLabel) {
        ScreenStyle::useBody(display, textColor);
        const String label = fitText(display, element.label, element.width);
        const int16_t labelX = alignedX(display, x, element.width, label, element.align);
        display.setCursor(labelX, blockTop + 16);
        display.print(label);
        valueY += 20;
    }

    float value = 0.0f;
    const bool available = resolveValue(dm, element.source, value);
    String valueText = available
        ? formatValue(value, element.decimals, element.unit)
        : String("--");
    if (element.fontSize != "auto") {
        const uint8_t fontPx = requestedFontPx(element, true);
        valueText = fitScaledText(valueText, element.width, fontPx, true);
        const int16_t valueWidth = scaledTextWidth(valueText, fontPx, true);
        const int16_t valueX = alignedScaledX(x, element.width, valueWidth, element.align);
        const int16_t availableHeight =
            element.height - (valueY - y);
        if (drawScaledUnicodeText(display, valueX, valueY, valueText, fontPx, true,
                                  textColor, availableHeight)) {
            return;
        }
    }

    useElementFont(display, element, true, textColor);
    valueText = fitText(display, valueText, element.width);
    const int16_t valueX = alignedX(display, x, element.width, valueText, element.align);
    display.setCursor(valueX, valueY + baselineOffset(element, true));
    display.print(valueText);
}

inline void drawProgress(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                         const CustomWidgetElementConfig& element, uint16_t textColor) {
    float value = 0.0f;
    const bool available = resolveValue(dm, element.source, value);

    int16_t barY = y + 4;
    if (element.showLabel) {
        ScreenStyle::useBody(display, textColor);
        const String rawLabel = !element.label.isEmpty() ? element.label : element.source;
        const String labelText = fitText(display, rawLabel, element.width);
        const int16_t labelX = alignedX(display, x, element.width, labelText, element.align);
        display.setCursor(labelX, y + 15);
        display.print(labelText);
        barY = y + 21;
    }
    int16_t barH = element.height - 22;
    if (barH < 10) barH = 10;
    if (barH > 18) barH = 18;
    display.drawRect(x, barY, element.width, barH, textColor);

    if (!available) return;

    float ratio = (value - element.minValue) / (element.maxValue - element.minValue);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    const int16_t fillWidth = static_cast<int16_t>((element.width - 4) * ratio);
    if (fillWidth > 0) display.fillRect(x + 2, barY + 2, fillWidth, barH - 4, textColor);
}

inline bool rfHistorySource(const String& source, int& stableIndex, String& metric) {
    if (!source.startsWith("rf.sensor")) return false;
    const int metricSeparator = source.indexOf('.', 3);
    if (metricSeparator <= 3) return false;

    const String slotId = source.substring(3, metricSeparator);
    if (!slotId.startsWith("sensor")) return false;
    const int slot = slotId.substring(6).toInt();
    if (slot < 1 || slot > MaxRfSensors || slotId != "sensor" + String(slot)) return false;

    metric = source.substring(metricSeparator + 1);
    if (metric != "temperatureC" && metric != "humidityPercent") return false;
    stableIndex = slot - 1;
    return true;
}

inline uint8_t normalizedGraphPeriodHours(uint8_t hours) {
    return sensorGraphPeriodIndex(hours) >= 0 ? hours : 12;
}

inline uint8_t historyCount(const DataModel& dm, const String& source,
                            uint8_t periodHours) {
    if (source == "solar.productionPowerW" ||
        source == "solar.houseConsumptionW") {
        return dm.solar.historyCount;
    }

    const int8_t periodIndex =
        sensorGraphPeriodIndex(normalizedGraphPeriodHours(periodHours));
    if (periodIndex < 0) return 0;

    if (source == "inside.temperatureC" ||
        source == "inside.humidityPercent" ||
        source == "inside.pressureHpa") {
        if (dm.inside.history == nullptr) return 0;
        return dm.inside.history->series[periodIndex].count;
    }

    int stableIndex = -1;
    String metric;
    if (!rfHistorySource(source, stableIndex, metric) ||
        stableIndex < 0 || stableIndex >= MaxRfSensors ||
        dm.rfSensors.history == nullptr ||
        dm.rfSensors.history[stableIndex] == nullptr) {
        return 0;
    }
    return dm.rfSensors.history[stableIndex]->series[periodIndex].count;
}

inline bool historyValueAt(const DataModel& dm, const String& source,
                           uint8_t periodHours,
                           uint8_t chronologicalIndex, float& value) {
    if (source == "solar.productionPowerW" ||
        source == "solar.houseConsumptionW") {
        if (chronologicalIndex >= dm.solar.historyCount) return false;
        const SolarHistorySample& sample = dm.solar.history[chronologicalIndex];
        if (source == "solar.productionPowerW")
            value = sample.productionPowerW;
        else
            value = sample.houseConsumptionW;
        return true;
    }

    const int8_t periodIndex =
        sensorGraphPeriodIndex(normalizedGraphPeriodHours(periodHours));
    if (periodIndex < 0) return false;

    if (source == "inside.temperatureC" ||
        source == "inside.humidityPercent" ||
        source == "inside.pressureHpa") {
        if (dm.inside.history == nullptr) return false;

        const InsideHistorySeries& series =
            dm.inside.history->series[periodIndex];
        if (chronologicalIndex >= series.count) return false;

        const uint8_t oldest =
            static_cast<uint8_t>(
                (series.next + SensorGraphSampleCount - series.count) %
                SensorGraphSampleCount);
        const uint8_t physical =
            static_cast<uint8_t>(
                (oldest + chronologicalIndex) % SensorGraphSampleCount);
        const InsideHistorySample& sample = series.samples[physical];

        if (source == "inside.temperatureC") {
            if ((sample.flags & 0x01) == 0) return false;
            value = sample.temperatureCenti / 100.0f;
            return true;
        }
        if (source == "inside.humidityPercent") {
            if ((sample.flags & 0x02) == 0) return false;
            value = sample.humidityPercent;
            return true;
        }
        if ((sample.flags & 0x04) == 0) return false;
        value = sample.pressureDeciHpa / 10.0f;
        return true;
    }

    int stableIndex = -1;
    String metric;
    if (!rfHistorySource(source, stableIndex, metric) ||
        stableIndex < 0 || stableIndex >= MaxRfSensors ||
        dm.rfSensors.history == nullptr ||
        dm.rfSensors.history[stableIndex] == nullptr) {
        return false;
    }

    const RfHistorySeries& series =
        dm.rfSensors.history[stableIndex]->series[periodIndex];
    if (chronologicalIndex >= series.count) return false;

    const uint8_t oldest =
        static_cast<uint8_t>(
            (series.next + SensorGraphSampleCount - series.count) %
            SensorGraphSampleCount);
    const uint8_t physical =
        static_cast<uint8_t>(
            (oldest + chronologicalIndex) % SensorGraphSampleCount);
    const RfHistorySample& sample = series.samples[physical];

    if (metric == "temperatureC") {
        if ((sample.flags & 0x01) == 0) return false;
        value = sample.temperatureCenti / 100.0f;
        return true;
    }
    if ((sample.flags & 0x02) == 0) return false;
    value = sample.humidityPercent;
    return true;
}

inline void drawMinMax(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                       const CustomWidgetElementConfig& element, uint8_t sharedPeriodHours,
                       uint16_t textColor) {
    const uint8_t periodHours =
        normalizedGraphPeriodHours(sharedPeriodHours);
    const uint8_t count =
        historyCount(dm, element.source, periodHours);

    float minValue = 0.0f;
    float maxValue = 0.0f;
    bool haveValue = false;

    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValueAt(dm, element.source, periodHours, i, value))
            continue;
        if (!haveValue) {
            minValue = maxValue = value;
            haveValue = true;
        } else {
            if (value < minValue) minValue = value;
            if (value > maxValue) maxValue = value;
        }
    }

    const int16_t rowHeight =
        elementFontHeight(element, false);
    const int16_t gap = 4;
    const int16_t blockHeight = rowHeight * 2 + gap;
    const int16_t top =
        y + verticalOffset(element, blockHeight);

    const String minValueText = haveValue
        ? formatValue(minValue, element.decimals, element.unit)
        : String("--");
    const String maxValueText = haveValue
        ? formatValue(maxValue, element.decimals, element.unit)
        : String("--");

    auto drawRow = [&](const String& prefix, const String& rawValue, int16_t rowY) {
        if (element.fontSize != "auto") {
            const uint8_t fontPx = requestedFontPx(element, false);
            const int16_t prefixWidth =
                scaledTextWidth(prefix, fontPx, false);
            const int16_t available =
                max<int16_t>(0, element.width - prefixWidth);
            const String value =
                fitScaledText(rawValue, available, fontPx, true);
            const int16_t valueWidth =
                scaledTextWidth(value, fontPx, true);
            const int16_t totalWidth = prefixWidth + valueWidth;
            const int16_t rowX =
                alignedScaledX(x, element.width, totalWidth, element.align);

            drawScaledUnicodeText(
                display, rowX, rowY, prefix, fontPx, false,
                textColor, element.height - (rowY - y));
            drawScaledUnicodeText(
                display, rowX + prefixWidth, rowY, value, fontPx, true,
                textColor, element.height - (rowY - y));
            return;
        }

        display.setTextColor(textColor);
        display.setUnicodeFont(DisplayFonts::body());
        const int16_t prefixWidth = display.textWidth(prefix);

        display.setUnicodeFont(DisplayFonts::strongBody());
        const String value =
            fitText(display, rawValue, max<int16_t>(0, element.width - prefixWidth));
        const int16_t valueWidth = display.textWidth(value);
        const int16_t totalWidth = prefixWidth + valueWidth;

        int16_t rowX = x;
        if (element.align == "center")
            rowX = x + (element.width - totalWidth) / 2;
        else if (element.align == "right")
            rowX = x + element.width - totalWidth;

        display.setUnicodeFont(DisplayFonts::body());
        display.setCursor(rowX, rowY + 18);
        display.print(prefix);

        display.setUnicodeFont(DisplayFonts::strongBody());
        display.setCursor(rowX + prefixWidth, rowY + 18);
        display.print(value);
    };

    drawRow("Min ", minValueText, top);
    drawRow("Max ", maxValueText, top + rowHeight + gap);
}

inline void drawTrend(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                      const CustomWidgetElementConfig& element, uint8_t sharedPeriodHours,
                      uint16_t textColor) {
    const uint8_t periodHours =
        normalizedGraphPeriodHours(sharedPeriodHours);
    const uint8_t count =
        historyCount(dm, element.source, periodHours);

    int direction = 0;
    float newest = 0.0f;
    float previous = 0.0f;
    bool haveNewest = false;
    bool havePrevious = false;

    for (int16_t i = static_cast<int16_t>(count) - 1;
         i >= 0 && !havePrevious;
         --i) {
        float candidate = 0.0f;
        if (!historyValueAt(
                dm, element.source, periodHours,
                static_cast<uint8_t>(i), candidate)) {
            continue;
        }
        if (!haveNewest) {
            newest = candidate;
            haveNewest = true;
        } else {
            previous = candidate;
            havePrevious = true;
        }
    }

    if (haveNewest && havePrevious) {
        float threshold = 0.15f;
        if (element.source.endsWith(".humidityPercent")) threshold = 1.0f;
        else if (element.source.endsWith(".pressureHpa")) threshold = 0.5f;
        const float delta = newest - previous;
        if (delta > threshold) direction = 1;
        else if (delta < -threshold) direction = -1;
    }

    // Trend has no caption. The arrow always occupies the whole element
    // and is centered in both axes regardless of the generic text alignment.
    const int16_t size =
        max<int16_t>(10, min<int16_t>(
            min<int16_t>(element.width, element.height),
            requestedFontPx(element, true)));

    const int16_t cx = x + element.width / 2;
    const int16_t cy = y + element.height / 2;
    const int16_t half = max<int16_t>(4, size / 3);

    const int16_t shaftHalf = max<int16_t>(1, size / 12);
    const int16_t headHalf = max<int16_t>(4, size / 4);
    const int16_t headDepth = max<int16_t>(4, size / 4);

    if (direction > 0) {
        // Wide filled-looking upward arrow, built from primitive lines so it
        // stays crisp on the monochrome panel.
        display.fillRect(
            cx - shaftHalf, cy - half + headDepth,
            shaftHalf * 2 + 1, half * 2 - headDepth, textColor);
        for (int16_t row = 0; row < headDepth; ++row) {
            const int16_t spread =
                static_cast<int16_t>(
                    (static_cast<int32_t>(headHalf) * row) /
                    max<int16_t>(1, headDepth - 1));
            display.drawLine(
                cx - spread, cy - half + row,
                cx + spread, cy - half + row, textColor);
        }
    } else if (direction < 0) {
        display.fillRect(
            cx - shaftHalf, cy - half,
            shaftHalf * 2 + 1, half * 2 - headDepth, textColor);
        for (int16_t row = 0; row < headDepth; ++row) {
            const int16_t spread =
                static_cast<int16_t>(
                    (static_cast<int32_t>(headHalf) *
                     (headDepth - 1 - row)) /
                    max<int16_t>(1, headDepth - 1));
            display.drawLine(
                cx - spread, cy + half - headDepth + 1 + row,
                cx + spread, cy + half - headDepth + 1 + row,
                textColor);
        }
    } else {
        // Stable state uses a bold horizontal arrow instead of a thin dash.
        display.fillRect(
            cx - half, cy - shaftHalf,
            half * 2 - headDepth, shaftHalf * 2 + 1, textColor);
        for (int16_t col = 0; col < headDepth; ++col) {
            const int16_t spread =
                static_cast<int16_t>(
                    (static_cast<int32_t>(headHalf) *
                     (headDepth - 1 - col)) /
                    max<int16_t>(1, headDepth - 1));
            display.drawLine(
                cx + half - headDepth + 1 + col, cy - spread,
                cx + half - headDepth + 1 + col, cy + spread,
                textColor);
        }
    }
}

inline uint8_t historySlotCount(const String& source, uint8_t count) {
    if (source.startsWith("rf.") ||
        source == "inside.temperatureC" ||
        source == "inside.humidityPercent" ||
        source == "inside.pressureHpa") {
        return 24;
    }
    return count > 0 ? count : 1;
}

inline float graphMinimumDelta(const String& source) {
    if (source.endsWith(".humidityPercent")) return 2.0f;
    if (source.endsWith(".pressureHpa")) return 1.0f;
    if (source.endsWith(".temperatureC")) return 0.5f;
    return 1.0f;
}

inline String graphPeriodLabel(uint8_t periodHours) {
    if (periodHours < 24) return "-" + String(periodHours) + " h";
    return "-" + String(periodHours / 24) + " d";
}

inline void drawSparkline(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                          const CustomWidgetElementConfig& element, uint8_t sharedPeriodHours,
                          uint16_t textColor) {
    int16_t graphY = y + 2;
    if (element.showLabel && !element.label.isEmpty()) {
        ScreenStyle::useBody(display, textColor);
        const String label = fitText(display, element.label, element.width);
        const int16_t labelX = alignedX(display, x, element.width, label, element.align);
        display.setCursor(labelX, y + 15);
        display.print(label);
        graphY = y + 20;
    }

    int16_t graphH = element.height - (graphY - y) - 2;
    if (graphH < 20) graphH = 20;

    const uint8_t periodHours =
        normalizedGraphPeriodHours(sharedPeriodHours);
    const uint8_t count =
        historyCount(dm, element.source, periodHours);

    // Sensor bar graph: current live value is the visual zero in the middle.
    // Historical samples extend above/below that center according to their
    // deviation from the current value.
    if (element.graphStyle == "bars") {
        const int16_t labelHeight = element.height >= 48 ? 13 : 0;
        const int16_t leftAxisWidth = element.width >= 70 ? 7 : 3;
        const int16_t left = x + leftAxisWidth;
        int16_t plotW = element.width - leftAxisWidth;
        if (plotW < 1) plotW = 1;

        const int16_t axisY = graphY + graphH - labelHeight - 1;
        const int16_t plotTop = graphY + 1;
        const int16_t plotBottom = axisY - 3;
        int16_t plotH = plotBottom - plotTop + 1;
        if (plotH < 4) plotH = 4;

        const int16_t centerY = plotTop + plotH / 2;
        const int16_t halfH = max<int16_t>(1, plotH / 2 - 1);

        // No vertical axis line: only a short center tick indicating the
        // current-value level. The bottom line is the zero/base of all bars.
        const int16_t yAxisX = x + leftAxisWidth - 2;
        display.drawLine(yAxisX - 2, centerY, yAxisX + 3, centerY, textColor);
        display.drawLine(left, axisY, left + plotW - 1, axisY, textColor);
        display.drawLine(left, axisY, left, axisY + 2, textColor);
        display.drawLine(left + plotW - 1, axisY,
                         left + plotW - 1, axisY + 2, textColor);

        if (labelHeight > 0) {
            display.setTextColor(textColor);
            display.setUnicodeFont(u8g2_font_t0_11_te);
            const String fromLabel = graphPeriodLabel(periodHours);
            const String nowLabel = "teď";
            display.setCursor(left, axisY + 11);
            display.print(fromLabel);
            const int16_t nowWidth = display.textWidth(nowLabel);
            display.setCursor(left + plotW - nowWidth, axisY + 11);
            display.print(nowLabel);
        }

        if (count == 0) return;

        float currentValue = 0.0f;
        if (!resolveValue(dm, element.source, currentValue)) {
            // If live data is temporarily unavailable, use the newest valid
            // historical sample as the center reference.
            bool found = false;
            for (int16_t i = static_cast<int16_t>(count) - 1;
                 i >= 0 && !found;
                 --i) {
                found = historyValueAt(
                    dm, element.source, periodHours,
                    static_cast<uint8_t>(i), currentValue);
            }
            if (!found) return;
        }

        float maxAbsDelta = graphMinimumDelta(element.source);
        for (uint8_t i = 0; i < count; ++i) {
            float value = 0.0f;
            if (!historyValueAt(
                    dm, element.source, periodHours, i, value)) {
                continue;
            }
            const float delta = fabsf(value - currentValue);
            if (delta > maxAbsDelta) maxAbsDelta = delta;
        }
        if (maxAbsDelta < 0.001f)
            maxAbsDelta = graphMinimumDelta(element.source);

        const uint8_t slotCount =
            historySlotCount(element.source, count);
        const uint8_t firstSlot =
            count < slotCount ? slotCount - count : 0;

        for (uint8_t i = 0; i < count; ++i) {
            float value = 0.0f;
            if (!historyValueAt(
                    dm, element.source, periodHours, i, value)) {
                continue;
            }

            const float delta = value - currentValue;
            float relative = delta / maxAbsDelta;
            if (relative < -1.0f) relative = -1.0f;
            if (relative > 1.0f) relative = 1.0f;

            // Current value maps to the center of the scale. Bars themselves
            // always grow from the bottom zero/base line up to that level.
            int16_t topY = centerY -
                static_cast<int16_t>(
                    relative * static_cast<float>(halfH));
            if (topY < plotTop) topY = plotTop;
            if (topY > plotBottom) topY = plotBottom;

            const uint8_t slot =
                static_cast<uint8_t>(firstSlot + i);
            const int16_t slotLeft =
                left + static_cast<int16_t>(
                    (static_cast<uint32_t>(slot) * plotW) / slotCount);
            const int16_t slotRight =
                left + static_cast<int16_t>(
                    (static_cast<uint32_t>(slot + 1) * plotW) / slotCount);
            int16_t barW = slotRight - slotLeft - 1;
            if (barW < 1) barW = 1;

            int16_t barH = plotBottom - topY + 1;
            if (barH < 1) barH = 1;
            display.fillRect(
                slotLeft, topY,
                barW, barH, textColor);
        }
        return;
    }

    // Keep the framed line graph behavior for users who explicitly select it.
    display.drawRect(x, graphY, element.width, graphH, textColor);
    if (count < 2) return;

    float minValue = 0.0f;
    float maxValue = 0.0f;
    bool firstValue = true;
    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValueAt(dm, element.source, periodHours, i, value)) continue;
        if (firstValue) {
            minValue = maxValue = value;
            firstValue = false;
        } else {
            if (value < minValue) minValue = value;
            if (value > maxValue) maxValue = value;
        }
    }
    if (firstValue) return;

    const bool sensorSource =
        element.source.startsWith("rf.") || element.source.startsWith("inside.");
    if (!sensorSource && minValue > 0.0f) minValue = 0.0f;
    if (maxValue - minValue < 0.01f) {
        minValue -= 0.5f;
        maxValue += 0.5f;
    }

    const int16_t left = x + 2;
    const int16_t top = graphY + 2;
    int16_t plotW = element.width - 4;
    int16_t plotH = graphH - 4;
    if (plotW < 1) plotW = 1;
    if (plotH < 1) plotH = 1;

    int16_t previousX = left;
    int16_t previousY = top + plotH - 1;
    bool previousValid = false;

    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValueAt(dm, element.source, periodHours, i, value)) continue;
        const int16_t px = left + static_cast<int16_t>(
            (static_cast<uint32_t>(i) * (plotW - 1)) / (count - 1));
        float normalized = (value - minValue) / (maxValue - minValue);
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        const int16_t py = top + plotH - 1 -
            static_cast<int16_t>(normalized * (plotH - 1));
        if (previousValid)
            display.drawLine(previousX, previousY, px, py, textColor);
        previousX = px;
        previousY = py;
        previousValid = true;
    }
}

inline void drawElements(IDisplay& display, const DataModel& dm,
                         const HomeLayoutWidgetConfig& widget, uint16_t textColor) {
    uint8_t sharedHistoryPeriod = 12;
    for (const CustomWidgetElementConfig& candidate : widget.elements) {
        if ((candidate.type == "sparkline" ||
             candidate.type == "trend" ||
             candidate.type == "minmax") &&
            sensorGraphPeriodIndex(candidate.graphPeriodHours) >= 0) {
            sharedHistoryPeriod = candidate.graphPeriodHours;
            break;
        }
    }
    const uint8_t historyPeriodHours =
        normalizedGraphPeriodHours(sharedHistoryPeriod);

    // elements[] is the Z-order: first is bottom, last is top.
    for (uint8_t i = 0; i < widget.elements.size() && i < MaxCustomWidgetElements; ++i) {
        const CustomWidgetElementConfig& element = widget.elements[i];
        const int16_t x = widget.x + element.x;
        const int16_t y = widget.y + element.y;

        if (element.type == "text") drawText(display, x, y, element, textColor);
        else if (element.type == "kpi") drawKpi(display, dm, x, y, element, textColor);
        else if (element.type == "progress") drawProgress(display, dm, x, y, element, textColor);
        else if (element.type == "sparkline")
            drawSparkline(display, dm, x, y, element, historyPeriodHours, textColor);
        else if (element.type == "trend")
            drawTrend(display, dm, x, y, element, historyPeriodHours, textColor);
        else if (element.type == "minmax")
            drawMinMax(display, dm, x, y, element, historyPeriodHours, textColor);
    }
}

inline void draw(
    IDisplay& display,
    const DataModel& dm,
    const HomeLayoutWidgetConfig& widget,
    const char* fallbackTitle = "VLASTNÍ") {
    const bool blackBackground = widget.background == "black";
    const uint16_t textColor = widget.inverseText ? 1 : 0;
    const WidgetIcons::Icon icon =
        WidgetIcons::resolved(widget.icon, widget.type);
    const bool hasIcon = icon != WidgetIcons::Icon::None;

    ScreenStyle::drawStyledCard(
        display, widget.x, widget.y, widget.width, widget.height,
        widget.title.isEmpty() ? fallbackTitle : widget.title.c_str(),
        widget.showFrame, blackBackground, widget.inverseText,
        hasIcon ? 51 : 12,
        hasIcon ? 51 : 10);

    if (hasIcon) {
        WidgetIcons::draw(
            display, icon,
            widget.x + 8, widget.y + 2,
            textColor);
    }

    drawElements(display, dm, widget, textColor);
}

} // namespace CustomWidgetRenderer
