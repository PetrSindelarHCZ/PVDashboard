#include "WifiManager.h"
#include <ESPmDNS.h>
#include <esp_wifi.h>

WifiManager::WifiManager() : _connected(false) {
}

const char* WifiManager::wlStatusToString(wl_status_t status) {
    switch (status) {
        case WL_NO_SHIELD: return "NO_SHIELD";
        case WL_IDLE_STATUS: return "IDLE";
        case WL_NO_SSID_AVAIL: return "SSID_NOT_FOUND";
        case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
        case WL_CONNECTED: return "CONNECTED";
        case WL_CONNECT_FAILED: return "CONNECT_FAILED";
        case WL_CONNECTION_LOST: return "CONNECTION_LOST";
        case WL_DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

void WifiManager::onStatusChange(WifiStatusCallback callback) {
    _statusCallback = callback;
}

void WifiManager::begin(const String& ssid, const String& password, const String& hostname) {
    const bool wasOnline = _connected || _configAccessPoint || WiFi.status() == WL_CONNECTED;

    _ssid = ssid;
    _password = password;
    _hostname = hostname;
    _connected = false;
    _configAccessPoint = false;
    _lastReconnectAttempt = millis();

    if (wasOnline && _statusCallback) {
        _statusCallback(false, "0.0.0.0");
    }

    MDNS.end();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(false);
    delay(100);
    WiFi.mode(WIFI_STA);

    // Event handlery registrujeme jen jednou. begin() lze volat znovu při
    // změně konfigurace bez hromadění duplicitních callbacků.
    if (!_eventsRegistered) {
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                Serial.printf("[WIFI-EVENT] Odpojeno! Dvod (reason code): %d\n", info.wifi_sta_disconnected.reason);
            } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
                Serial.println("[WIFI-EVENT] AP prirazeno (STA_CONNECTED).");
            } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                Serial.printf("[WIFI-EVENT] IP pridelena: %s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
            }
        });
        _eventsRegistered = true;
    }

    if (_ssid.isEmpty() || _ssid == "VASE_WIFI") {
        Serial.println("[WIFI] Wi-Fi neni nakonfigurovano, spoustim konfiguracni AP.");
        startConfigAccessPoint();
        return;
    }

    Serial.printf("\n[WIFI] Nastavuji kompatibilni rezim (802.11b/g/n, 19.5dBm, LR off)...\n");
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setAutoReconnect(true);
    WiFi.setHostname(_hostname.c_str());
    WiFi.persistent(false);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_ps(WIFI_PS_NONE);

    Serial.printf("[WIFI] Pripojuji k '%s'...\n", _ssid.c_str());
    WiFi.begin(_ssid.c_str(), _password.c_str());
}

bool WifiManager::waitForConnection(uint32_t timeoutMs) {
    if (_ssid.isEmpty() || _ssid == "VASE_WIFI") return false;

    Serial.printf("[WIFI] Cekam na pripojeni k '%s' (max %u s)...", _ssid.c_str(), timeoutMs / 1000);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < timeoutMs)) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _connected = true;
        Serial.printf("[WIFI] Uspesne pripojeno! IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        if (MDNS.begin(_hostname.c_str())) {
            Serial.printf("[WIFI] mDNS responder bezi: http://%s.local/\n", _hostname.c_str());
        }
        return true;
    }

    Serial.printf("[WIFI] Timeout pripojeni. Aktualni stav: %s\n", wlStatusToString(WiFi.status()));
    startConfigAccessPoint();
    return false;
}

void WifiManager::disconnectToConfigAccessPoint() {
    const bool wasConnected = _connected || WiFi.status() == WL_CONNECTED;
    _connected = false;
    if (wasConnected && _statusCallback) {
        _statusCallback(false, "0.0.0.0");
    }
    Serial.println("[WIFI] Rucni odpojeni od STA, prechazim do konfiguracniho AP.");
    startConfigAccessPoint();
}

void WifiManager::loop() {
    if (_configAccessPoint || _ssid.isEmpty() || _ssid == "VASE_WIFI") return;

    bool currentConnected = (WiFi.status() == WL_CONNECTED);

    if (currentConnected && !_connected) {
        _connected = true;
        String ip = WiFi.localIP().toString();
        Serial.printf("[WIFI] Pripojeno! IP: %s, RSSI: %d dBm\n", ip.c_str(), WiFi.RSSI());

        MDNS.end();
        if (MDNS.begin(_hostname.c_str())) {
            Serial.printf("[WIFI] mDNS responder bezi: http://%s.local/\n", _hostname.c_str());
        }

        if (_statusCallback) {
            _statusCallback(true, ip);
        }
    } else if (!currentConnected && _connected) {
        _connected = false;
        Serial.println("[WIFI] Spojeni ztraceno, pokus o znovupripojeni...");
        if (_statusCallback) {
            _statusCallback(false, "0.0.0.0");
        }
    } else if (!currentConnected) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 10000) {
            _lastReconnectAttempt = now;
            Serial.printf("[WIFI] Stav: %s -> zkousim znovu pripojit k '%s'...\n",
                          wlStatusToString(WiFi.status()), _ssid.c_str());
            WiFi.reconnect();
        }
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::isConfigAccessPoint() const {
    return _configAccessPoint;
}

String WifiManager::scanNetworksJson() {
    if (_configAccessPoint) {
        WiFi.mode(WIFI_AP_STA);
    }

    int16_t networkCount = WiFi.scanNetworks(false, true);
    String response = "[";
    for (int16_t index = 0; index < networkCount; ++index) {
        if (index > 0) response += ",";
        response += "{\"ssid\":\"";
        String ssid = WiFi.SSID(index);
        for (size_t character = 0; character < ssid.length(); ++character) {
            char value = ssid[character];
            if (value == '\\' || value == '"') response += '\\';
            response += value;
        }
        response += "\",\"rssi\":" + String(WiFi.RSSI(index));
        response += ",\"secure\":" + String(WiFi.encryptionType(index) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    response += "]";
    WiFi.scanDelete();
    return response;
}

int8_t WifiManager::getRssi() const {
    return isConnected() ? WiFi.RSSI() : 0;
}

String WifiManager::getIpAddress() const {
    if (_configAccessPoint) return WiFi.softAPIP().toString();
    return isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
}

void WifiManager::startConfigAccessPoint() {
    _configAccessPoint = true;
    _connected = false;
    MDNS.end();
    WiFi.disconnect(false);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Dashboard-Setup", "dashboard");
    Serial.printf("[WIFI] Konfiguracni AP: Dashboard-Setup | heslo: dashboard | IP: %s\n",
                  WiFi.softAPIP().toString().c_str());
}
