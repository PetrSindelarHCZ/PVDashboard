#include "OpenMeteoClient.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <cstring>

namespace {
constexpr int32_t ConnectTimeoutMs = 2500;
constexpr uint16_t ResponseTimeoutMs = 6000;

bool hasRequiredWeatherFields(const JsonDocument& doc) {
    return !doc["current"]["temperature_2m"].isNull() &&
           !doc["current"]["relative_humidity_2m"].isNull() &&
           !doc["current"]["weather_code"].isNull() &&
           doc["daily"]["time"].is<JsonArrayConst>() &&
           doc["daily"]["temperature_2m_max"].is<JsonArrayConst>() &&
           doc["daily"]["temperature_2m_min"].is<JsonArrayConst>();
}

template <size_t N>
void copyText(char (&destination)[N], const char* source) {
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    strlcpy(destination, source, N);
}
}

const char* OpenMeteoClient::conditionText(uint8_t weatherCode) {
    switch (weatherCode) {
        case 0: return "Jasno";
        case 1: return "Prevazne jasno";
        case 2: return "Polojasno";
        case 3: return "Zatazeno";
        case 45:
        case 48: return "Mlha";
        case 51:
        case 53:
        case 55: return "Mrholeni";
        case 56:
        case 57: return "Mrznouci mrholeni";
        case 61:
        case 63:
        case 65: return "Dest";
        case 66:
        case 67: return "Mrznouci dest";
        case 71:
        case 73:
        case 75:
        case 77: return "Snezeni";
        case 80:
        case 81:
        case 82: return "Destove prehanky";
        case 85:
        case 86: return "Snehove prehanky";
        case 95:
        case 96:
        case 99: return "Bourka";
        default: return "Neznamy stav";
    }
}

bool OpenMeteoClient::update(const WeatherConfig& config, WeatherData& weatherData) {
    if (!config.enabled) {
        weatherData.status.recordError("Weather disabled");
        return false;
    }
    if (config.provider != "open-meteo") {
        weatherData.status.recordError("Provider not implemented");
        return false;
    }
    if (config.latitude < -90.0 || config.latitude > 90.0 ||
        config.longitude < -180.0 || config.longitude > 180.0 ||
        (config.latitude == 0.0 && config.longitude == 0.0)) {
        weatherData.status.recordError("Location not configured");
        return false;
    }

    String url =
        "https://api.open-meteo.com/v1/forecast?latitude=" + String(config.latitude, 6) +
        "&longitude=" + String(config.longitude, 6) +
        "&current=temperature_2m,relative_humidity_2m,surface_pressure,weather_code,wind_speed_10m,precipitation"
        "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_sum,precipitation_probability_max,wind_speed_10m_max"
        "&hourly=temperature_2m,weather_code,precipitation_probability,precipitation,wind_speed_10m"
        "&forecast_days=4&timezone=auto";

    WiFiClientSecure client;
    // HTTPS chrani prenos. Overeni certifikatu doplnime spolu se spravou CA
    // pro vsechny internetove providery, aby aktualizace certifikatu neblokovala zarizeni.
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, url)) {
        weatherData.status.recordError("HTTPS begin failed");
        return false;
    }
    http.setConnectTimeout(ConnectTimeoutMs);
    http.setTimeout(ResponseTimeoutMs);
    http.useHTTP10(true);

    const int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        weatherData.status.recordError("HTTP " + String(httpCode));
        http.end();
        return false;
    }

    String parseError;
    const bool success = parseResponse(http.getStream(), weatherData, parseError);
    http.end();
    if (!success) {
        weatherData.status.recordError(parseError);
        return false;
    }

    weatherData.provider = "Open-Meteo";
    weatherData.lastUpdateMs = millis();
    weatherData.status.recordSuccess();
    Serial.printf("[WEATHER] Open-Meteo: %.1f C, %d %%, %u dnu, %u hodinovych bodu\n",
                  weatherData.outdoorTempC,
                  weatherData.outdoorHumidityPercent,
                  weatherData.dailyCount,
                  weatherData.hourlyCount);
    return true;
}

