#include "WeatherScreen.h"
#include "ScreenStyle.h"

namespace {
const char* conditionText(uint8_t code) {
    switch (code) {
        case 0: return "Jasno";
        case 1: return "Prevazne jasno";
        case 2: return "Polojasno";
        case 3: return "Zatazeno";
        case 45:
        case 48: return "Mlha";
        case 51:
        case 53:
        case 55:
        case 56:
        case 57: return "Mrholeni";
        case 61:
        case 63:
        case 65:
        case 66:
        case 67: return "Dest";
        case 71:
        case 73:
        case 75:
        case 77: return "Snezeni";
        case 80:
        case 81:
        case 82: return "Prehanky";
        case 85:
        case 86: return "Snehove prehanky";
        case 95:
        case 96:
        case 99: return "Bourka";
        default: return "Neznamy stav";
    }
}

bool isFog(uint8_t code) {
    return code == 45 || code == 48;
}

bool isRain(uint8_t code) {
    return (code >= 51 && code <= 67) || (code >= 80 && code <= 82);
}

bool isSnow(uint8_t code) {
    return (code >= 71 && code <= 77) || code == 85 || code == 86;
}

bool isThunderstorm(uint8_t code) {
    return code >= 95;
}

void drawSun(IDisplay& display, int16_t x, int16_t y) {
    ScreenStyle::drawBoldCircle(display, x, y, 10, 0);
    for (uint8_t i = 0; i < 3; ++i) {
        display.drawCircle(x, y, 5 + i, 0);
    }
    ScreenStyle::drawBoldLine(display, x, y - 18, x, y - 14, 0);
    ScreenStyle::drawBoldLine(display, x, y + 14, x, y + 18, 0);
    ScreenStyle::drawBoldLine(display, x - 18, y, x - 14, y, 0);
    ScreenStyle::drawBoldLine(display, x + 14, y, x + 18, y, 0);
    ScreenStyle::drawBoldLine(display, x - 13, y - 13, x - 10, y - 10, 0);
    ScreenStyle::drawBoldLine(display, x + 10, y + 10, x + 13, y + 13, 0);
    ScreenStyle::drawBoldLine(display, x + 10, y - 10, x + 13, y - 13, 0);
    ScreenStyle::drawBoldLine(display, x - 13, y + 13, x - 10, y + 10, 0);
}

void drawCloud(IDisplay& display, int16_t x, int16_t y) {
    display.fillCircle(x - 10, y + 2, 8, 0);
    display.fillCircle(x, y - 5, 11, 0);
    display.fillCircle(x + 12, y + 1, 9, 0);
    display.fillRoundRect(x - 19, y, 39, 13, 5, 0);
}

void drawRainDrops(IDisplay& display, int16_t x, int16_t y) {
    ScreenStyle::drawBoldLine(display, x - 11, y, x - 14, y + 7, 0);
    ScreenStyle::drawBoldLine(display, x, y, x - 3, y + 7, 0);
    ScreenStyle::drawBoldLine(display, x + 11, y, x + 8, y + 7, 0);
}

void drawSnow(IDisplay& display, int16_t x, int16_t y) {
    for (int16_t offset = -10; offset <= 10; offset += 10) {
        display.drawLine(x + offset - 3, y, x + offset + 3, y + 6, 0);
        display.drawLine(x + offset + 3, y, x + offset - 3, y + 6, 0);
        display.drawLine(x + offset, y - 1, x + offset, y + 7, 0);
    }
}

void drawWeatherSymbol(IDisplay& display, int16_t x, int16_t y, uint8_t code) {
    if (isFog(code)) {
        ScreenStyle::drawBoldLine(display, x - 20, y - 8, x + 15, y - 8, 0);
        ScreenStyle::drawBoldLine(display, x - 14, y, x + 20, y, 0);
        ScreenStyle::drawBoldLine(display, x - 20, y + 8, x + 12, y + 8, 0);
        return;
    }
    if (code == 0) {
        drawSun(display, x, y);
        return;
    }
    if (code == 1 || code == 2) {
        drawSun(display, x - 8, y - 7);
        drawCloud(display, x + 5, y + 5);
        return;
    }

    drawCloud(display, x, y - 5);
    if (isThunderstorm(code)) {
        ScreenStyle::drawBoldLine(display, x + 2, y + 8, x - 5, y + 19, 0);
        ScreenStyle::drawBoldLine(display, x - 5, y + 19, x + 2, y + 18, 0);
        ScreenStyle::drawBoldLine(display, x + 2, y + 18, x - 4, y + 29, 0);
    } else if (isSnow(code)) {
        drawSnow(display, x, y + 13);
    } else if (isRain(code)) {
        drawRainDrops(display, x, y + 12);
    }
}
void printShortDate(IDisplay& display, const char* date) {
    if (date != nullptr && strlen(date) >= 10) {
        display.printf("%c%c.%c%c.", date[8], date[9], date[5], date[6]);
    } else {
        display.print("--.--.");
    }
}
}

