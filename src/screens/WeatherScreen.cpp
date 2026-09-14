#include "WeatherScreen.h"
#include "ScreenStyle.h"

void WeatherScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 342, 402, "DNES");
    ScreenStyle::useMetric(display);
    display.setCursor(95, 155);
    display.printf("%.1f C", dm.weather.outdoorTempC);

    ScreenStyle::useBody(display);
    display.setCursor(95, 205);
    display.printf("Stav: %s", dm.weather.conditionText.c_str());
    display.setCursor(95, 250);
    display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
    display.setCursor(95, 295);
    display.printf("Rozsah: %.0f / %.0f C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
    display.setCursor(95, 340);
    display.print("Tlak: 1018 hPa");
    display.setCursor(95, 385);
    display.print("Tendence: Ustaleny");

    ScreenStyle::drawCard(display, 427, 63, 358, 402, "VYHLED NA 4 DNY");
    ScreenStyle::useStrongBody(display);
    display.setCursor(447, 155);
    display.print("Ctvrtek");
    display.setCursor(447, 215);
    display.print("Patek");
    display.setCursor(447, 275);
    display.print("Sobota");
    display.setCursor(447, 335);
    display.print("Nedele");

    ScreenStyle::useBody(display);
    display.setCursor(575, 155);
    display.print("20 / 11 C | Polojasno");
    display.setCursor(575, 215);
    display.print("19 / 10 C | Oblacno");
    display.setCursor(575, 275);
    display.print("21 / 13 C | Slunecno");
    display.setCursor(575, 335);
    display.print("23 / 14 C | Jasno");

}
