#include "HomeScreen.h"
#include "ScreenStyle.h"
#include "../layout/HomeLayout.h"
#include "../layout/CustomWidgetRenderer.h"
#include "../display/EInkGraph.h"

namespace {

uint16_t cardTextColor(const HomeLayoutWidgetConfig* style) {
    return style != nullptr && style->inverseText ? 1 : 0;
}

void drawHomeCardBackground(IDisplay& display, const LayoutWidget& widget,
                            const HomeLayoutWidgetConfig* style, const char* title) {
    const bool showFrame = style == nullptr ? true : style->showFrame;
    const bool blackBackground = style != nullptr && style->background == "black";
    const bool inverseText = style != nullptr && style->inverseText;
    ScreenStyle::drawStyledCard(display, widget.x, widget.y, widget.width, widget.height,
                                title, showFrame, blackBackground, inverseText);
}

void drawWeatherCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget,
                     const HomeLayoutWidgetConfig* style) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    const uint16_t color = cardTextColor(style);
    drawHomeCardBackground(display, widget, style, "PŘEDPOVĚĎ");

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
    drawHomeCardBackground(display, widget, style, "UVNITŘ");

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
        display.setCursor(x + 15, y + 83);
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


void drawFveSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    drawHomeCardBackground(display, widget, nullptr, "FVE / GOODWE");

    ScreenStyle::drawSolarStatus(display, x + 14, y + 49,
                                 dm.solar.status.available);

    ScreenStyle::useMetric(display);
    display.setCursor(x + 62, y + 86);
    if (dm.solar.status.available)
        display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);
    else
        display.print("--.- kW");

    ScreenStyle::useBody(display);
    display.setCursor(x + 15, y + 122);
    if (dm.solar.status.available)
        display.printf("Dnes %.1f kWh", dm.solar.energyTodayKWh);
    else
        display.print("Dnes --.- kWh");

    display.setCursor(x + 15, y + 151);
    if (dm.solar.status.available)
        display.printf("Dům %.1f | Síť %+.1f kW",
                       dm.solar.houseConsumptionW / 1000.0f,
                       dm.solar.gridPowerW / 1000.0f);
    else
        display.print("Dům --.- | Síť --.- kW");

    display.setCursor(x + 15, y + 181);
    if (dm.solar.status.available && dm.solar.batteryPresent)
        display.printf("Baterie %.0f %%  %+.0f W",
                       dm.solar.batterySocPercent, dm.solar.batteryPowerW);
    else if (dm.solar.status.available)
        display.print("Baterie neosazena");
    else
        display.print("GoodWe nedostupný");
}

void drawAZRouterSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    drawHomeCardBackground(display, widget, nullptr, "AZROUTER");

    ScreenStyle::drawRouterStatus(display, x + 14, y + 49,
                                  dm.azrouter.status.available);

    ScreenStyle::useMetric(display);
    display.setCursor(x + 62, y + 86);
    if (dm.azrouter.status.available && dm.azrouter.hasRoutedPower)
        display.printf("%.0f W", dm.azrouter.routedPowerW);
    else
        display.print("-- W");

    ScreenStyle::useBody(display);
    display.setCursor(x + 15, y + 122);
    if (dm.azrouter.hasRoutedEnergyToday)
        display.printf("Dnes %.1f kWh", dm.azrouter.routedEnergyTodayKWh);
    else
        display.print("Dnes --.- kWh");

    display.setCursor(x + 15, y + 151);
    if (dm.azrouter.hasGridPower)
        display.printf("Síť AZ %+.0f W", dm.azrouter.gridPowerW);
    else
        display.print("Síť AZ -- W");

    display.setCursor(x + 15, y + 181);
    if (dm.azrouter.hasSystemTemp)
        display.printf("Teplota %.1f °C", dm.azrouter.systemTempC);
    else
        display.print(dm.azrouter.status.available ? "Online" : "Nedostupný");
}

void drawPoolSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    drawHomeCardBackground(display, widget, nullptr, "BAZÉN");

    ScreenStyle::useMetric(display);
    display.setCursor(x + 15, y + 82);
    if (dm.pool.status.available)
        display.printf("%.1f °C", dm.pool.waterTempC);
    else
        display.printf("%.1f °C", dm.inside.poolTempC);

    ScreenStyle::useBody(display);
    display.setCursor(x + 15, y + 119);
    if (dm.pool.status.available)
        display.printf("Cíl %.1f °C", dm.pool.targetTempC);
    else
        display.print("Teplota čidla");

    display.setCursor(x + 15, y + 148);
    if (dm.pool.status.available)
        display.printf("Filtrace %s", dm.pool.filtrationRunning ? "ON" : "OFF");
    else
        display.print("Řízení bez dat");
}

void drawConsumptionSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    drawHomeCardBackground(display, widget, nullptr, "SPOTŘEBA DOMU");

    ScreenStyle::useMetric(display);
    display.setCursor(x + 15, y + 80);
    if (dm.solar.status.available)
        display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);
    else
        display.print("--.- kW");

    // Compact 24h consumption history. Bars are intentionally simple and
    // high-contrast so they survive partial e-paper refreshes.
    if (dm.solar.historyCount > 1) {
        float maxPower = 500.0f;
        for (uint8_t i = 0; i < dm.solar.historyCount; ++i) {
            if (dm.solar.history[i].houseConsumptionW > maxPower)
                maxPower = dm.solar.history[i].houseConsumptionW;
        }

        const int16_t graphX = x + 15;
        const int16_t graphY = y + 105;
        const int16_t graphW = w - 30;
        const int16_t graphH = 48;
        display.drawLine(graphX, graphY + graphH, graphX + graphW, graphY + graphH, 0);

        const uint8_t count = dm.solar.historyCount;
        const uint8_t step = count > static_cast<uint8_t>(graphW / 3)
            ? static_cast<uint8_t>((count + graphW / 3 - 1) / (graphW / 3))
            : 1;
        int16_t bar = 0;
        for (uint8_t i = 0; i < count && bar < graphW; i = static_cast<uint8_t>(i + step), ++bar) {
            const float watts = dm.solar.history[i].houseConsumptionW;
            int16_t bh = static_cast<int16_t>((watts / maxPower) * graphH);
            if (bh < 1 && watts > 0.0f) bh = 1;
            if (bh > graphH) bh = graphH;
            const int16_t bx = graphX + (bar * graphW) / ((count + step - 1) / step);
            display.drawLine(bx, graphY + graphH, bx, graphY + graphH - bh, 0);
        }
    } else {
        ScreenStyle::useBody(display);
        display.setCursor(x + 15, y + 132);
        display.print("Čekám na historii");
    }
}

void drawSystemSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    drawHomeCardBackground(display, widget, nullptr, "STAV SYSTÉMU");

    ScreenStyle::useBody(display);
    display.setCursor(x + 12, y + 58);
    display.printf("Wi-Fi      %s", dm.system.wifiConnected ? "OK" : "OFF");

    display.setCursor(x + 12, y + 82);
    display.printf("GoodWe     %s",
                   dm.solar.enabled ? (dm.solar.status.available ? "OK" : "OFF") : "--");

    display.setCursor(x + 12, y + 106);
    display.printf("AZRouter   %s",
                   dm.azrouter.enabled ? (dm.azrouter.status.available ? "OK" : "OFF") : "--");

    display.setCursor(x + 12, y + 130);
    display.printf("Předpověď  %s",
                   dm.weather.enabled ? (dm.weather.status.available ? "OK" : "OFF") : "--");

    display.setCursor(x + 12, y + 154);
    display.printf("433 MHz    %u", dm.rfSensors.sensorCount);
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
        switch (widget.type) {
            case LayoutWidgetType::HomeWeatherCard:
                drawWeatherCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeEnergyCard:
                drawEnergyCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeIndoorCard:
                drawIndoorCard(display, dm, widget, widgetConfig);
                break;
            case LayoutWidgetType::HomeFveCard:
                drawFveSummaryCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomeAZRouterCard:
                drawAZRouterSummaryCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomePoolCard:
                drawPoolSummaryCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomeConsumptionCard:
                drawConsumptionSummaryCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomeSystemCard:
                drawSystemSummaryCard(display, dm, widget);
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
