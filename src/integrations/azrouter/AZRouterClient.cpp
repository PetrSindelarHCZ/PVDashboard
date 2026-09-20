#include "AZRouterClient.h"
#include <math.h>

namespace {
constexpr int32_t ConnectTimeoutMs = 400;
constexpr uint16_t ResponseTimeoutMs = 1000;

bool readNumber(JsonVariantConst value, float& result) {
    if (value.isNull()) return false;

    const bool numeric =
        value.is<int>() ||
        value.is<unsigned int>() ||
        value.is<long>() ||
        value.is<unsigned long>() ||
        value.is<float>() ||
        value.is<double>();

    if (!numeric) return false;

    result = value.as<float>();
    return isfinite(result);
}
}

AZRouterClient::AZRouterClient() {
}

void AZRouterClient::begin(const String& host, uint16_t port,
                           const String& username,
                           const String& password) {
    _host = host;
    _port = port;
    _username = username;
    _password = password;
    clearSession();

    Serial.printf(
        "[AZROUTER] Inicializován klient HTTP http://%s:%u/ | auth=%s\n",
        _host.c_str(),
        _port,
        credentialsConfigured() ? "configured" : "anonymous");
}

bool AZRouterClient::credentialsConfigured() const {
    return !_username.isEmpty() && !_password.isEmpty();
}

void AZRouterClient::clearSession() {
    _bearerToken = "";
    _sessionCookie = "";
    _loginCompleted = false;
}

String AZRouterClient::firstCookiePair(const String& setCookie) {
    if (setCookie.isEmpty()) return "";

    int end = setCookie.indexOf(';');
    if (end < 0) end = setCookie.length();

    String cookie = setCookie.substring(0, end);
    cookie.trim();
    return cookie;
}

void AZRouterClient::addAuthHeaders(HTTPClient& http) {
    http.addHeader("Accept", "application/json");
    if (!_bearerToken.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + _bearerToken);
    } else if (!_sessionCookie.isEmpty()) {
        http.addHeader("Cookie", _sessionCookie);
    }
}

bool AZRouterClient::login(String& errorMessage) {
    errorMessage = "";

    if (!credentialsConfigured()) {
        errorMessage = "Credentials not configured";
        return false;
    }

    const String url =
        "http://" + _host + ":" + String(_port) + "/api/v1/login";

    HTTPClient http;
    if (!http.begin(url)) {
        errorMessage = "Login HTTP begin failed";
        return false;
    }

    http.setConnectTimeout(ConnectTimeoutMs);
    http.setTimeout(ResponseTimeoutMs);

    const char* headerKeys[] = {"Set-Cookie"};
    http.collectHeaders(headerKeys, 1);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");

    JsonDocument requestDoc;
    JsonObject data = requestDoc["data"].to<JsonObject>();
    data["username"] = _username;
    data["password"] = _password;

    String payload;
    serializeJson(requestDoc, payload);

    const int httpCode = http.POST(payload);
    if (httpCode < 200 || httpCode >= 300) {
        errorMessage = "Login HTTP " + String(httpCode);
        http.end();
        clearSession();
        return false;
    }

    const String setCookie = http.header("Set-Cookie");
    JsonDocument responseDoc;
    const DeserializationError jsonError =
        deserializeJson(responseDoc, http.getStream());
    http.end();

    clearSession();

    if (!jsonError) {
        const char* keys[] = {
            "token", "access_token", "accessToken", "jwt", "session"
        };

        for (const char* key : keys) {
            const char* value = responseDoc[key].as<const char*>();
            if (value != nullptr && value[0] != '\0') {
                _bearerToken = value;
                break;
            }
        }

        if (_bearerToken.isEmpty() && responseDoc["data"].is<JsonObject>()) {
            JsonObjectConst responseData = responseDoc["data"].as<JsonObjectConst>();
            for (const char* key : keys) {
                const char* value = responseData[key].as<const char*>();
                if (value != nullptr && value[0] != '\0') {
                    _bearerToken = value;
                    break;
                }
            }
        }
    }

    if (_bearerToken.isEmpty()) {
        _sessionCookie = firstCookiePair(setCookie);
    }
    _loginCompleted = true;

    if (!_bearerToken.isEmpty()) {
        Serial.println("[AZROUTER][AUTH] Přihlášení OK, používám Bearer token.");
    } else if (!_sessionCookie.isEmpty()) {
        Serial.println("[AZROUTER][AUTH] Přihlášení OK, používám session cookie.");
    } else {
        // Some firmware variants can return HTTP 2xx without a token in JSON.
        // Continue and let the following authenticated endpoint reveal whether
        // the login is IP/session based or insufficient.
        Serial.println(
            "[AZROUTER][AUTH] Login HTTP OK, ale odpověď neobsahuje token ani cookie.");
    }

    return true;
}

