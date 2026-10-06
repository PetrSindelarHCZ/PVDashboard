#include "HomeScreen.h"
#include "ScreenStyle.h"
#include "../layout/HomeLayout.h"
#include "../layout/CustomWidgetRenderer.h"
#include "../display/assets/WidgetIcons.h"
#include "../display/EInkGraph.h"

namespace {

uint16_t cardTextColor(const HomeLayoutWidgetConfig* style) {
    return style != nullptr && style->inverseText ? 1 : 0;
}

void drawHomeCardBackground(IDisplay& display, const LayoutWidget& widget,
                            const HomeLayoutWidgetConfig* style, const char* title,
                            bool hasHeaderIcon = false) {
    const bool showFrame = style == nullptr ? true : style->showFrame;
    const bool blackBackground = style != nullptr && style->background == "black";
    const bool inverseText = style != nullptr && style->inverseText;
    ScreenStyle::drawStyledCard(display, widget.x, widget.y, widget.width, widget.height,
                                title, showFrame, blackBackground, inverseText,
                                hasHeaderIcon ? 50 : 12,
                                hasHeaderIcon ? 50 : 10);
}

WidgetIcons::Icon resolvedWidgetIcon(
    const HomeLayoutWidgetConfig* style,
    const char* widgetType) {
    const uint8_t configured =
        style != nullptr
            ? style->icon
            : static_cast<uint8_t>(WidgetIcons::Icon::Auto);
    return WidgetIcons::resolved(configured, String(widgetType));
}

void drawWidgetHeaderIcon(
    IDisplay& d,
    const LayoutWidget& widget,
    WidgetIcons::Icon icon,
    uint16_t color = 0) {
    if (icon == WidgetIcons::Icon::None) return;
    WidgetIcons::draw(d, icon, widget.x + 8, widget.y + 2, color);
}

void drawCardIcon(IDisplay& d, int16_t centerX, int16_t centerY,
                  SidebarIcons::Icon iconId, uint16_t color = 0) {
    const SidebarIcons::Bitmap icon = SidebarIcons::get(iconId);
    if (icon.data == nullptr) return;
    d.drawBitmap(centerX - icon.width / 2,
                 centerY - icon.height / 2,
                 icon.data,
                 icon.width,
                 icon.height,
                 color);
}

void drawCardHeaderIcon(IDisplay& d, const LayoutWidget& widget,
                        SidebarIcons::Icon iconId, uint16_t color = 0) {
    const SidebarIcons::Bitmap icon = SidebarIcons::get(iconId);
    if (icon.data == nullptr || icon.width != 40 || icon.height != 40) return;

    // Header icon intentionally follows the visual mock-up:
    // larger symbol on the left, title starts beside it.
    constexpr int16_t target = 32;
    constexpr int16_t sourceRowBytes = 5;
    const int16_t left = widget.x + 8;
    const int16_t top = widget.y + 2;

    for (int16_t ty = 0; ty < target; ++ty) {
        for (int16_t tx = 0; tx < target; ++tx) {
            const int16_t sx0 = (tx * 40) / target;
            const int16_t sx1 = ((tx + 1) * 40) / target;
            const int16_t sy0 = (ty * 40) / target;
            const int16_t sy1 = ((ty + 1) * 40) / target;
            bool set = false;

            for (int16_t sy = sy0; sy < sy1 && !set; ++sy) {
                for (int16_t sx = sx0; sx < sx1; ++sx) {
                    const uint8_t value =
                        pgm_read_byte(icon.data + sy * sourceRowBytes + sx / 8);
                    if (value & (0x80 >> (sx & 7))) {
                        set = true;
                        break;
                    }
                }
            }
            if (set) d.drawPixel(left + tx, top + ty, color);
        }
    }
}

void drawRfHeaderIcon(IDisplay& d, const LayoutWidget& widget, uint16_t color = 0) {
    const int16_t x = widget.x + 8;
    const int16_t y = widget.y + 2;
    d.drawLine(x + 15, y + 9, x + 15, y + 29, color);
    d.fillCircle(x + 15, y + 30, 1, color);
    d.drawLine(x + 10, y + 11, x + 5, y + 6, color);
    d.drawLine(x + 20, y + 11, x + 25, y + 6, color);
    d.drawLine(x + 7, y + 16, x + 1, y + 10, color);
    d.drawLine(x + 23, y + 16, x + 29, y + 10, color);
}

void drawHouseSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t color = 0) {
    d.drawLine(x + 2, y + 12, x + 15, y + 2, color);
    d.drawLine(x + 15, y + 2, x + 28, y + 12, color);
    d.drawLine(x + 5, y + 11, x + 5, y + 28, color);
    d.drawLine(x + 25, y + 11, x + 25, y + 28, color);
    d.drawLine(x + 5, y + 28, x + 25, y + 28, color);
    d.fillRect(x + 13, y + 19, 5, 9, color);
}

void drawPoolSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t color = 0) {
    d.drawLine(x + 8, y + 3, x + 8, y + 18, color);
    d.drawLine(x + 19, y + 3, x + 19, y + 18, color);
    d.drawLine(x + 8, y + 4, x + 19, y + 4, color);
    d.drawLine(x + 8, y + 10, x + 19, y + 10, color);
    for (int16_t offset = 0; offset < 2; ++offset) {
        d.drawLine(x + 2, y + 22 + offset, x + 7, y + 20 + offset, color);
        d.drawLine(x + 7, y + 20 + offset, x + 12, y + 22 + offset, color);
        d.drawLine(x + 12, y + 22 + offset, x + 17, y + 20 + offset, color);
        d.drawLine(x + 17, y + 20 + offset, x + 22, y + 22 + offset, color);
        d.drawLine(x + 22, y + 22 + offset, x + 28, y + 20 + offset, color);
    }
}

void drawOkSymbol(IDisplay& d, int16_t x, int16_t y, uint16_t color = 0) {
    d.drawCircle(x + 14, y + 14, 13, color);
    d.drawCircle(x + 14, y + 14, 12, color);
    d.drawLine(x + 7, y + 14, x + 12, y + 19, color);
    d.drawLine(x + 12, y + 19, x + 22, y + 8, color);
    d.drawLine(x + 7, y + 15, x + 12, y + 20, color);
    d.drawLine(x + 12, y + 20, x + 22, y + 9, color);
}

void drawMiniBars(IDisplay& d, int16_t x, int16_t y, int16_t w, int16_t h,
                  const SolarData& solar, uint16_t color = 0) {
    if (solar.historyCount < 2 || w <= 0 || h <= 0) return;

    float maxValue = 500.0f;
    for (uint8_t i = 0; i < solar.historyCount; ++i) {
        if (solar.history[i].productionPowerW > maxValue)
            maxValue = solar.history[i].productionPowerW;
    }

    const uint8_t maxBars = static_cast<uint8_t>(w / 4);
    if (maxBars == 0) return;
    const uint8_t step = solar.historyCount > maxBars
        ? static_cast<uint8_t>((solar.historyCount + maxBars - 1) / maxBars)
        : 1;
    const uint8_t bars = static_cast<uint8_t>((solar.historyCount + step - 1) / step);

    for (uint8_t b = 0, i = 0; b < bars && i < solar.historyCount; ++b, i = static_cast<uint8_t>(i + step)) {
        float value = 0.0f;
        const uint8_t end = min<uint8_t>(solar.historyCount, static_cast<uint8_t>(i + step));
        for (uint8_t j = i; j < end; ++j)
            if (solar.history[j].productionPowerW > value) value = solar.history[j].productionPowerW;

        int16_t bh = static_cast<int16_t>((value / maxValue) * h);
        if (bh < 1 && value > 0.0f) bh = 1;
        if (bh > h) bh = h;
        const int16_t bx = x + (b * w) / bars;
        d.fillRect(bx, y + h - bh, 2, bh, color);
    }
}

void drawPhaseBars(IDisplay& d, int16_t x, int16_t y, int16_t h,
                   const AZRouterData& az, uint16_t color = 0) {
    float maxValue = 1.0f;
    for (uint8_t phase = 0; phase < 3; ++phase) {
        if (az.hasRoutedPhasePower[phase] && az.routedPhasePowerW[phase] > maxValue)
            maxValue = az.routedPhasePowerW[phase];
    }

    for (uint8_t phase = 0; phase < 3; ++phase) {
        const float value = az.hasRoutedPhasePower[phase] ? az.routedPhasePowerW[phase] : 0.0f;
        int16_t bh = static_cast<int16_t>((value / maxValue) * h);
        if (bh < 1 && value > 0.0f) bh = 1;
        d.drawRect(x + phase * 12, y, 7, h, color);
        if (bh > 0) d.fillRect(x + 1 + phase * 12, y + h - bh + 1, 5, bh - 1, color);
    }
}

void drawWeatherCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                     const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    const uint16_t color = cardTextColor(style);
    const WidgetIcons::Icon icon = resolvedWidgetIcon(style, "weather");
    drawHomeCardBackground(
        display, widget, style, "PŘEDPOVĚĎ",
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    if (h < 250) {
        if (dm.weather.status.available) {
            ScreenStyle::drawWeatherSymbol(display, x + w - 48, y + 72, dm.weather.weatherCode,
                                          IconAssets::WeatherSize::Medium40, color);
            ScreenStyle::useMetric(display, color);
            display.setCursor(x + 15, y + 88);
            display.printf("%.1f °C", dm.weather.outdoorTempC);

            ScreenStyle::useBody(display, color);
            display.setCursor(x + 15, y + 125);
            display.printf("Vlhkost %d %%", dm.weather.outdoorHumidityPercent);
            display.setCursor(x + 15, y + 153);
            display.printf("Max %.0f / Min %.0f °C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);

            const String location = dm.weather.locationName.isEmpty() ? String("Online předpověď") : dm.weather.locationName;
            display.setCursor(x + 15, y + 183);
            display.print(location);
        } else {
            ScreenStyle::useValue(display, color);
            display.setCursor(x + 15, y + 88);
            display.print("--.- °C");
            ScreenStyle::useBody(display, color);
            display.setCursor(x + 15, y + 130);
            display.print("Předpověď nedostupná");
        }

        if (!dm.weather.provider.isEmpty()) {
            ScreenStyle::useBody(display, color);
            const String providerLabel = "Zdroj: " + dm.weather.provider;
            display.setCursor(x + 15, y + h - 14);
            display.print(providerLabel);
        }
        return;
    }

    if (dm.weather.status.available) {
        ScreenStyle::drawWeatherSymbol(display, x + w - 52, y + 67, dm.weather.weatherCode,
                                      IconAssets::WeatherSize::Medium40, color);

        ScreenStyle::useMetric(display, color);
        display.setCursor(x + 15, y + 82);
        display.printf("%.1f °C", dm.weather.outdoorTempC);

        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 122);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(x + 15, y + 172);
        display.print("Dnešní rozsah");
        ScreenStyle::useValue(display, color);
        display.setCursor(x + 15, y + 207);
        display.printf("%.0f / %.0f °C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 262);
        display.printf("Stav: %s", dm.weather.conditionText.c_str());
        display.setCursor(x + 15, y + 312);
        display.print(dm.weather.locationName.isEmpty() ? dm.weather.provider : dm.weather.locationName);
    } else {
        ScreenStyle::useValue(display, color);
        display.setCursor(x + 15, y + 82);
        display.print("--.- °C");
        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 132);
        display.print("Počasí nedostupné");
        display.setCursor(x + 15, y + 172);
        display.print(dm.weather.status.lastError);
    }

    if (!dm.weather.provider.isEmpty()) {
        ScreenStyle::useBody(display, color);
        const String providerLabel = "Data: " + dm.weather.provider;
        display.setCursor(x + w - 10 - display.textWidth(providerLabel), y + h - 20);
        display.print(providerLabel);
    }
}

void drawEnergyCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                    const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    const uint16_t color = cardTextColor(style);
    drawHomeCardBackground(display, widget, style, "ENERGIE");

    if (w >= 400) {
        const int16_t leftX = x + 20;
        const int16_t rightX = x + w / 2 + 10;

        ScreenStyle::useBody(display, color);
        display.setCursor(leftX, y + 62);
        display.print("Výroba FVE");
        ScreenStyle::useValue(display, color);
        display.setCursor(leftX, y + 87);
        if (dm.solar.status.available) display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);
        else display.print("--.- kW");

        ScreenStyle::useBody(display, color);
        display.setCursor(leftX, y + 172);
        display.print("Spotřeba domu");
        ScreenStyle::useValue(display, color);
        display.setCursor(leftX, y + 197);
        if (dm.solar.status.available) display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);
        else display.print("--.- kW");

        ScreenStyle::useBody(display, color);
        display.setCursor(rightX, y + 62);
        display.print("Distribuce");
        ScreenStyle::useValue(display, color);
        display.setCursor(rightX, y + 87);
        if (dm.solar.status.available) display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);
        else display.print("--.- kW");

        ScreenStyle::useBody(display, color);
        display.setCursor(rightX, y + 172);
        display.print("Baterie");
        ScreenStyle::useValue(display, color);
        display.setCursor(rightX, y + 197);
        if (dm.solar.status.available) display.printf("%.0f %%", dm.solar.batterySocPercent);
        else display.print("-- %");
        ScreenStyle::useBody(display, color);
        display.setCursor(rightX, y + 237);
        if (dm.solar.status.available) display.printf("%+.0f W", dm.solar.batteryPowerW);
        else display.print("-- W");
        return;
    }

    const int16_t valueX = x + 15;

    ScreenStyle::useBody(display, color);
    display.setCursor(valueX, y + 62);
    display.print("Výroba FVE");
    ScreenStyle::useValue(display, color);
    display.setCursor(valueX, y + 87);
    if (dm.solar.status.available) display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);
    else display.print("--.- kW");

    ScreenStyle::useBody(display, color);
    display.setCursor(valueX, y + 132);
    display.print("Spotřeba domu");
    ScreenStyle::useValue(display, color);
    display.setCursor(valueX, y + 157);
    if (dm.solar.status.available) display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);
    else display.print("--.- kW");

    ScreenStyle::useBody(display, color);
    display.setCursor(valueX, y + 202);
    display.print("Distribuce");
    ScreenStyle::useValue(display, color);
    display.setCursor(valueX, y + 227);
    if (dm.solar.status.available) display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);
    else display.print("--.- kW");

    ScreenStyle::useBody(display, color);
    display.setCursor(valueX, y + 272);
    display.print("Baterie");
    ScreenStyle::useValue(display, color);
    display.setCursor(valueX, y + 297);
    if (dm.solar.status.available)
        display.printf("%.0f %% (%+.0f W)", dm.solar.batterySocPercent, dm.solar.batteryPowerW);
    else
        display.print("-- % (-- W)");
}

void drawIndoorCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                    const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    const uint16_t color = cardTextColor(style);
    const String title =
        style != nullptr && !style->title.isEmpty() ? style->title : String("UVNITŘ");
    const WidgetIcons::Icon icon = resolvedWidgetIcon(style, "indoor");
    drawHomeCardBackground(
        display, widget, style, title.c_str(),
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    if (style != nullptr && !style->elements.empty()) {
        CustomWidgetRenderer::drawElements(display, dm, *style, color);
        return;
    }

    auto drawBmeTemperature = [&](int16_t valueX, int16_t valueY) {
        ScreenStyle::useValue(display, color);
        display.setCursor(valueX, valueY);
        if (dm.inside.status.available) {
            display.printf("%.1f °C", dm.inside.temperatureC);
        } else {
            display.print("--.- °C");
        }
    };

    auto drawBmeDetails = [&](int16_t valueX, int16_t humidityY, int16_t pressureY) {
        ScreenStyle::useBody(display, color);
        display.setCursor(valueX, humidityY);
        if (dm.inside.status.available) {
            display.printf("Vlhkost: %d %%", dm.inside.humidityPercent);
        } else {
            display.print("Vlhkost: -- %");
        }

        display.setCursor(valueX, pressureY);
        if (dm.inside.status.available) {
            display.printf("Tlak: %.0f hPa", dm.inside.pressureHpa);
        } else {
            display.print("Tlak: ---- hPa");
        }
    };

    if (h < 220) {
        ScreenStyle::useMetric(display, color);
        display.setCursor(x + 15, y + 90);
        if (dm.inside.status.available)
            display.printf("%.1f °C", dm.inside.temperatureC);
        else
            display.print("--.- °C");

        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 118);
        if (dm.inside.status.available)
            display.printf("Vlhkost %d %%", dm.inside.humidityPercent);
        else
            display.print("Vlhkost -- %");

        display.setCursor(x + 15, y + 147);
        if (dm.inside.status.available)
            display.printf("Tlak %.0f hPa", dm.inside.pressureHpa);
        else
            display.print("Tlak ---- hPa");
        return;
    }

    if (w >= 500) {
        const int16_t leftX = x + 20;
        const int16_t rightX = x + w / 2 + 10;

        ScreenStyle::useBody(display, color);
        display.setCursor(leftX, y + 62);
        display.print("Obývák");
        drawBmeTemperature(leftX, y + 87);
        drawBmeDetails(leftX, y + 132, y + 167);

        ScreenStyle::useBody(display, color);
        display.setCursor(rightX, y + 62);
        display.print("Ložnice");
        ScreenStyle::useValue(display, color);
        display.setCursor(rightX, y + 87);
        display.printf("%.1f °C", dm.inside.bedroomTempC);

        if (dm.pool.enabled) {
            ScreenStyle::useBody(display, color);
            display.setCursor(rightX, y + 172);
            display.print("Bazén");
            ScreenStyle::useValue(display, color);
            display.setCursor(rightX, y + 197);
            display.printf("%.1f °C", dm.inside.poolTempC);
        }
        return;
    }

    const int16_t valueX = x + 15;

    ScreenStyle::useBody(display, color);
    display.setCursor(valueX, y + 62);
    display.print("Obývák");
    drawBmeTemperature(valueX, y + 87);
    drawBmeDetails(valueX, y + 132, y + 167);

    if (h >= 250) {
        ScreenStyle::useBody(display, color);
        display.setCursor(valueX, y + 205);
        display.print("Ložnice");
        ScreenStyle::useValue(display, color);
        display.setCursor(valueX, y + 230);
        display.printf("%.1f °C", dm.inside.bedroomTempC);
    }

    if (dm.pool.enabled && h >= 350) {
        ScreenStyle::useBody(display, color);
        display.setCursor(valueX, y + 285);
        display.print("Bazén");
        ScreenStyle::useValue(display, color);
        display.setCursor(valueX, y + 310);
        display.printf("%.1f °C", dm.inside.poolTempC);
    }
}


void drawFveSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                        const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const uint16_t color = cardTextColor(style);
    const WidgetIcons::Icon icon = resolvedWidgetIcon(style, "fve-summary");
    drawHomeCardBackground(
        display, widget, style, "FVE / GOODWE",
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    ScreenStyle::useMetric(display, color);
    display.setCursor(x + 18, y + 91);
    if (dm.solar.status.available)
        display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);
    else
        display.print("--.- kW");

    drawMiniBars(display, x + 150, y + 58, 54, 38, dm.solar, color);

    ScreenStyle::useBody(display, color);
    display.setCursor(x + 18, y + 132);
    if (dm.solar.status.available)
        display.printf("Dnes %.1f kWh", dm.solar.energyTodayKWh);
    else
        display.print("Dnes --.- kWh");

    display.setCursor(x + 18, y + 162);
    if (dm.solar.status.available)
        display.printf("Síť %+.1f kW", dm.solar.gridPowerW / 1000.0f);
    else
        display.print("Síť --.- kW");

    display.setCursor(x + 18, y + 192);
    if (dm.solar.status.available && dm.solar.batteryPresent)
        display.printf("Baterie %.0f %%", dm.solar.batterySocPercent);
    else if (dm.solar.status.available)
        display.print("Bez baterie");
    else
        display.print("GoodWe offline");
}

void drawAZRouterSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                             const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const uint16_t color = cardTextColor(style);
    const WidgetIcons::Icon icon = resolvedWidgetIcon(style, "azrouter-summary");
    drawHomeCardBackground(
        display, widget, style, "AZROUTER",
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    ScreenStyle::useMetric(display, color);
    display.setCursor(x + 18, y + 91);
    if (dm.azrouter.status.available && dm.azrouter.hasRoutedPower)
        display.printf("%.0f W", dm.azrouter.routedPowerW);
    else
        display.print("-- W");

    drawPhaseBars(display, x + 176, y + 56, 38, dm.azrouter, color);

    ScreenStyle::useBody(display, color);
    display.setCursor(x + 18, y + 132);
    if (dm.azrouter.hasRoutedEnergyToday)
        display.printf("Dnes %.1f kWh", dm.azrouter.routedEnergyTodayKWh);
    else
        display.print("Dnes --.- kWh");

    display.setCursor(x + 18, y + 162);
    if (dm.azrouter.hasGridPower)
        display.printf("Síť %+.0f W", dm.azrouter.gridPowerW);
    else
        display.print("Síť -- W");

    display.setCursor(x + 18, y + 192);
    if (dm.azrouter.hasSystemTemp)
        display.printf("%.1f °C", dm.azrouter.systemTempC);
    else
        display.print(dm.azrouter.status.available ? "Online" : "Offline");
}

const RfSensorData* rfSensorForWidget(const DataModel& dm, const HomeLayoutWidgetConfig* style) {
    if (style != nullptr && style->rfSensorSlot > 0) {
        const String slotId = HomeLayout::rfSlotId(style->rfSensorSlot);
        for (uint8_t i = 0; i < dm.rfSensors.sensorCount && i < MaxRfSensors; ++i) {
            const RfSensorData& sensor = dm.rfSensors.sensors[i];
            if (sensor.slotId == slotId) return &sensor;
        }
        return nullptr;
    }

    for (uint8_t i = 0; i < dm.rfSensors.sensorCount && i < MaxRfSensors; ++i) {
        const RfSensorData& sensor = dm.rfSensors.sensors[i];
        if (sensor.configured && sensor.hasTemperature) return &sensor;
    }
    return nullptr;
}

void drawRfSensorCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                      const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const uint16_t color = cardTextColor(style);
    const String title =
        style != nullptr && !style->title.isEmpty() ? style->title : String("VENKU");
    const WidgetIcons::Icon icon = resolvedWidgetIcon(style, "rf-sensor");
    drawHomeCardBackground(
        display, widget, style, title.c_str(),
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    if (style != nullptr && !style->elements.empty()) {
        CustomWidgetRenderer::drawElements(display, dm, *style, color);
        return;
    }

    const RfSensorData* sensor = rfSensorForWidget(dm, style);
    const bool showHumidity = style == nullptr || style->rfShowHumidity;
    const bool showLastSeen = style == nullptr || style->rfShowLastSeen;

    ScreenStyle::useMetric(display, color);
    display.setCursor(x + 15, y + 91);
    if (sensor != nullptr && sensor->available && sensor->hasTemperature)
        display.printf("%.1f °C", sensor->temperatureC);
    else
        display.print("--.- °C");

    ScreenStyle::useBody(display, color);
    int16_t lineY = y + 124;
    if (showHumidity) {
        display.setCursor(x + 15, lineY);
        if (sensor != nullptr && sensor->available && sensor->hasHumidity)
            display.printf("Vlhkost %d %%", sensor->humidityPercent);
        else
            display.print("Vlhkost -- %");
        lineY += 28;
    }

    if (showLastSeen) {
        display.setCursor(x + 15, lineY);
        if (sensor != nullptr && sensor->lastUpdateMs > 0) {
            const uint32_t nowMs = dm.system.uptimeSeconds * 1000UL;
            const uint32_t ageSeconds = nowMs >= sensor->lastUpdateMs
                ? (nowMs - sensor->lastUpdateMs) / 1000UL
                : 0;
            if (ageSeconds < 60)
                display.printf("Před %lu s", static_cast<unsigned long>(ageSeconds));
            else
                display.printf("Před %lu min", static_cast<unsigned long>(ageSeconds / 60));
        } else {
            display.print("Bez příjmu");
        }
    }
}

void drawPoolSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                         const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const uint16_t color = cardTextColor(style);
    const String title =
        style != nullptr && !style->title.isEmpty() ? style->title : String("BAZÉN");
    const WidgetIcons::Icon icon = resolvedWidgetIcon(style, "pool-summary");
    drawHomeCardBackground(
        display, widget, style, title.c_str(),
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    if (style != nullptr && !style->elements.empty()) {
        CustomWidgetRenderer::drawElements(display, dm, *style, color);
        return;
    }

    ScreenStyle::useMetric(display, color);
    display.setCursor(x + 15, y + 91);
    if (dm.pool.status.available)
        display.printf("%.1f °C", dm.pool.waterTempC);
    else
        display.printf("%.1f °C", dm.inside.poolTempC);

    ScreenStyle::useBody(display, color);
    display.setCursor(x + 15, y + 132);
    if (dm.pool.status.available)
        display.printf("Cíl %.1f °C", dm.pool.targetTempC);
    else
        display.print("Teplota čidla");

    display.setCursor(x + 15, y + 160);
    if (dm.pool.status.available)
        display.printf("Filtrace %s", dm.pool.filtrationRunning ? "ON" : "OFF");
    else
        display.print("Řízení bez dat");
}

void drawConsumptionSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                                const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const uint16_t color = cardTextColor(style);
    const WidgetIcons::Icon icon =
        resolvedWidgetIcon(style, "consumption-summary");
    drawHomeCardBackground(
        display, widget, style, "SPOTŘEBA DOMU",
        icon != WidgetIcons::Icon::None);
    drawWidgetHeaderIcon(display, widget, icon, color);

    ScreenStyle::useMetric(display, color);
    display.setCursor(x + 15, y + 90);
    if (dm.solar.status.available)
        display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);
    else
        display.print("--.- kW");

    if (dm.solar.historyCount > 1) {
        float maxPower = 500.0f;
        for (uint8_t i = 0; i < dm.solar.historyCount; ++i)
            if (dm.solar.history[i].houseConsumptionW > maxPower)
                maxPower = dm.solar.history[i].houseConsumptionW;

        const int16_t graphX = x + 15;
        const int16_t graphY = y + 112;
        const int16_t graphW = w - 30;
        const int16_t graphH = 42;
        display.drawLine(graphX, graphY + graphH, graphX + graphW, graphY + graphH, color);

        const uint8_t bars = 24;
        for (uint8_t b = 0; b < bars; ++b) {
            const uint8_t i = static_cast<uint8_t>((static_cast<uint16_t>(b) * dm.solar.historyCount) / bars);
            const float watts = dm.solar.history[i].houseConsumptionW;
            int16_t bh = static_cast<int16_t>((watts / maxPower) * graphH);
            if (bh < 1 && watts > 0.0f) bh = 1;
            if (bh > graphH) bh = graphH;
            const int16_t bx = graphX + (b * graphW) / bars;
            display.drawLine(bx, graphY + graphH, bx, graphY + graphH - bh, color);
        }
    } else {
        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 140);
        display.print("Historie se sbírá");
    }
}


void drawFlowArrow(IDisplay& d, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                   uint16_t color = 0) {
    d.drawLine(x1, y1, x2, y2, color);
    d.drawLine(x1, y1 + 1, x2, y2 + 1, color);
    const int16_t dx = x2 - x1;
    const int16_t dy = y2 - y1;
    if (abs(dx) >= abs(dy)) {
        const int16_t dir = dx >= 0 ? 1 : -1;
        d.drawLine(x2, y2, x2 - dir * 9, y2 - 6, color);
        d.drawLine(x2, y2, x2 - dir * 9, y2 + 6, color);
    } else {
        const int16_t dir = dy >= 0 ? 1 : -1;
        d.drawLine(x2, y2, x2 - 6, y2 - dir * 9, color);
        d.drawLine(x2, y2, x2 + 6, y2 - dir * 9, color);
    }
}

void drawEnergyFlowCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                        const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;
    const uint16_t color = cardTextColor(style);

    drawHomeCardBackground(
        display, widget, style,
        style != nullptr && !style->title.isEmpty()
            ? style->title.c_str()
            : "ENERGETICKÝ TOK");

    const int16_t cx = x + w / 2;
    const int16_t houseTop = y + 118;
    const int16_t houseBottom = y + h - 78;
    const int16_t houseHalfW = min<int16_t>(86, w / 5);

    // Stylized house.
    display.drawLine(cx - houseHalfW, houseTop + 42, cx, houseTop, color);
    display.drawLine(cx, houseTop, cx + houseHalfW, houseTop + 42, color);
    display.drawLine(cx - houseHalfW + 12, houseTop + 36,
                     cx - houseHalfW + 12, houseBottom, color);
    display.drawLine(cx + houseHalfW - 12, houseTop + 36,
                     cx + houseHalfW - 12, houseBottom, color);
    display.drawLine(cx - houseHalfW + 12, houseBottom,
                     cx + houseHalfW - 12, houseBottom, color);
    display.drawRect(cx - 14, houseBottom - 42, 28, 42, color);

    // Solar panel above the house.
    const int16_t panelW = 76;
    const int16_t panelH = 34;
    const int16_t panelX = cx - panelW / 2;
    const int16_t panelY = y + 58;
    display.drawRect(panelX, panelY, panelW, panelH, color);
    for (int16_t col = 1; col < 4; ++col)
        display.drawLine(panelX + col * panelW / 4, panelY,
                         panelX + col * panelW / 4, panelY + panelH, color);
    display.drawLine(panelX, panelY + panelH / 2,
                     panelX + panelW, panelY + panelH / 2, color);
    drawFlowArrow(display, cx, panelY + panelH + 4, cx, houseTop - 7, color);

    ScreenStyle::useBody(display, color);
    display.setCursor(panelX + panelW + 12, panelY + 4);
    display.print("FVE");
    ScreenStyle::useMetric(display, color);
    display.setCursor(panelX + panelW + 12, panelY + 28);
    if (dm.solar.status.available)
        display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);
    else
        display.print("--.- kW");

    // Grid on the left.
    const int16_t gridX = x + 28;
    const int16_t gridY = houseTop + 18;
    display.drawLine(gridX + 18, gridY, gridX, gridY + 56, color);
    display.drawLine(gridX + 18, gridY, gridX + 36, gridY + 56, color);
    display.drawLine(gridX + 18, gridY, gridX + 18, gridY + 58, color);
    display.drawLine(gridX + 5, gridY + 18, gridX + 31, gridY + 18, color);
    display.drawLine(gridX + 2, gridY + 34, gridX + 34, gridY + 34, color);

    const bool exporting = dm.solar.status.available && dm.solar.gridPowerW >= 0.0f;
    if (exporting)
        drawFlowArrow(display, cx - houseHalfW - 8, gridY + 34, gridX + 42, gridY + 34, color);
    else
        drawFlowArrow(display, gridX + 42, gridY + 34, cx - houseHalfW - 8, gridY + 34, color);

    ScreenStyle::useBody(display, color);
    display.setCursor(gridX + 44, gridY - 2);
    display.print("SÍŤ");
    ScreenStyle::useMetric(display, color);
    display.setCursor(gridX + 44, gridY + 24);
    if (dm.solar.status.available)
        display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);
    else
        display.print("--.- kW");
    ScreenStyle::useBody(display, color);
    display.setCursor(gridX + 44, gridY + 50);
    display.print(exporting ? "přetok" : "odběr");

    // AZRouter branch on the right.
    const int16_t azX = x + w - 132;
    const int16_t azY = gridY + 8;
    display.drawRoundRect(azX, azY, 40, 58, 4, color);
    display.drawLine(azX + 13, azY + 15, azX + 13, azY + 34, color);
    display.drawLine(azX + 20, azY + 12, azX + 20, azY + 37, color);
    display.drawLine(azX + 27, azY + 15, azX + 27, azY + 34, color);
    drawFlowArrow(display, cx + houseHalfW + 8, azY + 29, azX - 8, azY + 29, color);

    ScreenStyle::useBody(display, color);
    display.setCursor(azX + 48, azY + 4);
    display.print("AZ");
    ScreenStyle::useMetric(display, color);
    display.setCursor(azX + 48, azY + 30);
    if (dm.azrouter.status.available)
        display.printf("%.1f kW", dm.azrouter.routedPowerW / 1000.0f);
    else
        display.print("--.- kW");
    ScreenStyle::useBody(display, color);
    display.setCursor(azX + 48, azY + 54);
    if (dm.azrouter.hasRoutedEnergyToday)
        display.printf("%.1f kWh", dm.azrouter.routedEnergyTodayKWh);

    // Household load is the visual anchor at the bottom.
    const int16_t loadY = y + h - 54;
    drawFlowArrow(display, cx, houseBottom + 5, cx, loadY - 8, color);
    ScreenStyle::useBody(display, color);
    display.setCursor(x + 22, loadY);
    display.print("SPOTŘEBA DOMU");
    ScreenStyle::useValue(display, color);
    display.setCursor(x + w - 150, loadY + 2);
    if (dm.solar.status.available)
        display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);
    else
        display.print("--.- kW");

    // Battery state inside the house.
    ScreenStyle::useBody(display, color);
    display.setCursor(cx - 52, houseBottom - 58);
    if (dm.solar.status.available && dm.solar.batteryPresent) {
        display.printf("Bat %.0f%%", dm.solar.batterySocPercent);
    } else {
        display.print("Bez baterie");
    }
}


} // namespace

