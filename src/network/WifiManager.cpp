#include "WifiManager.h"
#include <ESPmDNS.h>

WifiManager::WifiManager() : _connected(false) {
}

void WifiManager::begin(const String& ssid, const String& password, const String& hostname) {
    _ssid = ssid;
    _password = password;
    _hostname = hostname;

    if (_ssid.isEmpty() || _ssid == "VASE_WIFI") {
        Serial.println("[WIFI] UPOZORNENI: Wi-Fi neni nakonfigurovano (vychozi placeholder).");
        return;
    }

    Serial.printf("[WIFI] Nastavuji hostname '%s' a pripojuji k '%s'...\n", _hostname.c_str(), _ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(_hostname.c_str());
    WiFi.begin(_ssid.c_str(), _password.c_str());
}

void WifiManager::loop() {
    if (_ssid.isEmpty() || _ssid == "VASE_WIFI") return;

    bool currentConnected = (WiFi.status() == WL_CONNECTED);

    if (currentConnected && !_connected) {
        _connected = true;
        Serial.printf("[WIFI] Pripojeno! IP: %s, RSSI: %d dBm\n", 
                      WiFi.localIP().toString().c_str(), 
                      WiFi.RSSI());

        // Spustit mDNS pro 'http://dashboard.local'
        if (MDNS.begin(_hostname.c_str())) {
            Serial.printf("[WIFI] mDNS responder bezi: http://%s.local/\n", _hostname.c_str());
        }
    } else if (!currentConnected && _connected) {
        _connected = false;
        Serial.println("[WIFI] Spojeni ztraceno, pokus o znovupripojeni...");
    } else if (!currentConnected) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 15000) {
            _lastReconnectAttempt = now;
            WiFi.reconnect();
        }
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

int8_t WifiManager::getRssi() const {
    return isConnected() ? WiFi.RSSI() : 0;
}

String WifiManager::getIpAddress() const {
    return isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
}
