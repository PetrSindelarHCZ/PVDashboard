#include "AZRouterClient.h"
#include "../../diagnostics/Performance.h"
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

void AZRouterClient::begin(const String& host, uint16_t port) {
    _host = host;
    _port = port;
    Serial.printf("[AZROUTER] Inicializován klient HTTP http://%s:%u/\n", _host.c_str(), _port);
}

bool AZRouterClient::update(AZRouterData& azData) {
    if (_host.isEmpty()) {
        azData.status.recordError("No Host");
        return false;
    }

    auto getJson = [this](const char* path, JsonDocument& doc, Performance::Metric metric, String& errorMessage) {
        const String url = "http://" + _host + ":" + String(_port) + path;
        Performance::Scope timing(metric);

        if (!_http.begin(url)) {
            errorMessage = "HTTP begin failed";
            return false;
        }

        _http.setConnectTimeout(ConnectTimeoutMs);
        _http.setTimeout(ResponseTimeoutMs);
        const int httpCode = _http.GET();
        if (httpCode != HTTP_CODE_OK) {
            errorMessage = "HTTP " + String(httpCode);
            _http.end();
            return false;
        }

        const DeserializationError jsonError = deserializeJson(doc, _http.getStream());
        _http.end();
        if (jsonError) {
            errorMessage = "JSON " + String(jsonError.c_str());
            return false;
        }
        return true;
    };

    // /power is the mandatory endpoint. Optional endpoint failures must not
    // invalidate otherwise usable power/grid data.
    JsonDocument powerDoc;
    String powerError;
    if (!getJson("/api/v1/power", powerDoc, Performance::AzPower, powerError)) {
        azData.status.recordError(powerError);
        return false;
    }

    // A successful cycle rebuilds per-metric validity from the received JSON.
    // Values are zeroed as well so stale/demo numbers can never leak into UI
    // when a particular field is missing from an otherwise valid response.
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
    // power.temperature = actual boiler/TUV probe temperature.
    // power.totalPower  = current device power; prefer it for a single TUV
    // device because it directly describes the boiler load.
    JsonDocument devicesDoc;
    optionalError = "";
    if (getJson("/api/v1/devices", devicesDoc, Performance::AzDevices, optionalError)) {
        float boilerPower = 0.0f;
        if (readNumber(devicesDoc["power"]["totalPower"], boilerPower) &&
            boilerPower >= 0.0f) {
            azData.routedPowerW = boilerPower;
            azData.hasRoutedPower = true;
        }

        float boilerTemp = 0.0f;
        if (readNumber(devicesDoc["power"]["temperature"], boilerTemp) &&
            boilerTemp > 0.0f && boilerTemp < 150.0f) {
            azData.boilerTempC = boilerTemp;
            azData.hasBoilerTemp = true;
        }
    } else {
        Serial.printf("[AZROUTER] Volitelný /devices selhal: %s\n", optionalError.c_str());
    }

    azData.lastUpdateMs = millis();
    azData.status.recordSuccess();

    Serial.printf(
        "[AZROUTER] valid route=%d energyToday=%d grid=%d boilerTemp=%d systemTemp=%d\n",
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
