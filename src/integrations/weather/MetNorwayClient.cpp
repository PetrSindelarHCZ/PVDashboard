#include "MetNorwayClient.h"
#include "WeatherTls.h"
#include "../../../include/Version.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ctime>
#include <cstring>
#include <math.h>

namespace {
constexpr int32_t ConnectTimeoutMs = 3000;
constexpr uint16_t ResponseTimeoutMs = 12000;
constexpr uint32_t MinimumCacheSeconds = 60;
constexpr uint32_t MaximumCacheSeconds = 21600;

template <size_t N>
void copyText(char (&destination)[N], const char* source) {
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    strlcpy(destination, source, N);
}

uint8_t symbolToWeatherCode(const char* symbol) {
    if (symbol == nullptr) return 3;
    const String value(symbol);
    if (value.startsWith("clearsky")) return 0;
    if (value.startsWith("fair")) return 1;
    if (value.startsWith("partlycloudy")) return 2;
    if (value.startsWith("cloudy")) return 3;
    if (value.startsWith("fog")) return 45;
    if (value.indexOf("thunder") >= 0) return 95;
    if (value.indexOf("snow") >= 0 || value.indexOf("sleet") >= 0) return 71;
    if (value.indexOf("rainshowers") >= 0) return 80;
    if (value.indexOf("rain") >= 0) return 61;
    return 3;
}

const char* conditionForCode(uint8_t code) {
    switch (code) {
        case 0: return "Jasno";
        case 1: return "Prevazne jasno";
        case 2: return "Polojasno";
        case 3: return "Zatazeno";
        case 45: return "Mlha";
        case 61: return "Dest";
        case 71: return "Snezeni";
        case 80: return "Destove prehanky";
        case 95: return "Bourka";
        default: return "Neznamy stav";
    }
}

const char* forecastSymbol(const JsonObjectConst& data) {
    const char* symbol = data["next_1_hours"]["summary"]["symbol_code"];
    if (symbol == nullptr) symbol = data["next_6_hours"]["summary"]["symbol_code"];
    if (symbol == nullptr) symbol = data["next_12_hours"]["summary"]["symbol_code"];
    return symbol;
}

JsonObjectConst precipitationDetails(const JsonObjectConst& data) {
    JsonObjectConst details = data["next_1_hours"]["details"].as<JsonObjectConst>();
    if (!details.isNull()) return details;
    return data["next_6_hours"]["details"].as<JsonObjectConst>();
}

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned adjustedMonth = month > 2 ? month - 3 : month + 9;
    const unsigned dayOfYear =
        (153 * adjustedMonth + 2) / 5 + day - 1;
    const unsigned dayOfEra =
        yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 +
        dayOfYear;
    return static_cast<int64_t>(era) * 146097 +
           static_cast<int64_t>(dayOfEra) - 719468;
}

time_t utcTimestamp(
    int year,
    unsigned month,
    unsigned day,
    unsigned hour,
    unsigned minute,
    unsigned second) {
    return static_cast<time_t>(
        daysFromCivil(year, month, day) * 86400 +
        hour * 3600 + minute * 60 + second);
}
bool parseUtcTime(const char* text, time_t& utc, tm& local) {
    if (text == nullptr || strlen(text) < 19) return false;
    tm parsed{};
    if (sscanf(text, "%d-%d-%dT%d:%d:%dZ",
               &parsed.tm_year,
               &parsed.tm_mon,
               &parsed.tm_mday,
               &parsed.tm_hour,
               &parsed.tm_min,
               &parsed.tm_sec) != 6) {
        return false;
    }
    utc = utcTimestamp(
        parsed.tm_year,
        static_cast<unsigned>(parsed.tm_mon),
        static_cast<unsigned>(parsed.tm_mday),
        static_cast<unsigned>(parsed.tm_hour),
        static_cast<unsigned>(parsed.tm_min),
        static_cast<unsigned>(parsed.tm_sec));
    localtime_r(&utc, &local);
    return true;
}

