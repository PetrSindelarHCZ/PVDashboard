#include "WeatherScreen.h"
#include "ScreenStyle.h"
#include "../display/EInkGraph.h"

namespace {
const char* conditionText(uint8_t code) {
    switch (code) {
        case 0: return "Jasno";
        case 1: return "Převážně jasno";
        case 2: return "Polojasno";
        case 3: return "Zataženo";
        case 45:
        case 48: return "Mlha";
        case 51:
        case 53:
        case 55:
        case 56:
        case 57: return "Mrholení";
        case 61:
        case 63:
        case 65:
        case 66:
        case 67: return "Déšť";
        case 71:
        case 73:
        case 75:
        case 77: return "Sněžení";
        case 80:
        case 81:
        case 82: return "Přeháňky";
        case 85:
        case 86: return "Sněhové přeháňky";
        case 95:
        case 96:
        case 99: return "Bouřka";
        default: return "Neznámý stav";
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

    ScreenStyle::drawCard(display, 75, 63, 282, 402, "AKTUÁLNĚ");
    if (!dm.weather.status.available) {
        ScreenStyle::useValue(display);
        display.setCursor(95, 150);
        display.print("Počasí nedostupné");
        ScreenStyle::useBody(display);
        display.setCursor(95, 195);
        display.print(dm.weather.status.lastError);
        display.setCursor(95, 240);
        display.print("Zkontrolujte Wi-Fi");
        display.setCursor(95, 270);
        display.print("a konfiguraci ve WebUI");
    } else {
        ScreenStyle::drawWeatherSymbol(display, 300, 132, dm.weather.weatherCode,
                                       IconAssets::WeatherSize::Large56);

        ScreenStyle::useMetric(display);
        display.setCursor(95, 145);
        display.printf("%.1f °C", dm.weather.outdoorTempC);

        ScreenStyle::useStrongBody(display);
        display.setCursor(95, 190);
        display.print(dm.weather.conditionText);
        ScreenStyle::useBody(display);
        display.setCursor(95, 230);
        display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);
        display.setCursor(95, 270);
        display.printf("Tlak: %.0f hPa", dm.weather.surfacePressureHpa);
        display.setCursor(95, 310);
        display.printf("Vítr: %.1f km/h", dm.weather.windSpeedKmh);
        display.setCursor(95, 350);
        display.printf("Srážky: %.1f mm", dm.weather.currentPrecipitationMm);
        display.setCursor(95, 390);
        display.printf("Dnes: %.0f / %.0f °C",
                       dm.weather.tempMaxTodayC,
                       dm.weather.tempMinTodayC);
        display.setCursor(95, 435);
        display.printf("Zdroj: %s", dm.weather.provider.c_str());
    }

    ScreenStyle::drawCard(display, 367, 63, 418, 402, "PŘEDPOVĚĎ NA 4 DNY");
    if (!dm.weather.status.available || dm.weather.dailyCount == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(387, 145);
        display.print("Předpověď zatím není k dispozici.");
        return;
    }

    for (uint8_t i = 0; i < dm.weather.dailyCount; ++i) {
        const DailyWeatherForecast& day = dm.weather.daily[i];
        const int16_t y = 125 + i * 83;
        ScreenStyle::drawWeatherSymbol(display, 410, y + 22, day.weatherCode,
                                       IconAssets::WeatherSize::Medium40);

        ScreenStyle::useStrongBody(display);
        display.setCursor(442, y);
        printShortDate(display, day.date);
        display.setCursor(510, y);
        display.print(conditionText(day.weatherCode));

        ScreenStyle::useValue(display);
        display.setCursor(442, y + 30);
        display.printf("%.0f / %.0f °C", day.tempMaxC, day.tempMinC);

        ScreenStyle::useBody(display);
        display.setCursor(590, y + 28);
        if (day.hasPrecipitationProbability) {
            display.printf("Déšť %u %% / %.1f mm",
                           day.precipitationProbabilityPercent,
                           day.precipitationMm);
        } else {
            display.printf("Déšť %.1f mm", day.precipitationMm);
        }
        display.setCursor(590, y + 52);
        display.printf("Vítr max %.0f km/h", day.windMaxKmh);
    }
}

void WeatherScreen::renderHourly(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);
    ScreenStyle::useTitle(display);
    display.setCursor(85, 91);
    display.print("HODINOVÁ PŘEDPOVĚĎ");
    if (!dm.weather.status.available || _forecastDay >= dm.weather.dailyCount) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 155);
        display.print("Předpověď zatím není k dispozici.");
        return;
    }

    const char* date = dm.weather.daily[_forecastDay].date;
    display.setCursor(650, 91);
    printShortDate(display, date);
    ScreenStyle::useBody(display);
    display.setCursor(85, 116);
    display.printf("Po %s hodinách | Zdroj: %s",
                   dm.weather.provider == "MET Norway" ? "3-6" : "3",
                   dm.weather.provider.c_str());

    const HourlyWeatherForecast* slots[WeatherHourlySlotsPerDay] = {};
    EInkGraphPoint tempPoints[WeatherHourlySlotsPerDay] = {};
    uint8_t slotCount = 0;
    float minTemp = 100.0f;
    float maxTemp = -100.0f;

    for (uint8_t i = 0; i < dm.weather.hourlyCount && slotCount < WeatherHourlySlotsPerDay; ++i) {
        const HourlyWeatherForecast& hour = dm.weather.hourly[i];
        if (strcmp(hour.date, date) != 0 && slotCount > 0) {
            const uint8_t lastHour = hourOfDay(*slots[slotCount - 1], date);
            if (lastHour < 24) {
                slots[slotCount] = &hour;
                tempPoints[slotCount].x = static_cast<float>(hourOfDay(hour, date));
                tempPoints[slotCount].y = hour.tempC;
                if (hour.tempC < minTemp) minTemp = hour.tempC;
                if (hour.tempC > maxTemp) maxTemp = hour.tempC;
                ++slotCount;
            }
            break;
        }
        if (strcmp(hour.date, date) != 0) continue;
        slots[slotCount] = &hour;
        tempPoints[slotCount].x = static_cast<float>(hourOfDay(hour, date));
        tempPoints[slotCount].y = hour.tempC;
        if (hour.tempC < minTemp) minTemp = hour.tempC;
        if (hour.tempC > maxTemp) maxTemp = hour.tempC;
        ++slotCount;
    }

    if (slotCount == 0) {
        ScreenStyle::useBody(display);
        display.setCursor(95, 155);
        display.print("Pro tento den nejsou hodinová data.");
        return;
    }

    minTemp = floorf(minTemp - 1.0f);
    maxTemp = ceilf(maxTemp + 1.0f);
    if (maxTemp - minTemp < 4.0f) {
        const float middle = (minTemp + maxTemp) * 0.5f;
        minTemp = middle - 2.0f;
        maxTemp = middle + 2.0f;
    }

    const int16_t panelX = 85;
    const int16_t panelY = 135;
    const int16_t panelW = 690;
    const int16_t panelH = 315;
    display.drawRoundRect(panelX, panelY, panelW, panelH, ScreenStyle::CardRadius, 0);

    const int16_t firstX = panelX + 38;
    const int16_t lastX = panelX + panelW - 30;
    const int16_t axisW = lastX - firstX;
    const int16_t graphTop = panelY + 193;
    const int16_t graphBottom = panelY + 267;

    EInkGraph graph(display, firstX, graphTop, axisW + 1, graphBottom - graphTop + 1);
    graph.setXRange(0.0f, 24.0f);
    graph.setYRange(minTemp, maxTemp);
    graph.drawHorizontalGrid(4);
    graph.drawVerticalGrid(4);
    graph.drawLineSeries(tempPoints, slotCount, true);

    ScreenStyle::useBody(display);
    for (uint8_t grid = 0; grid <= 4; ++grid) {
        const float value = maxTemp - ((maxTemp - minTemp) * grid / 4.0f);
        const int16_t y = graph.mapY(value);
        display.setCursor(panelX + 8, y + 5);
        display.printf("%.0f°", value);
    }

    const uint8_t axisHours[] = {0, 6, 12, 18, 24};
    for (uint8_t i = 0; i < 5; ++i) {
        const int16_t x = graph.mapX(static_cast<float>(axisHours[i]));
        display.setCursor(x - (axisHours[i] >= 10 ? 11 : 6), graphBottom + 24);
        if (axisHours[i] == 24) display.print("24h");
        else display.printf("%u", axisHours[i]);
    }

    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        const HourlyWeatherForecast& hour = *slots[slot];
        const int16_t x = graph.mapX(static_cast<float>(hourOfDay(hour, date)));

        ScreenStyle::useBody(display);
        display.setCursor(x - 18, panelY + 28);
        display.print(hourOfDay(hour, date) == 24 ? "24:00" : hour.time);

        ScreenStyle::drawWeatherSymbol(display, x, panelY + 65, hour.weatherCode,
                                       IconAssets::WeatherSize::Small24);

        ScreenStyle::useValue(display);
        display.setCursor(x - 22, panelY + 111);
        display.printf("%.0f°", hour.tempC);

        ScreenStyle::useBody(display);
        display.setCursor(x - 27, panelY + 140);
        display.printf("%.0f km/h", hour.windKmh);

        const int16_t pointY = graph.mapY(hour.tempC);
        ScreenStyle::useBody(display);
        display.setCursor(x - 14, pointY - 8);
        display.printf("%.0f", hour.tempC);

        display.fillCircle(x - 13, panelY + 294, 2, 0);
        ScreenStyle::drawBoldLine(display, x - 13, panelY + 297, x - 15, panelY + 302, 0);
        display.setCursor(x - 5, panelY + 304);
        if (hour.hasPrecipitationProbability) {
            display.printf("%u%%", hour.precipitationProbabilityPercent);
        } else {
            display.printf("%.1f", hour.precipitationMm);
        }
    }
}
