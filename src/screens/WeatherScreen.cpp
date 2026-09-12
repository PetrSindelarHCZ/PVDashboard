#include "WeatherScreen.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

void WeatherScreen::render(IDisplay& display, const DataModel& dm) {
    display.fillRect(0, 0, 800, 48, 1);
    display.setTextColor(0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 32);
    display.print("PREDPOVED POCASI");

    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(680, 35);
    display.print(dm.system.timeStr);

    display.drawLine(0, 48, 800, 48, 0);

    // Hlavní karta - Dnes
    display.drawRoundRect(20, 70, 360, 340, 6, 0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(40, 105);
    display.print("Dnes (Aktualne)");
    display.drawLine(40, 115, 360, 115, 0);

    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(50, 175);
    display.printf("%.1f C", dm.weather.outdoorTempC);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(50, 220);
    display.printf("Stav: %s", dm.weather.conditionText.c_str());

    display.setCursor(50, 260);
    display.printf("Vlhkost vzduchu: %d %%", dm.weather.outdoorHumidityPercent);

    display.setCursor(50, 300);
    display.printf("Rozsah dnes: %.0f C / %.0f C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);

    display.setCursor(50, 340);
    display.print("Tlak: 1018 hPa (Ustáleny)");

    // Dny předpovědi vpravo
    display.drawRoundRect(400, 70, 380, 340, 6, 0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(420, 105);
    display.print("Vyhled na 3 dny");
    display.drawLine(420, 115, 760, 115, 0);

    // Den 1
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(430, 155);
    display.print("Ctvrtek:");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(560, 155);
    display.print("20 C / 11 C | Polojasno");

    // Den 2
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(430, 215);
    display.print("Patek:");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(560, 215);
    display.print("19 C / 10 C | Oblacno");

    // Den 3
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(430, 275);
    display.print("Sobota:");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(560, 275);
    display.print("21 C / 13 C | Slunecno");

    // Den 4
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(430, 335);
    display.print("Nedele:");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(560, 335);
    display.print("23 C / 14 C | Jasno");

    // Footer
    display.drawLine(0, 430, 800, 430, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 460);
    display.print("Modul Pocasi - Pripraven pro OpenWeatherMap / Meteo API");
}
