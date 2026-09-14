#include "AZRouterClient.h"
#include "../../diagnostics/Performance.h"

namespace {
constexpr int32_t ConnectTimeoutMs = 400;
constexpr uint16_t ResponseTimeoutMs = 1000;
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

    // /power je hlavní endpoint. Když neodpoví, další dva požadavky by jen
    // prodloužily blokování stejného nedostupného zařízení.
    JsonDocument powerDoc;
    String powerError;
    if (!getJson("/api/v1/power", powerDoc, Performance::AzPower, powerError)) {
        azData.status.recordError(powerError);
        return false;
    }

    if (powerDoc["output"]["power"].is<JsonArray>()) {
        float sumPower = 0.0f;
        float id3Power = 0.0f;
        bool hasId3 = false;

        for (JsonObject item : powerDoc["output"]["power"].as<JsonArray>()) {
            const int id = item["id"] | -1;
            const float value = item["value"] | 0.0f;
            if (id == 3) {
                id3Power = value;
                hasId3 = true;
            } else if (id >= 0 && id <= 2) {
                sumPower += value;
            }
        }
        azData.routedPowerW = (hasId3 && id3Power > 0.0f) ? id3Power : sumPower;
    }

    if (powerDoc["output"]["energy"].is<JsonArray>()) {
        for (JsonObject item : powerDoc["output"]["energy"].as<JsonArray>()) {
            if ((item["id"] | -1) == 4) {
                azData.routedEnergyTodayKWh = item["value"] | 0.0f;
            }
        }
    }

    if (powerDoc["input"]["power"].is<JsonArray>()) {
        float gridSum = 0.0f;
        for (JsonObject item : powerDoc["input"]["power"].as<JsonArray>()) {
            const int id = item["id"] | -1;
            if (id >= 0 && id <= 2) {
                gridSum += (item["value"] | 0.0f);
            }
        }
        azData.gridPowerW = gridSum;
    }

    // Doplňkové endpointy neovlivňují dostupnost hlavních výkonových dat.
    JsonDocument statusDoc;
    String optionalError;
    if (getJson("/api/v1/status", statusDoc, Performance::AzStatus, optionalError)) {
        if (statusDoc["system"]["temperature"].is<float>() && azData.boilerTempC <= 0.0f) {
            azData.boilerTempC = statusDoc["system"]["temperature"].as<float>();
        }
    } else {
        Serial.printf("[AZROUTER] Volitelný /status selhal: %s\n", optionalError.c_str());
    }

    JsonDocument devicesDoc;
    optionalError = "";
    if (getJson("/api/v1/devices", devicesDoc, Performance::AzDevices, optionalError)) {
        if (devicesDoc["power"]["temperature"].is<float>()) {
            const float deviceTemp = devicesDoc["power"]["temperature"].as<float>();
            if (deviceTemp > 0.0f) {
                azData.boilerTempC = deviceTemp;
            }
        }
    } else {
        Serial.printf("[AZROUTER] Volitelný /devices selhal: %s\n", optionalError.c_str());
    }

    azData.lastUpdateMs = millis();
    azData.status.recordSuccess();
    Serial.printf("[AZROUTER] Vytěžování: %.0f W | Bojler: %.1f °C | Dnes: %.1f kWh | Síť AZ: %.0f W\n",
                  azData.routedPowerW,
                  azData.boilerTempC,
                  azData.routedEnergyTodayKWh,
                  azData.gridPowerW);
    return true;
}
