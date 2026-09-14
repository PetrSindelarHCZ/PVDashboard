#include "WeatherScreen.h"
#include "ScreenStyle.h"

void WeatherScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawHeader(display, "POCASI", dm);

    ScreenStyle::drawCard(display, 15, 65, 370, 345, "DNES");
    ScreenStyle::useMetric(display);
    display.setCursor(35, 155);
    display.printf("%.1f C", dm.weather.outdoorTempC);

    ScreenStyle::useBody(display);
    display.setCursor(35, 205);
    display.printf("Stav: %s", dm.weather.conditionText.c_str());
    display.setCursor(35, 250);
    display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
    display.setCursor(35, 295);
    display.printf("Rozsah: %.0f / %.0f C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);
    display.setCursor(35, 340);
    display.print("Tlak: 1018 hPa");
    display.setCursor(35, 385);
    display.print("Tendence: Ustaleny");

    ScreenStyle::drawCard(display, 400, 65, 385, 345, "VYHLED NA 4 DNY");
    ScreenStyle::useStrongBody(display);
    display.setCursor(420, 155);
    display.print("Ctvrtek");
    display.setCursor(420, 215);
    display.print("Patek");
    display.setCursor(420, 275);
    display.print("Sobota");
    display.setCursor(420, 335);
    display.print("Nedele");

    ScreenStyle::useBody(display);
    display.setCursor(550, 155);
    display.print("20 / 11 C | Polojasno");
    display.setCursor(550, 215);
    display.print("19 / 10 C | Oblacno");
    display.setCursor(550, 275);
    display.print("21 / 13 C | Slunecno");
    display.setCursor(550, 335);
    display.print("23 / 14 C | Jasno");

    ScreenStyle::drawFooter(display, "Modul Pocasi | pripraven pro meteorologicke API");
}