void HomeScreen::buildLayout(const DataModel& dm, ScreenLayout& layout) const {
    if (_layoutConfig != nullptr) {
        HomeLayout::buildResolved(*_layoutConfig, dm, layout);
    } else {
        HomeLayout::buildDefault(dm, layout);
    }
}

void HomeScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenLayout layout;
    buildLayout(dm, layout);

    for (uint8_t i = 0; i < layout.count(); ++i) {
        const LayoutWidget& widget = layout[i];
        const HomeLayoutWidgetConfig* widgetConfig =
            _layoutConfig != nullptr ? HomeLayout::findWidget(*_layoutConfig, widget.id) : nullptr;

        if (widgetConfig != nullptr && widgetConfig->allowOverlap &&
            widgetConfig->whiteHalo > 0) {
            const int16_t halo = widgetConfig->whiteHalo;
            const int16_t left = max<int16_t>(HomeLayout::ContentLeft, widget.x - halo);
            const int16_t top = max<int16_t>(HomeLayout::ContentTop, widget.y - halo);
            const int16_t right = min<int16_t>(HomeLayout::ContentRight, widget.x + widget.width + halo);
            const int16_t bottom = min<int16_t>(HomeLayout::ContentBottom, widget.y + widget.height + halo);
            display.fillRect(left, top, right - left, bottom - top, 1);
        }

        switch (widget.type) {
            case LayoutWidgetType::HomeWeatherCard:
                if (widgetConfig != nullptr && !widgetConfig->elements.empty()) {
                    CustomWidgetRenderer::draw(display, dm, *widgetConfig, "VENKU");
                } else {
                    drawWeatherCard(display, dm, widget, widgetConfig);
                }
                break;
            case LayoutWidgetType::HomeEnergyCard:
                if (widgetConfig != nullptr && !widgetConfig->elements.empty()) {
                    CustomWidgetRenderer::draw(display, dm, *widgetConfig, "ENERGIE");
                } else {
                    drawEnergyCard(display, dm, widget, widgetConfig);
                }
                break;
            case LayoutWidgetType::HomeIndoorCard:
                if (widgetConfig != nullptr && !widgetConfig->elements.empty()) {
                    CustomWidgetRenderer::draw(display, dm, *widgetConfig, "UVNITŘ");
                } else {
                    drawIndoorCard(display, dm, widget, widgetConfig);
                }
                break;
            case LayoutWidgetType::HomeFveCard:
                drawFveSummaryCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeAZRouterCard:
                drawAZRouterSummaryCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomePoolCard:
                drawPoolSummaryCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeConsumptionCard:
                drawConsumptionSummaryCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeEnergyFlowCard:
                drawEnergyFlowCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeRfSensorCard:
                drawRfSensorCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeCustomCard:
                if (widgetConfig != nullptr) CustomWidgetRenderer::draw(display, dm, *widgetConfig);
                break;
        }
    }

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void HomeScreen::buildNavigationLayout(const DataModel& dm, NavigationLayout& layout) const {
    layout.clear();

    ScreenLayout screenLayout;
    buildLayout(dm, screenLayout);
    for (uint8_t i = 0; i < screenLayout.count(); ++i) {
        const LayoutWidget& widget = screenLayout[i];
        layout.add(widget.id, widget.x, widget.y, widget.width, widget.height);
    }
}