bool OpenMeteoClient::parseResponse(Stream& stream, WeatherData& weatherData, String& error) {
    JsonDocument doc;
    const DeserializationError jsonError = deserializeJson(doc, stream);
    if (jsonError) {
        error = "JSON " + String(jsonError.c_str());
        return false;
    }
    if (!hasRequiredWeatherFields(doc)) {
        error = "Missing weather fields";
        return false;
    }

    const JsonObjectConst current = doc["current"].as<JsonObjectConst>();
    weatherData.outdoorTempC = current["temperature_2m"].as<float>();
    weatherData.outdoorHumidityPercent = current["relative_humidity_2m"].as<int>();
    weatherData.surfacePressureHpa = current["surface_pressure"] | 0.0f;
    weatherData.windSpeedKmh = current["wind_speed_10m"] | 0.0f;
    weatherData.currentPrecipitationMm = current["precipitation"] | 0.0f;
    weatherData.weatherCode = current["weather_code"].as<uint8_t>();
    weatherData.conditionText = conditionText(weatherData.weatherCode);

    const JsonObjectConst daily = doc["daily"].as<JsonObjectConst>();
    const JsonArrayConst dailyTimes = daily["time"].as<JsonArrayConst>();
    const JsonArrayConst dailyCodes = daily["weather_code"].as<JsonArrayConst>();
    const JsonArrayConst dailyMax = daily["temperature_2m_max"].as<JsonArrayConst>();
    const JsonArrayConst dailyMin = daily["temperature_2m_min"].as<JsonArrayConst>();
    const JsonArrayConst dailyRain = daily["precipitation_sum"].as<JsonArrayConst>();
    const JsonArrayConst dailyRainChance = daily["precipitation_probability_max"].as<JsonArrayConst>();
    const JsonArrayConst dailyWind = daily["wind_speed_10m_max"].as<JsonArrayConst>();

    weatherData.dailyCount = static_cast<uint8_t>(
        min(static_cast<size_t>(WeatherForecastDayCount), dailyTimes.size()));
    for (size_t i = 0; i < weatherData.dailyCount; ++i) {
        DailyWeatherForecast& item = weatherData.daily[i];
        copyText(item.date, dailyTimes[i].as<const char*>());
        item.weatherCode = dailyCodes[i] | 0;
        item.tempMaxC = dailyMax[i] | 0.0f;
        item.tempMinC = dailyMin[i] | 0.0f;
        item.precipitationMm = dailyRain[i] | 0.0f;
        item.precipitationProbabilityPercent = dailyRainChance[i] | 0;
        item.windMaxKmh = dailyWind[i] | 0.0f;
    }
    if (weatherData.dailyCount > 0) {
        weatherData.tempMaxTodayC = weatherData.daily[0].tempMaxC;
        weatherData.tempMinTodayC = weatherData.daily[0].tempMinC;
    }

    const JsonObjectConst hourly = doc["hourly"].as<JsonObjectConst>();
    const JsonArrayConst hourlyTimes = hourly["time"].as<JsonArrayConst>();
    const JsonArrayConst hourlyTemps = hourly["temperature_2m"].as<JsonArrayConst>();
    const JsonArrayConst hourlyCodes = hourly["weather_code"].as<JsonArrayConst>();
    const JsonArrayConst hourlyRainChance = hourly["precipitation_probability"].as<JsonArrayConst>();
    const JsonArrayConst hourlyRain = hourly["precipitation"].as<JsonArrayConst>();
    const JsonArrayConst hourlyWind = hourly["wind_speed_10m"].as<JsonArrayConst>();

    weatherData.hourlyCount = 0;
    const size_t hourlyAvailable = hourlyTimes.size();
    for (size_t sourceIndex = 0;
         sourceIndex < hourlyAvailable && weatherData.hourlyCount < WeatherHourlySlotCount;
         sourceIndex += 3) {
        HourlyWeatherForecast& item = weatherData.hourly[weatherData.hourlyCount++];
        const char* timestamp = hourlyTimes[sourceIndex].as<const char*>();
        copyText(item.time, timestamp != nullptr && strlen(timestamp) >= 16 ? timestamp + 11 : "--:--");
        item.weatherCode = hourlyCodes[sourceIndex] | 0;
        item.tempC = hourlyTemps[sourceIndex] | 0.0f;
        item.precipitationProbabilityPercent = hourlyRainChance[sourceIndex] | 0;
        item.precipitationMm = hourlyRain[sourceIndex] | 0.0f;
        item.windKmh = hourlyWind[sourceIndex] | 0.0f;
    }

    return true;
}