bool AZRouterClient::getJson(const char* path,
                             JsonDocument& doc,
                             Performance::Metric metric,
                             String& errorMessage,
                             bool allowRelogin) {
    const String url = "http://" + _host + ":" + String(_port) + path;
    Performance::Scope timing(metric);

    HTTPClient http;
    if (!http.begin(url)) {
        errorMessage = "HTTP begin failed";
        return false;
    }

    http.setConnectTimeout(ConnectTimeoutMs);
    http.setTimeout(ResponseTimeoutMs);
    addAuthHeaders(http);

    const int httpCode = http.GET();

    if ((httpCode == HTTP_CODE_UNAUTHORIZED || httpCode == HTTP_CODE_FORBIDDEN) &&
        allowRelogin && credentialsConfigured()) {
        http.end();
        clearSession();

        String loginError;
        if (!login(loginError)) {
            errorMessage = loginError;
            return false;
        }

        return getJson(path, doc, metric, errorMessage, false);
    }

    if (httpCode != HTTP_CODE_OK) {
        errorMessage = "HTTP " + String(httpCode);
        http.end();
        return false;
    }

    const DeserializationError jsonError =
        deserializeJson(doc, http.getStream());
    http.end();

    if (jsonError) {
        errorMessage = "JSON " + String(jsonError.c_str());
        return false;
    }

    return true;
}

