#include "AZRouterClient.h"
#include "../../diagnostics/Performance.h"

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

    bool powerSuccess = false;

    // 1. Čtení /api/v1/power (výkon vytěžování a energie)
    {
        String url = "http://" + _host + ":" + String(_port) + "/api/v1/power";
        Performance::Scope timing(Performance::AzPower);
        _http.begin(url);
        _http.setTimeout(1500);

        int httpCode = _http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = _http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                // Výkon vytěžování (output.power)
                // id 3 = celkový výkon, id 0,1,2 = fáze L1, L2, L3
                if (doc["output"]["power"].is<JsonArray>()) {
                    float sumPower = 0.0f;
                    float id3Power = 0.0f;
                    bool hasId3 = false;

                    for (JsonObject item : doc["output"]["power"].as<JsonArray>()) {
                        int id = item["id"] | -1;
                        float val = item["value"] | 0.0f;
                        if (id == 3) {
                            id3Power = val;
                            hasId3 = true;
                        } else if (id >= 0 && id <= 2) {
                            sumPower += val;
                        }
                    }
                    azData.routedPowerW = (hasId3 && id3Power > 0) ? id3Power : sumPower;
                }

                // Dnešní vytěžená energie (output.energy: index/id 4 = Saved Energy Today)
                if (doc["output"]["energy"].is<JsonArray>()) {
                    for (JsonObject item : doc["output"]["energy"].as<JsonArray>()) {
                        int id = item["id"] | -1;
                        if (id == 4) { // Dnes
                            azData.routedEnergyTodayKWh = item["value"] | 0.0f;
                        }
                    }
                }

                // Měřený tok ze sítě na vstupu AZRouteru (input.power: fáze 0,1,2)
                if (doc["input"]["power"].is<JsonArray>()) {
                    float gridSum = 0.0f;
                    for (JsonObject item : doc["input"]["power"].as<JsonArray>()) {
                        int id = item["id"] | -1;
                        if (id >= 0 && id <= 2) {
                            gridSum += (item["value"] | 0.0f);
                        }
                    }
                    azData.gridPowerW = gridSum;
                }

                powerSuccess = true;
            } else {
                azData.status.recordError("Power JSON Err");
            }
        } else {
            azData.status.recordError("HTTP " + String(httpCode));
        }
        _http.end();
    }

    // 2. Čtení /api/v1/status (teplota zařízení)
    {
        String url = "http://" + _host + ":" + String(_port) + "/api/v1/status";
        Performance::Scope timing(Performance::AzStatus);
        _http.begin(url);
        _http.setTimeout(1500);

        int httpCode = _http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = _http.getString();
            JsonDocument doc;
            if (!deserializeJson(doc, payload)) {
                if (doc["system"]["temperature"].is<float>()) {
                    // Teplota zařízení / chladiče
                    float temp = doc["system"]["temperature"].as<float>();
                    if (azData.boilerTempC <= 0.0f) {
                        azData.boilerTempC = temp;
                    }
                }
            }
        }
        _http.end();
    }

    // 3. Čtení /api/v1/devices (teplota bojleru / čidla zařízení)
    {
        String url = "http://" + _host + ":" + String(_port) + "/api/v1/devices";
        Performance::Scope timing(Performance::AzDevices);
        _http.begin(url);
        _http.setTimeout(1500);

        int httpCode = _http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = _http.getString();
            JsonDocument doc;
            if (!deserializeJson(doc, payload)) {
                // Přijmout jak variantu s jedním objektem, tak pole
                if (doc["power"]["temperature"].is<float>()) {
                    float devTemp = doc["power"]["temperature"].as<float>();
                    if (devTemp > 0.0f) {
                        azData.boilerTempC = devTemp;
                    }
                }
            }
        }
        _http.end();
    }

    if (powerSuccess) {
        azData.lastUpdateMs = millis();
        azData.status.recordSuccess();
        Serial.printf("[AZROUTER] Vytěžování: %.0f W | Bojler: %.1f °C | Dnes: %.1f kWh | Síť AZ: %.0f W\n",
                      azData.routedPowerW,
                      azData.boilerTempC,
                      azData.routedEnergyTodayKWh,
                      azData.gridPowerW);
        return true;
    }

    return false;
}

