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

inline void drawText(IDisplay& display, int16_t x, int16_t y,
                     const CustomWidgetElementConfig& element, uint16_t textColor) {
    if (element.fontSize != "auto") {
        const uint8_t fontPx = requestedFontPx(element, false);
        const String text = fitScaledText(element.text, element.width, fontPx, false);
        const int16_t textWidth = scaledTextWidth(text, fontPx, false);
        const int16_t textX = alignedScaledX(x, element.width, textWidth, element.align);
        if (drawScaledUnicodeText(display, textX, y, text, fontPx, false, textColor, element.height)) {
            return;
        }
    }

    useElementFont(display, element, false, textColor);
    const String text = fitText(display, element.text, element.width);
    const int16_t textX = alignedX(display, x, element.width, text, element.align);
    display.setCursor(textX, y + baselineOffset(element, false));
    display.print(text);
}

inline void drawKpi(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                    const CustomWidgetElementConfig& element, uint16_t textColor) {
    int16_t valueY = y;
    if (element.showLabel && !element.label.isEmpty()) {
        ScreenStyle::useBody(display, textColor);
        const String label = fitText(display, element.label, element.width);
        const int16_t labelX = alignedX(display, x, element.width, label, element.align);
        display.setCursor(labelX, y + 16);
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
        const int16_t availableHeight = element.height - (valueY - y);
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

inline uint8_t historyCount(const DataModel& dm, const String& source) {
    if (source == "solar.productionPowerW" || source == "solar.houseConsumptionW")
        return dm.solar.historyCount;

    if (source == "inside.temperatureC" ||
        source == "inside.humidityPercent" ||
        source == "inside.pressureHpa") {
        return dm.inside.history != nullptr ? dm.inside.history->count : 0;
    }

    if (source == "inside.temperatureC" ||
        source == "inside.humidityPercent" ||
        source == "inside.pressureHpa") {
        if (dm.inside.history == nullptr ||
            chronologicalIndex >= dm.inside.history->count) {
            return false;
        }
        const InsideHistory& history = *dm.inside.history;
        const uint8_t oldest =
            static_cast<uint8_t>((history.next + InsideHistorySampleCount - history.count) %
                                 InsideHistorySampleCount);
        const uint8_t physical =
            static_cast<uint8_t>((oldest + chronologicalIndex) % InsideHistorySampleCount);
        const InsideHistorySample& sample = history.samples[physical];
        if (source == "inside.temperatureC") value = sample.temperatureCenti / 100.0f;
        else if (source == "inside.humidityPercent") value = sample.humidityPercent;
        else value = sample.pressureDeciHpa / 10.0f;
        return true;
    }

    int stableIndex = -1;
    String metric;
    if (!rfHistorySource(source, stableIndex, metric) ||
        dm.rfSensors.history == nullptr) {
        return 0;
    }
    return dm.rfSensors.history[stableIndex].count;
}

inline bool historyValueAt(const DataModel& dm, const String& source,
                           uint8_t chronologicalIndex, float& value) {
    if (source == "solar.productionPowerW" || source == "solar.houseConsumptionW") {
        if (chronologicalIndex >= dm.solar.historyCount) return false;
        const SolarHistorySample& sample = dm.solar.history[chronologicalIndex];
        if (source == "solar.productionPowerW") value = sample.productionPowerW;
        else value = sample.houseConsumptionW;
        return true;
    }

    int stableIndex = -1;
    String metric;
    if (!rfHistorySource(source, stableIndex, metric) ||
        dm.rfSensors.history == nullptr) {
        return false;
    }

    const RfSensorHistory& history = dm.rfSensors.history[stableIndex];
    if (chronologicalIndex >= history.count) return false;

    const uint8_t oldest =
        static_cast<uint8_t>((history.next + RfHistorySampleCount - history.count) %
                             RfHistorySampleCount);
    const uint8_t physical =
        static_cast<uint8_t>((oldest + chronologicalIndex) % RfHistorySampleCount);
    const RfHistorySample& sample = history.samples[physical];

    if (metric == "temperatureC") {
        if ((sample.flags & 0x01) == 0) return false;
        value = sample.temperatureCenti / 100.0f;
        return true;
    }
    if ((sample.flags & 0x02) == 0) return false;
    value = sample.humidityPercent;
    return true;
}

inline void drawTrend(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                      const CustomWidgetElementConfig& element, uint16_t textColor) {
    const uint8_t count = historyCount(dm, element.source);
    int direction = 0;
    if (count >= 2) {
        float previous = 0.0f;
        float current = 0.0f;
        if (historyValueAt(dm, element.source, count - 2, previous) &&
            historyValueAt(dm, element.source, count - 1, current)) {
            float threshold = 0.15f;
            if (element.source.endsWith(".humidityPercent")) threshold = 1.0f;
            else if (element.source.endsWith(".pressureHpa")) threshold = 0.5f;
            const float delta = current - previous;
            if (delta > threshold) direction = 1;
            else if (delta < -threshold) direction = -1;
        }
    }

    int16_t top = y;
    int16_t availableH = element.height;
    if (element.showLabel && !element.label.isEmpty()) {
        ScreenStyle::useBody(display, textColor);
        const String label = fitText(display, element.label, element.width);
        const int16_t labelX = alignedX(display, x, element.width, label, element.align);
        display.setCursor(labelX, y + 15);
        display.print(label);
        top += 20;
        availableH -= 20;
    }
    if (availableH < 8) return;

    int16_t size = min<int16_t>(element.width, availableH);
    const uint8_t requested = requestedFontPx(element, true);
    if (requested > 0 && requested < size) size = requested;
    if (size < 10) size = 10;

    int16_t left = x;
    if (element.align == "center") left = x + (element.width - size) / 2;
    else if (element.align == "right") left = x + element.width - size;
    const int16_t cx = left + size / 2;
    const int16_t cy = top + availableH / 2;
    const int16_t half = max<int16_t>(4, size / 3);

    if (direction > 0) {
        display.drawLine(cx, cy + half, cx, cy - half, textColor);
        display.drawLine(cx, cy - half, cx - half / 2, cy - half / 2, textColor);
        display.drawLine(cx, cy - half, cx + half / 2, cy - half / 2, textColor);
    } else if (direction < 0) {
        display.drawLine(cx, cy - half, cx, cy + half, textColor);
        display.drawLine(cx, cy + half, cx - half / 2, cy + half / 2, textColor);
        display.drawLine(cx, cy + half, cx + half / 2, cy + half / 2, textColor);
    } else {
        display.drawLine(cx - half, cy, cx + half, cy, textColor);
        display.drawLine(cx + half, cy, cx + half / 2, cy - half / 2, textColor);
        display.drawLine(cx + half, cy, cx + half / 2, cy + half / 2, textColor);
    }
}

inline void drawSparkline(IDisplay& display, const DataModel& dm, int16_t x, int16_t y,
                          const CustomWidgetElementConfig& element, uint16_t textColor) {
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
    display.drawRect(x, graphY, element.width, graphH, textColor);

    const uint8_t count = historyCount(dm, element.source);
    if (count < 2) return;

    float minValue = 0.0f;
    float maxValue = 0.0f;
    bool firstValue = true;
    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValueAt(dm, element.source, i, value)) continue;
        if (firstValue) {
            minValue = maxValue = value;
            firstValue = false;
        } else {
            if (value < minValue) minValue = value;
            if (value > maxValue) maxValue = value;
        }
    }
    if (firstValue) return;

    const bool rfSource = element.source.startsWith("rf.");
    if (!rfSource && minValue > 0.0f) minValue = 0.0f;
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

    if (element.graphStyle == "bars") {
        int16_t barWidth = count > 0 ? plotW / count : 1;
        if (barWidth < 1) barWidth = 1;
        for (uint8_t i = 0; i < count; ++i) {
            float value = 0.0f;
            if (!historyValueAt(dm, element.source, i, value)) continue;
            float normalized = (value - minValue) / (maxValue - minValue);
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            const int16_t barH = static_cast<int16_t>(normalized * (plotH - 1));
            const int16_t px = left + static_cast<int16_t>((static_cast<uint32_t>(i) * plotW) / count);
            if (barH > 0) {
                const int16_t w = barWidth > 1 ? barWidth - 1 : 1;
                display.fillRect(px, top + plotH - barH, w, barH, textColor);
            }
        }
        return;
    }

    int16_t previousX = left;
    int16_t previousY = top + plotH - 1;
    bool previousValid = false;

    for (uint8_t i = 0; i < count; ++i) {
        float value = 0.0f;
        if (!historyValueAt(dm, element.source, i, value)) continue;
        const int16_t px = left + static_cast<int16_t>((static_cast<uint32_t>(i) * (plotW - 1)) / (count - 1));
        float normalized = (value - minValue) / (maxValue - minValue);
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        const int16_t py = top + plotH - 1 - static_cast<int16_t>(normalized * (plotH - 1));
        if (previousValid) display.drawLine(previousX, previousY, px, py, textColor);
        previousX = px;
        previousY = py;
        previousValid = true;
    }
}

