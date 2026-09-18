#include "HomeScreen.h"
#include "ScreenStyle.h"
#include "../layout/HomeLayout.h"
#include "../layout/CustomWidgetRenderer.h"

namespace {

void drawWeatherCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    ScreenStyle::drawCard(display, x, y, w, h, "VENKU");

    if (dm.weather.status.available) {
        ScreenStyle::drawWeatherSymbol(display, x + w - 52, y + 67, dm.weather.weatherCode);

        ScreenStyle::useMetric(display);
        display.setCursor(x + 15, y + 82);
        display.printf("%.1f °C", dm.weather.outdoorTempC);

        ScreenStyle::useBody(display);
        display.setCursor(x + 15, y + 122);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(x + 15, y + 172);
        display.print("Dnešní rozsah");
        ScreenStyle::useValue(display);
        display.setCursor(x + 15, y + 207);
        display.printf("%.0f / %.0f °C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
        ScreenStyle::useBody(display);
        display.setCursor(x + 15, y + 262);
        display.printf("Stav: %s", dm.weather.conditionText.c_str());
        display.setCursor(x + 15, y + 312);
        display.print(dm.weather.locationName.isEmpty() ? dm.weather.provider : dm.weather.locationName);
    } else {
        ScreenStyle::useValue(display);
        display.setCursor(x + 15, y + 82);
        display.print("--.- °C");
        ScreenStyle::useBody(display);
        display.setCursor(x + 15, y + 132);
        display.print("Počasí nedostupné");
        display.setCursor(x + 15, y + 172);
        display.print(dm.weather.status.lastError);
    }

    if (!dm.weather.provider.isEmpty()) {
        ScreenStyle::useBody(display);
        const String providerLabel = "Data: " + dm.weather.provider;
        display.setCursor(x + w - 10 - display.textWidth(providerLabel), y + h - 20);
        display.print(providerLabel);
    }
}

void drawEnergyCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    ScreenStyle::drawCard(display, x, y, w, h, "ENERGIE");

    if (w >= 400) {
        const int16_t leftX = x + 20;
        const int16_t rightX = x + w / 2 + 10;

        ScreenStyle::useBody(display);
        display.setCursor(leftX, y + 62);
        display.print("Výroba FVE");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, y + 87);
        display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(leftX, y + 172);
        display.print("Spotřeba domu");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, y + 197);
        display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(rightX, y + 62);
        display.print("Distribuce");
        ScreenStyle::useValue(display);
        display.setCursor(rightX, y + 87);
        display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

        ScreenStyle::useBody(display);
        display.setCursor(rightX, y + 172);
        display.print("Baterie");
        ScreenStyle::useValue(display);
        display.setCursor(rightX, y + 197);
        display.printf("%.0f %%", dm.solar.batterySocPercent);
        ScreenStyle::useBody(display);
        display.setCursor(rightX, y + 237);
        display.printf("%+.0f W", dm.solar.batteryPowerW);
        return;
    }

    const int16_t valueX = x + 15;

    ScreenStyle::useBody(display);
    display.setCursor(valueX, y + 62);
    display.print("Výroba FVE");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, y + 87);
    display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, y + 132);
    display.print("Spotřeba domu");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, y + 157);
    display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, y + 202);
    display.print("Distribuce");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, y + 227);
    display.printf("%+.1f kW", dm.solar.gridPowerW / 1000.0f);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, y + 272);
    display.print("Baterie");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, y + 297);
    display.printf("%.0f %% (%+.0f W)", dm.solar.batterySocPercent, dm.solar.batteryPowerW);
}

void drawIndoorCard(IDisplay& display, const DataModel& dm, const LayoutWidget& widget) {
    const int16_t x = widget.x;
    const int16_t y = widget.y;
    const int16_t w = widget.width;
    const int16_t h = widget.height;

    ScreenStyle::drawCard(display, x, y, w, h, "UVNITŘ - DEMO");

    if (w >= 500) {
        const int16_t leftX = x + 20;
        const int16_t rightX = x + w / 2 + 10;

        ScreenStyle::useBody(display);
        display.setCursor(leftX, y + 62);
        display.print("Obývák");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, y + 87);
        display.printf("%.1f °C", dm.inside.livingRoomTempC);

        ScreenStyle::useBody(display);
        display.setCursor(leftX, y + 172);
        display.print("Ložnice");
        ScreenStyle::useValue(display);
        display.setCursor(leftX, y + 197);
        display.printf("%.1f °C", dm.inside.bedroomTempC);

        if (dm.pool.enabled) {
            ScreenStyle::useBody(display);
            display.setCursor(rightX, y + 62);
            display.print("Bazén");
            ScreenStyle::useValue(display);
            display.setCursor(rightX, y + 87);
            display.printf("%.1f °C", dm.inside.poolTempC);
        }
        return;
    }

    const int16_t valueX = x + 15;

    ScreenStyle::useBody(display);
    display.setCursor(valueX, y + 62);
    display.print("Obývák");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, y + 87);
    display.printf("%.1f °C", dm.inside.livingRoomTempC);

    ScreenStyle::useBody(display);
    display.setCursor(valueX, y + 132);
    display.print("Ložnice");
    ScreenStyle::useValue(display);
    display.setCursor(valueX, y + 157);
    display.printf("%.1f °C", dm.inside.bedroomTempC);

    if (dm.pool.enabled) {
        ScreenStyle::useBody(display);
        display.setCursor(valueX, y + 202);
        display.print("Bazén");
        ScreenStyle::useValue(display);
        display.setCursor(valueX, y + 227);
        display.printf("%.1f °C", dm.inside.poolTempC);
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
        switch (widget.type) {
            case LayoutWidgetType::HomeWeatherCard:
                drawWeatherCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomeEnergyCard:
                drawEnergyCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomeIndoorCard:
                drawIndoorCard(display, dm, widget);
                break;
            case LayoutWidgetType::HomeCustomCard:
                if (_layoutConfig != nullptr) {
                    const HomeLayoutWidgetConfig* config =
                        HomeLayout::findWidget(*_layoutConfig, widget.id);
                    if (config != nullptr) CustomWidgetRenderer::draw(display, dm, *config);
                }
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
