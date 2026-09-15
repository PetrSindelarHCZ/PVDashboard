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

void printShortDate(IDisplay& display, const char* date) {
    if (date != nullptr && strlen(date) >= 10) {
        display.printf("%c%c.%c%c.", date[8], date[9], date[5], date[6]);
    } else {
        display.print("--.--.");
    }
}
uint8_t hourOfDay(const HourlyWeatherForecast& hour, const char* dayDate) {
    if (dayDate != nullptr && strlen(dayDate) >= 10 && strcmp(hour.date, dayDate) != 0) return 24;
    if (strlen(hour.time) < 2) return 0;
    const uint8_t tens = hour.time[0] >= '0' && hour.time[0] <= '9' ? hour.time[0] - '0' : 0;
    const uint8_t ones = hour.time[1] >= '0' && hour.time[1] <= '9' ? hour.time[1] - '0' : 0;
    const uint8_t value = tens * 10 + ones;
    return value > 24 ? 24 : value;
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
        ScreenStyle::drawWeatherSymbol(display, 300, 130, dm.weather.weatherCode);

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
        ScreenStyle::drawWeatherSymbol(display, 405, y + 24, day.weatherCode);

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

    const HourlyWeatherForecast* slots[WeatherHourlySlotsPerDay] = {};
    uint8_t slotCount = 0;
    float minTemp = 100.0f;
    float maxTemp = -100.0f;
    for (uint8_t i = 0; i < dm.weather.hourlyCount && slotCount < WeatherHourlySlotsPerDay; ++i) {
        const HourlyWeatherForecast& hour = dm.weather.hourly[i];
        if (strcmp(hour.date, date) != 0 && slotCount > 0) {
            const uint8_t lastHour = hourOfDay(*slots[slotCount - 1], date);
            if (lastHour < 24) {
                slots[slotCount++] = &hour;
                if (hour.tempC < minTemp) minTemp = hour.tempC;
                if (hour.tempC > maxTemp) maxTemp = hour.tempC;
            }
            break;
        }
        if (strcmp(hour.date, date) != 0) continue;
        slots[slotCount++] = &hour;
        if (hour.tempC < minTemp) minTemp = hour.tempC;
        if (hour.tempC > maxTemp) maxTemp = hour.tempC;
    }
    if (slotCount == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 155);
        display.print("Pro tento den nejsou hodinova data.");
        return;
    }
    if (maxTemp - minTemp < 1.0f) {
        maxTemp += 0.5f;
        minTemp -= 0.5f;
    }

    const int16_t panelX = 85;
    const int16_t panelY = 135;
    const int16_t panelW = 690;
    const int16_t panelH = 315;
    display.drawRoundRect(panelX, panelY, panelW, panelH, ScreenStyle::CardRadius, 0);
    display.drawLine(panelX + 18, panelY + 190, panelX + panelW - 18, panelY + 190, 0);

    const int16_t firstX = panelX + 35;
    const int16_t lastX = panelX + panelW - 35;
    const int16_t axisW = lastX - firstX;
    const int16_t graphTop = panelY + 208;
    const int16_t graphBottom = panelY + 258;
    int16_t previousX = 0;
    int16_t previousY = 0;

    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        const HourlyWeatherForecast& hour = *slots[slot];
        const int16_t x = firstX + static_cast<int16_t>((axisW * hourOfDay(hour, date)) / 24);
        const float normalized = (hour.tempC - minTemp) / (maxTemp - minTemp);
        const int16_t pointY = graphBottom - static_cast<int16_t>(normalized * (graphBottom - graphTop));
        if (slot > 0) ScreenStyle::drawBoldLine(display, previousX, previousY, x, pointY, 0);
        previousX = x;
        previousY = pointY;
    }

    ScreenStyle::useBody(display);
    display.setCursor(firstX - 8, panelY + 187);
    display.print("0");
    display.setCursor(firstX + axisW / 4 - 10, panelY + 187);
    display.print("6");
    display.setCursor(firstX + axisW / 2 - 14, panelY + 187);
    display.print("12");
    display.setCursor(firstX + axisW * 3 / 4 - 14, panelY + 187);
    display.print("18");
    display.setCursor(lastX - 18, panelY + 187);
    display.print("24h");

    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        const HourlyWeatherForecast& hour = *slots[slot];
        const int16_t x = firstX + static_cast<int16_t>((axisW * hourOfDay(hour, date)) / 24);
        const float normalized = (hour.tempC - minTemp) / (maxTemp - minTemp);
        const int16_t pointY = graphBottom - static_cast<int16_t>(normalized * (graphBottom - graphTop));

        ScreenStyle::useBody(display);
        display.setCursor(x - 21, panelY + 29);
        display.print(hourOfDay(hour, date) == 24 ? "24:00" : hour.time);
        ScreenStyle::drawWeatherSymbol(display, x, panelY + 75, hour.weatherCode);
        ScreenStyle::useValue(display);
        display.setCursor(x - 25, panelY + 138);
        display.printf("%.0f C", hour.tempC);
        ScreenStyle::useBody(display);
        display.setCursor(x - 28, panelY + 167);
        display.printf("%.0f km/h", hour.windKmh);

        display.fillCircle(x, pointY, 4, 0);
        display.drawCircle(x, pointY, 6, 1);
        display.fillCircle(x - 13, panelY + 286, 2, 0);
        ScreenStyle::drawBoldLine(display, x - 13, panelY + 289, x - 15, panelY + 294, 0);
        display.setCursor(x - 5, panelY + 296);
        if (hour.hasPrecipitationProbability) {
            display.printf("%u%%", hour.precipitationProbabilityPercent);
        } else {
            display.printf("%.1f", hour.precipitationMm);
        }
    }
}
