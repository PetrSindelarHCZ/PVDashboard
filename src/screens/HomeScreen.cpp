#include "HomeScreen.h"
#include "ScreenStyle.h"
#include "../layout/HomeLayout.h"
#include "../layout/CustomWidgetRenderer.h"

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
        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 62);
        display.print("Obývák");
        drawBmeTemperature(x + 15, y + 88);

        ScreenStyle::useBody(display, color);
        display.setCursor(x + 15, y + 122);
        if (dm.inside.status.available)
            display.printf("Vlhkost %d %%  |  %.0f hPa",
                           dm.inside.humidityPercent, dm.inside.pressureHpa);
        else
            display.print("Senzor nedostupný");

        display.setCursor(x + 15, y + 151);
        display.printf("Ložnice %.1f °C", dm.inside.bedroomTempC);
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
        display.printf("Síť %+.1f kW", dm.solar.gridPowerW / 1000.0f);
    else
        display.print("Síť --.- kW");

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
        display.print("--.- °C");

    ScreenStyle::useBody(display);
    display.setCursor(x + 15, y + 119);
    if (dm.pool.status.available)
        display.printf("Cíl %.1f °C", dm.pool.targetTempC);
    else
        display.print("Data nedostupná");

    display.setCursor(x + 15, y + 148);
    if (dm.pool.status.available)
        display.printf("Filtrace %s  Topení %s",
                       dm.pool.filtrationRunning ? "ON" : "OFF",
                       dm.pool.heatingActive ? "ON" : "OFF");
}

void drawSystemSummaryCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    drawHomeCardBackground(display, widget, nullptr, "STAV SYSTÉMU");

    ScreenStyle::useBody(display);
    display.setCursor(x + 15, y + 61);
    display.printf("Wi-Fi      %s", dm.system.wifiConnected ? "Online" : "Offline");

    display.setCursor(x + 15, y + 88);
    if (dm.solar.enabled)
        display.printf("GoodWe     %s", dm.solar.status.available ? "Online" : "Offline");
    else
        display.print("GoodWe     vypnuto");

    display.setCursor(x + 15, y + 115);
    if (dm.azrouter.enabled)
        display.printf("AZRouter   %s", dm.azrouter.status.available ? "Online" : "Offline");
    else
        display.print("AZRouter   vypnuto");

    display.setCursor(x + 15, y + 142);
    display.printf("Předpověď  %s", dm.weather.status.available ? "Online" : "Offline");

    display.setCursor(x + 15, y + 169);
    display.printf("433 MHz    %u čidel", dm.rfSensors.sensorCount);
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