void WeatherScreen::render(IDisplay& display, const DataModel& dm) {
    if (_forecastDay >= 0) {
        renderHourly(display, dm);
        return;
    }
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 282, 402, "AKTUALNE");
    if (!dm.weather.status.available) {
        ScreenStyle::useValue(display);
        display.setCursor(95, 150);
        display.print("Pocasi nedostupne");
        ScreenStyle::useBody(display);
        display.setCursor(95, 195);
        display.print(dm.weather.status.lastError);
        display.setCursor(95, 240);
        display.print("Zkontrolujte Wi-Fi");
        display.setCursor(95, 270);
        display.print("a konfiguraci ve WebUI");
    } else {
        drawWeatherSymbol(display, 300, 130, dm.weather.weatherCode);

        ScreenStyle::useMetric(display);
        display.setCursor(95, 145);
        display.printf("%.1f C", dm.weather.outdoorTempC);

        ScreenStyle::useStrongBody(display);
        display.setCursor(95, 190);
        display.print(dm.weather.conditionText);
        ScreenStyle::useBody(display);
        display.setCursor(95, 230);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(95, 270);
        display.printf("Tlak: %.0f hPa", dm.weather.surfacePressureHpa);
        display.setCursor(95, 310);
        display.printf("Vitr: %.1f km/h", dm.weather.windSpeedKmh);
        display.setCursor(95, 350);
        display.printf("Srazky: %.1f mm", dm.weather.currentPrecipitationMm);
        display.setCursor(95, 390);
        display.printf("Dnes: %.0f / %.0f C",
                       dm.weather.tempMaxTodayC,
                       dm.weather.tempMinTodayC);
        display.setCursor(95, 435);
        display.printf("Zdroj: %s", dm.weather.provider.c_str());
    }

    ScreenStyle::drawCard(display, 367, 63, 418, 402, "PREDPOVED NA 4 DNY");
    if (!dm.weather.status.available || dm.weather.dailyCount == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(387, 145);
        display.print("Predpoved zatim neni k dispozici.");
        return;
    }

    for (uint8_t i = 0; i < dm.weather.dailyCount; ++i) {
        const DailyWeatherForecast& day = dm.weather.daily[i];
        const int16_t y = 125 + i * 83;
        drawWeatherSymbol(display, 405, y + 24, day.weatherCode);

        ScreenStyle::useStrongBody(display);
        display.setCursor(442, y);
        printShortDate(display, day.date);
        display.setCursor(510, y);
        display.print(conditionText(day.weatherCode));

        ScreenStyle::useValue(display);
        display.setCursor(442, y + 30);
        display.printf("%.0f / %.0f C", day.tempMaxC, day.tempMinC);

        ScreenStyle::useBody(display);
        display.setCursor(590, y + 28);
        if (day.hasPrecipitationProbability) {
            display.printf("Dest %u %% / %.1f mm",
                           day.precipitationProbabilityPercent,
                           day.precipitationMm);
        } else {
            display.printf("Dest %.1f mm", day.precipitationMm);
        }
        display.setCursor(590, y + 52);
        display.printf("Vitr max %.0f km/h", day.windMaxKmh);
    }
}
void WeatherScreen::renderHourly(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);
    ScreenStyle::useTitle(display);
    display.setCursor(85, 91);
    display.print("HODINOVA PREDPOVED");
    if (!dm.weather.status.available || _forecastDay >= dm.weather.dailyCount) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 155);
        display.print("Predpoved zatim neni k dispozici.");
        return;
    }

    const char* date = dm.weather.daily[_forecastDay].date;
    display.setCursor(650, 91);
    printShortDate(display, date);
    ScreenStyle::useBody(display);
    display.setCursor(85, 116);
    display.printf("Po %s hodinach | Zdroj: %s",
                   dm.weather.provider == "MET Norway" ? "3-6" : "3",
                   dm.weather.provider.c_str());

    uint8_t slot = 0;
    for (uint8_t i = 0; i < dm.weather.hourlyCount && slot < WeatherHourlySlotsPerDay; ++i) {
        const HourlyWeatherForecast& hour = dm.weather.hourly[i];
        if (strcmp(hour.date, date) != 0) continue;
        const int16_t x = 75 + (slot % 4) * 179;
        const int16_t y = 131 + (slot / 4) * 167;
        ScreenStyle::drawCard(display, x, y, 173, 160, hour.time);
        drawWeatherSymbol(display, x + 135, y + 51, hour.weatherCode);
        ScreenStyle::useValue(display);
        display.setCursor(x + 12, y + 67);
        display.printf("%.1f C", hour.tempC);
        ScreenStyle::useBody(display);
        display.setCursor(x + 12, y + 101);
        display.printf("Vitr %.0f km/h", hour.windKmh);
        display.setCursor(x + 12, y + 127);
        display.printf("Srazky %.1f mm", hour.precipitationMm);
        if (hour.hasPrecipitationProbability) {
            display.setCursor(x + 12, y + 150);
            display.printf("Pravdep. %u %%", hour.precipitationProbabilityPercent);
        }
        ++slot;
    }
    if (slot == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 155);
        display.print("Pro tento den nejsou hodinova data.");
    }
}