inline void drawElements(IDisplay& display, const DataModel& dm,
                         const HomeLayoutWidgetConfig& widget, uint16_t textColor) {
    // elements[] is the Z-order: first is bottom, last is top.
    for (uint8_t i = 0; i < widget.elements.size() && i < MaxCustomWidgetElements; ++i) {
        const CustomWidgetElementConfig& element = widget.elements[i];
        const int16_t x = widget.x + element.x;
        const int16_t y = widget.y + element.y;

        if (element.type == "text") drawText(display, x, y, element, textColor);
        else if (element.type == "kpi") drawKpi(display, dm, x, y, element, textColor);
        else if (element.type == "progress") drawProgress(display, dm, x, y, element, textColor);
        else if (element.type == "sparkline") drawSparkline(display, dm, x, y, element, textColor);
        else if (element.type == "trend") drawTrend(display, dm, x, y, element, textColor);
    }
}

inline void draw(IDisplay& display, const DataModel& dm, const HomeLayoutWidgetConfig& widget) {
    const bool blackBackground = widget.background == "black";
    const uint16_t textColor = widget.inverseText ? 1 : 0;
    ScreenStyle::drawStyledCard(
        display, widget.x, widget.y, widget.width, widget.height,
        widget.title.isEmpty() ? "VLASTNÍ" : widget.title.c_str(),
        widget.showFrame, blackBackground, widget.inverseText);
    drawElements(display, dm, widget, textColor);
}

} // namespace CustomWidgetRenderer