bool parseHttpDate(const String& value, time_t& output) {
    if (value.isEmpty()) return false;
    tm parsed{};
    char* end = strptime(value.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &parsed);
    if (end == nullptr || *end != '\0') return false;
    output = utcTimestamp(
        parsed.tm_year + 1900,
        static_cast<unsigned>(parsed.tm_mon + 1),
        static_cast<unsigned>(parsed.tm_mday),
        static_cast<unsigned>(parsed.tm_hour),
        static_cast<unsigned>(parsed.tm_min),
        static_cast<unsigned>(parsed.tm_sec));
    return output > 0;
}
}

bool MetNorwayClient::update(const WeatherConfig& config, WeatherData& weatherData) {
    if (config.latitude < -90.0 || config.latitude > 90.0 ||
        config.longitude < -180.0 || config.longitude > 180.0 ||
        (config.latitude == 0.0 && config.longitude == 0.0)) {
        weatherData.status.recordError("Location not configured");
        return false;
    }

    const String url =
        "https://api.met.no/weatherapi/locationforecast/2.0/compact?lat=" +
        String(config.latitude, 4) + "&lon=" + String(config.longitude, 4);

    Serial.printf("[WEATHER] MET heap pred TLS: free=%u, maxBlock=%u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    WiFiClientSecure client;
    configureWeatherTls(client);

    HTTPClient http;
    if (!http.begin(client, url)) {
        weatherData.status.recordError("HTTPS begin failed");
        return false;
    }

    http.setConnectTimeout(ConnectTimeoutMs);
    http.setTimeout(ResponseTimeoutMs);
    http.useHTTP10(true);
    http.setUserAgent(
        "PVDashboard/" FIRMWARE_VERSION " (+https://github.com/PetrSindelarHCZ/PVDashboard)");

    const char* headerKeys[] = {"Expires", "Date", "X-ErrorClass"};
    http.collectHeaders(headerKeys, 3);

    const int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        String detail = "HTTP " + String(httpCode);
        const String errorClass = http.header("X-ErrorClass");
        if (!errorClass.isEmpty()) detail += " " + errorClass;

        String responseBody = http.getString();
        responseBody.replace("\r", " ");
        responseBody.replace("\n", " ");
        responseBody.trim();
        if (responseBody.length() > 160) {
            responseBody = responseBody.substring(0, 160);
        }
        if (!responseBody.isEmpty()) detail += ": " + responseBody;

        weatherData.status.recordError(detail);
        Serial.printf("[WEATHER] MET Norway chyba: %s | free=%u, maxBlock=%u\n",
                      detail.c_str(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        http.end();
        return false;
    }

    WeatherData parsedData = weatherData;
    String parseError;
    const bool success =
        parseResponse(http.getStream(), parsedData, parseError);
    if (success) {
        updateCachePolicy(http.header("Date"), http.header("Expires"));
    }
    http.end();

    if (!success) {
        weatherData.status.recordError(parseError);
        Serial.printf("[WEATHER] MET parse chyba: %s | free=%u, maxBlock=%u\n",
                      parseError.c_str(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        return false;
    }

    weatherData = parsedData;
    weatherData.provider = "MET Norway";
    weatherData.lastUpdateMs = millis();
    weatherData.status.recordSuccess();

    Serial.printf(
        "[WEATHER] MET Norway: %.1f C, %d %%, %u dnu, %u hodinovych bodu | "
        "free=%u, maxBlock=%u\n",
        weatherData.outdoorTempC,
        weatherData.outdoorHumidityPercent,
        weatherData.dailyCount,
        weatherData.hourlyCount,
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap());
    return true;
}
void MetNorwayClient::resetCache() {
    _cacheSeconds = 0;
}

uint32_t MetNorwayClient::recommendedPollIntervalSeconds(uint32_t configuredSeconds) const {
    return max(configuredSeconds, _cacheSeconds);
}

void MetNorwayClient::updateCachePolicy(
    const String& dateHeader,
    const String& expiresHeader) {
    time_t responseDate = 0;
    time_t expires = 0;
    if (parseHttpDate(dateHeader, responseDate) &&
        parseHttpDate(expiresHeader, expires) &&
        expires > responseDate) {
        _cacheSeconds = constrain(
            static_cast<uint32_t>(expires - responseDate),
            MinimumCacheSeconds,
            MaximumCacheSeconds);
    }
}

bool MetNorwayClient::parseResponse(
    Stream& stream,
    WeatherData& weatherData,
    String& error) {
    // Locationforecast může vrátit desítky až stovky časových bodů. Držet celé
    // filtrované pole timeseries v jednom JsonDocument vedlo po rozběhu WebUI
    // k JSON NoMemory. Proto obálku pouze proskenujeme a každý bod parsujeme
    // samostatně přímo ze streamu.
    constexpr char TimeseriesKey[] = "\"timeseries\"";
    constexpr uint32_t StreamWaitMs = ResponseTimeoutMs;

    auto readByte = [&](bool peekOnly) -> int {
        const uint32_t started = millis();
        for (;;) {
            const int value = peekOnly ? stream.peek() : stream.read();
            if (value >= 0) return value;
            if (millis() - started >= StreamWaitMs) return -1;
            delay(1);
        }
    };

    size_t matched = 0;
    while (matched < sizeof(TimeseriesKey) - 1) {
        const int value = readByte(false);
        if (value < 0) {
            error = "MET timeseries timeout";
            return false;
        }
        const char ch = static_cast<char>(value);
        if (ch == TimeseriesKey[matched]) {
            ++matched;
        } else {
            matched = ch == TimeseriesKey[0] ? 1 : 0;
        }
    }

    bool arrayStarted = false;
    while (!arrayStarted) {
        const int value = readByte(false);
        if (value < 0) {
            error = "Missing MET timeseries array";
            return false;
        }
        if (value == '[') arrayStarted = true;
    }

    JsonDocument filter;
    filter["time"] = true;
    filter["data"]["instant"]["details"]["air_pressure_at_sea_level"] = true;
    filter["data"]["instant"]["details"]["air_temperature"] = true;
    filter["data"]["instant"]["details"]["relative_humidity"] = true;
    filter["data"]["instant"]["details"]["wind_speed"] = true;
    filter["data"]["next_1_hours"]["summary"]["symbol_code"] = true;
    filter["data"]["next_1_hours"]["details"]["precipitation_amount"] = true;
    filter["data"]["next_1_hours"]["details"]["probability_of_precipitation"] = true;
    filter["data"]["next_6_hours"]["summary"]["symbol_code"] = true;
    filter["data"]["next_6_hours"]["details"]["precipitation_amount"] = true;
    filter["data"]["next_6_hours"]["details"]["probability_of_precipitation"] = true;
    filter["data"]["next_12_hours"]["summary"]["symbol_code"] = true;

    weatherData.dailyCount = 0;
    weatherData.hourlyCount = 0;
    time_t lastHourlyUtc = 0;
    bool haveCurrent = false;
    uint16_t parsedPoints = 0;

    for (;;) {
        int value = readByte(true);
        while (value >= 0 &&
               (value == ',' || value == ' ' || value == '\r' ||
                value == '\n' || value == '\t')) {
            stream.read();
            value = readByte(true);
        }

        if (value < 0) {
            error = "MET timeseries truncated";
            return false;
        }
        if (value == ']') {
            stream.read();
            break;
        }
        if (value != '{') {
            error = "Unexpected MET timeseries token";
            return false;
        }

        JsonDocument pointDoc;
        const DeserializationError jsonError = deserializeJson(
            pointDoc,
            stream,
            DeserializationOption::Filter(filter));
        if (jsonError) {
            error = "JSON point " + String(parsedPoints) + " " + String(jsonError.c_str());
            return false;
        }
        ++parsedPoints;

        const JsonObjectConst point = pointDoc.as<JsonObjectConst>();
        const char* timestamp = point["time"];
        time_t utc = 0;
        tm local{};
        if (!parseUtcTime(timestamp, utc, local)) continue;

        const JsonObjectConst data = point["data"].as<JsonObjectConst>();
        const JsonObjectConst instant =
            data["instant"]["details"].as<JsonObjectConst>();
        if (instant["air_temperature"].isNull()) continue;

        if (!haveCurrent) {
            if (instant["relative_humidity"].isNull()) {
                error = "Missing MET current data";
                return false;
            }
            weatherData.outdoorTempC = instant["air_temperature"].as<float>();
            weatherData.outdoorHumidityPercent =
                static_cast<int>(roundf(instant["relative_humidity"].as<float>()));
            weatherData.surfacePressureHpa =
                instant["air_pressure_at_sea_level"] | 0.0f;
            weatherData.windSpeedKmh =
                (instant["wind_speed"] | 0.0f) * 3.6f;

            const JsonObjectConst currentPrecipitation =
                precipitationDetails(data);
            weatherData.currentPrecipitationMm =
                currentPrecipitation["precipitation_amount"] | 0.0f;
            weatherData.weatherCode =
                symbolToWeatherCode(forecastSymbol(data));
            weatherData.conditionText =
                conditionForCode(weatherData.weatherCode);
            haveCurrent = true;
        }

        char date[11];
        snprintf(date, sizeof(date), "%04d-%02d-%02d",
                 local.tm_year + 1900,
                 local.tm_mon + 1,
                 local.tm_mday);

        int dayIndex = -1;
        for (uint8_t i = 0; i < weatherData.dailyCount; ++i) {
            if (strcmp(weatherData.daily[i].date, date) == 0) {
                dayIndex = i;
                break;
            }
        }

        if (dayIndex < 0 && weatherData.dailyCount >= WeatherForecastDayCount) {
            // Timeseries je chronologická. Jakmile začíná pátý den, další
            // body už pro čtyřdenní dashboard nepotřebujeme.
            break;
        }

        if (dayIndex < 0) {
            dayIndex = weatherData.dailyCount++;
            DailyWeatherForecast& created = weatherData.daily[dayIndex];
            created = {};
            copyText(created.date, date);
            created.tempMaxC = instant["air_temperature"].as<float>();
            created.tempMinC = created.tempMaxC;
            created.weatherCode = symbolToWeatherCode(forecastSymbol(data));
        }

        DailyWeatherForecast& day = weatherData.daily[dayIndex];
        const float temperature = instant["air_temperature"].as<float>();
        day.tempMaxC = max(day.tempMaxC, temperature);
        day.tempMinC = min(day.tempMinC, temperature);
        day.windMaxKmh = max(
            day.windMaxKmh,
            (instant["wind_speed"] | 0.0f) * 3.6f);

        const JsonObjectConst precip = precipitationDetails(data);
        day.precipitationMm += precip["precipitation_amount"] | 0.0f;
        if (!precip["probability_of_precipitation"].isNull()) {
            day.hasPrecipitationProbability = true;
            day.precipitationProbabilityPercent = max(
                day.precipitationProbabilityPercent,
                precip["probability_of_precipitation"].as<uint8_t>());
        }
        if (local.tm_hour >= 11 && local.tm_hour <= 14) {
            day.weatherCode = symbolToWeatherCode(forecastSymbol(data));
        }

        if (weatherData.hourlyCount < WeatherHourlySlotCount &&
            local.tm_hour % 3 == 0 &&
            (lastHourlyUtc == 0 || utc - lastHourlyUtc >= 3 * 3600)) {
            HourlyWeatherForecast& hour =
                weatherData.hourly[weatherData.hourlyCount++];
            hour = {};
            copyText(hour.date, date);
            snprintf(hour.time, sizeof(hour.time), "%02d:%02d",
                     local.tm_hour,
                     local.tm_min);
            hour.tempC = temperature;
            hour.windKmh = (instant["wind_speed"] | 0.0f) * 3.6f;
            hour.precipitationMm =
                precip["precipitation_amount"] | 0.0f;
            if (!precip["probability_of_precipitation"].isNull()) {
                hour.hasPrecipitationProbability = true;
                hour.precipitationProbabilityPercent =
                    precip["probability_of_precipitation"].as<uint8_t>();
            }
            hour.weatherCode = symbolToWeatherCode(forecastSymbol(data));
            lastHourlyUtc = utc;
        }
    }

    if (!haveCurrent) {
        error = "Missing MET current data";
        return false;
    }
    if (weatherData.dailyCount == 0) {
        error = "No MET forecast days";
        return false;
    }

    weatherData.tempMaxTodayC = weatherData.daily[0].tempMaxC;
    weatherData.tempMinTodayC = weatherData.daily[0].tempMinC;

    Serial.printf("[WEATHER] MET stream parser: %u bodu, %u dnu, %u hodinovych bodu\n",
                  parsedPoints,
                  weatherData.dailyCount,
                  weatherData.hourlyCount);
    return true;
}