bool AZRouterClient::update(AZRouterData& azData) {
    if (_host.isEmpty()) {
        azData.status.recordError("No Host");
        return false;
    }

    const bool partialCredentials =
        (!_username.isEmpty() && _password.isEmpty()) ||
        (_username.isEmpty() && !_password.isEmpty());

    if (partialCredentials) {
        azData.authenticated = false;
        azData.authMode = "invalid";
        azData.status.recordError("Incomplete credentials");
        return false;
    }

    if (credentialsConfigured() && !_loginCompleted) {
        String loginError;
        if (!login(loginError)) {
            azData.authenticated = false;
            azData.authMode = "login-failed";
            azData.status.recordError(loginError);
            return false;
        }
    }

    azData.authenticated =
        credentialsConfigured() && _loginCompleted;
    azData.authMode =
        !_bearerToken.isEmpty() ? "bearer" :
        (!_sessionCookie.isEmpty() ? "cookie" :
         (credentialsConfigured() ? "login-no-session" : "anonymous"));

    // /power is the mandatory endpoint. Optional endpoint failures must not
    // invalidate otherwise usable power/grid data.
    JsonDocument powerDoc;
    String powerError;
    if (!getJson("/api/v1/power", powerDoc, Performance::AzPower, powerError)) {
        azData.authenticated =
            !_bearerToken.isEmpty() || !_sessionCookie.isEmpty();
        azData.status.recordError(powerError);
        return false;
    }

    // A successful cycle rebuilds per-metric validity from the received JSON.
    azData.gridPowerW = 0.0f;
    azData.hasGridPower = false;
    azData.routedPowerW = 0.0f;
    azData.hasRoutedPower = false;
    azData.routedEnergyTodayKWh = 0.0f;
    azData.hasRoutedEnergyToday = false;
    azData.boilerTempC = 0.0f;
    azData.hasBoilerTemp = false;
    azData.systemTempC = 0.0f;
    azData.hasSystemTemp = false;

    // Master routed power:
    // output.power id 0..2 = routed power per phase
    // output.power id 3    = routed total
    if (powerDoc["output"]["power"].is<JsonArray>()) {
        float phaseSum = 0.0f;
        bool hasPhaseValue = false;
        float routedTotal = 0.0f;
        bool hasRoutedTotal = false;

        for (JsonObjectConst item : powerDoc["output"]["power"].as<JsonArrayConst>()) {
            const int id = item["id"] | -1;
            float value = 0.0f;
            if (!readNumber(item["value"], value)) continue;

            if (id >= 0 && id <= 2) {
                phaseSum += value;
                hasPhaseValue = true;
            } else if (id == 3) {
                routedTotal = value;
                hasRoutedTotal = true;
            }
        }

        if (hasRoutedTotal) {
            azData.routedPowerW = routedTotal;
            azData.hasRoutedPower = true;
        } else if (hasPhaseValue) {
            azData.routedPowerW = phaseSum;
            azData.hasRoutedPower = true;
        }
    }

    // output.energy id 4 = saved/routed energy today, in kWh.
    if (powerDoc["output"]["energy"].is<JsonArray>()) {
        for (JsonObjectConst item : powerDoc["output"]["energy"].as<JsonArrayConst>()) {
            if ((item["id"] | -1) != 4) continue;

            float value = 0.0f;
            if (readNumber(item["value"], value) && value >= 0.0f) {
                azData.routedEnergyTodayKWh = value;
                azData.hasRoutedEnergyToday = true;
            }
            break;
        }
    }

    // Grid total is the sum of L1..L3 input power. Keep the API sign as-is.
    if (powerDoc["input"]["power"].is<JsonArray>()) {
        float gridSum = 0.0f;
        bool hasGridPhase = false;

        for (JsonObjectConst item : powerDoc["input"]["power"].as<JsonArrayConst>()) {
            const int id = item["id"] | -1;
            if (id < 0 || id > 2) continue;

            float value = 0.0f;
            if (!readNumber(item["value"], value)) continue;

            gridSum += value;
            hasGridPhase = true;
        }

        if (hasGridPhase) {
            azData.gridPowerW = gridSum;
            azData.hasGridPower = true;
        }
    }

    if (!azData.hasRoutedPower &&
        !azData.hasRoutedEnergyToday &&
        !azData.hasGridPower) {
        azData.status.recordError("Power payload missing metrics");
        return false;
    }

    // /status system.temperature is the AZRouter MASTER electronics
    // temperature. It is diagnostic data and must never be presented as the
    // boiler/TUV temperature.
    JsonDocument statusDoc;
    String optionalError;
    if (getJson("/api/v1/status", statusDoc, Performance::AzStatus, optionalError)) {
        float systemTemp = 0.0f;
        if (readNumber(statusDoc["system"]["temperature"], systemTemp) &&
            systemTemp > -40.0f && systemTemp < 125.0f) {
            azData.systemTempC = systemTemp;
            azData.hasSystemTemp = true;
        }
    } else {
        Serial.printf("[AZROUTER] Volitelný /status selhal: %s\n", optionalError.c_str());
    }

    // Smart Slave / TUV device telemetry.
    JsonDocument devicesDoc;
    optionalError = "";
    if (getJson("/api/v1/devices", devicesDoc, Performance::AzDevices, optionalError)) {
        // Known firmware/captures can return either one device object, a list,
        // or {"devices":[...]}. Prefer a Power/Smart Slave (deviceType 1).
        JsonObjectConst boilerDevice;

        if (devicesDoc.is<JsonObject>()) {
            JsonObjectConst root = devicesDoc.as<JsonObjectConst>();
            if (root["devices"].is<JsonArray>()) {
                for (JsonObjectConst item : root["devices"].as<JsonArrayConst>()) {
                    const String deviceType = item["deviceType"] | "";
                    if (deviceType == "1" || item["deviceType"].as<int>() == 1) {
                        boilerDevice = item;
                        break;
                    }
                }
            } else if (root["power"].is<JsonObject>()) {
                boilerDevice = root;
            }
        } else if (devicesDoc.is<JsonArray>()) {
            for (JsonObjectConst item : devicesDoc.as<JsonArrayConst>()) {
                const String deviceType = item["deviceType"] | "";
                if (deviceType == "1" || item["deviceType"].as<int>() == 1) {
                    boilerDevice = item;
                    break;
                }
            }
        }

        if (!boilerDevice.isNull()) {
            float boilerPower = 0.0f;
            if (readNumber(boilerDevice["power"]["totalPower"], boilerPower) &&
                boilerPower >= 0.0f) {
                azData.routedPowerW = boilerPower;
                azData.hasRoutedPower = true;
            }

            float boilerTemp = 0.0f;
            if (readNumber(boilerDevice["power"]["temperature"], boilerTemp) &&
                boilerTemp > 0.0f && boilerTemp < 150.0f) {
                azData.boilerTempC = boilerTemp;
                azData.hasBoilerTemp = true;
            }
        }
    } else {
        Serial.printf("[AZROUTER] Volitelný /devices selhal: %s\n", optionalError.c_str());
    }

    azData.authenticated =
        credentialsConfigured() && _loginCompleted;
    azData.authMode =
        !_bearerToken.isEmpty() ? "bearer" :
        (!_sessionCookie.isEmpty() ? "cookie" :
         (credentialsConfigured() ? "login-no-session" : "anonymous"));

    azData.lastUpdateMs = millis();
    azData.status.recordSuccess();

    Serial.printf(
        "[AZROUTER] auth=%s | valid route=%d energyToday=%d grid=%d boilerTemp=%d systemTemp=%d\n",
        azData.authMode.c_str(),
        azData.hasRoutedPower,
        azData.hasRoutedEnergyToday,
        azData.hasGridPower,
        azData.hasBoilerTemp,
        azData.hasSystemTemp);

    if (azData.hasRoutedPower) {
        Serial.printf("[AZROUTER] Vytěžování: %.0f W\n", azData.routedPowerW);
    }
    if (azData.hasBoilerTemp) {
        Serial.printf("[AZROUTER] Bojler: %.1f °C\n", azData.boilerTempC);
    }
    if (azData.hasRoutedEnergyToday) {
        Serial.printf("[AZROUTER] Dnes: %.1f kWh\n", azData.routedEnergyTodayKWh);
    }
    if (azData.hasGridPower) {
        Serial.printf("[AZROUTER] Síť AZ: %+.0f W\n", azData.gridPowerW);
    }
    if (azData.hasSystemTemp) {
        Serial.printf("[AZROUTER] Teplota jednotky: %.1f °C\n", azData.systemTempC);
    }

    return true;
}
