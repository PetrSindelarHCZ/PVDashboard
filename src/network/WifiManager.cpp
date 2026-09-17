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

void WifiManager::onKnownNetworkLookup(KnownNetworkLookupCallback callback) {
    _knownNetworkLookupCallback = callback;
}

void WifiManager::onAutoNetworkSelected(AutoNetworkSelectedCallback callback) {
    _autoNetworkSelectedCallback = callback;
}

void WifiManager::begin(const String& ssid, const String& password, const String& hostname) {
    const bool wasOnline = _connected || _configAccessPoint || WiFi.status() == WL_CONNECTED;

    _ssid = ssid;
    _password = password;
    _hostname = hostname;
    _connected = false;
    _configAccessPoint = false;
    _lastReconnectAttempt = millis();
    _staFailureStarted = millis();
    _autoScanRunning = false;
    _autoJoinInProgress = false;
    _autoJoinCandidateCount = 0;
    _autoJoinCandidateIndex = 0;

    if (wasOnline && _statusCallback) {
        _statusCallback(false, "0.0.0.0");
    }

    MDNS.end();
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(false);
    delay(100);
    WiFi.mode(WIFI_STA);

    // Event handlery registrujeme jen jednou. begin() lze volat znovu při
    // změně konfigurace bez hromadění duplicitních callbacků.
    if (!_eventsRegistered) {
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                Serial.printf("[WIFI-EVENT] Odpojeno! Duvod (reason code): %d\n", info.wifi_sta_disconnected.reason);
            } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
                Serial.println("[WIFI-EVENT] AP prirazeno (STA_CONNECTED).");
            } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                Serial.printf("[WIFI-EVENT] IP pridelena: %s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
            }
        });
        _eventsRegistered = true;
    }

    if (_ssid.isEmpty() || _ssid == "VASE_WIFI") {
        Serial.println("[WIFI] Wi-Fi neni aktivne vybrana, spoustim AP a hledani znamych siti.");
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
        completeConnection(false);
        return true;
    }

    Serial.printf("[WIFI] Timeout pripojeni. Aktualni stav: %s. Prechazim do AP+STA fallbacku.\n",
                  wlStatusToString(WiFi.status()));
    startConfigAccessPoint();
    return false;
}

void WifiManager::disconnectToConfigAccessPoint() {
    const bool wasConnected = _connected || WiFi.status() == WL_CONNECTED;
    _connected = false;
    if (wasConnected && _statusCallback) {
        _statusCallback(false, "0.0.0.0");
    }
    Serial.println("[WIFI] Rucni odpojeni od STA. AP zustane aktivni; automaticke hledani znamych siti ma 60 s odklad.");
    startConfigAccessPoint(ManualDisconnectGraceMs);
}

void WifiManager::loop() {
    if (_configAccessPoint) {
        serviceConfigAccessPoint();
        return;
    }

    if (_ssid.isEmpty() || _ssid == "VASE_WIFI") {
        startConfigAccessPoint();
        return;
    }

    const bool currentConnected = WiFi.status() == WL_CONNECTED;
    const unsigned long now = millis();

    if (currentConnected && !_connected) {
        completeConnection(false);
        return;
    }

    if (!currentConnected && _connected) {
        _connected = false;
        _staFailureStarted = now;
        Serial.println("[WIFI] Spojeni ztraceno, zkousim obnovit STA pred fallbackem do AP.");
        if (_statusCallback) {
            _statusCallback(false, "0.0.0.0");
        }
    }

    if (!currentConnected) {
        if (_staFailureStarted == 0) _staFailureStarted = now;

        if (now - _staFailureStarted >= StaFallbackTimeoutMs) {
            Serial.println("[WIFI] STA se nepodarilo obnovit do 30 s, prechazim do AP+STA fallbacku.");
            startConfigAccessPoint();
            return;
        }

        if (now - _lastReconnectAttempt >= ReconnectIntervalMs) {
            _lastReconnectAttempt = now;
            Serial.printf("[WIFI] Stav: %s -> zkousim znovu '%s'...\n",
                          wlStatusToString(WiFi.status()), _ssid.c_str());
            WiFi.reconnect();
        }
    }
}

void WifiManager::completeConnection(bool selectedFromFallback) {
    _connected = true;
    _staFailureStarted = 0;
    _lastReconnectAttempt = millis();

    const String ip = WiFi.localIP().toString();
    Serial.printf("[WIFI] Uspesne pripojeno k '%s'! IP: %s | RSSI: %d dBm\n",
                  _ssid.c_str(), ip.c_str(), WiFi.RSSI());

    if (_configAccessPoint) {
        WiFi.softAPdisconnect(true);
        _configAccessPoint = false;
        Serial.println("[WIFI] Znama sit je dostupna, konfiguracni AP vypinam.");
    }

    _autoScanRunning = false;
    _autoJoinInProgress = false;
    _autoJoinCandidateCount = 0;
    _autoJoinCandidateIndex = 0;
    WiFi.scanDelete();
    WiFi.setAutoReconnect(true);

    MDNS.end();
    if (MDNS.begin(_hostname.c_str())) {
        Serial.printf("[WIFI] mDNS responder bezi: http://%s.local/\n", _hostname.c_str());
    }

    if (selectedFromFallback && _autoNetworkSelectedCallback) {
        _autoNetworkSelectedCallback(_ssid, _password);
    }

    if (_statusCallback) {
        _statusCallback(true, ip);
    }
}

void WifiManager::serviceConfigAccessPoint() {
    const unsigned long now = millis();

    if (WiFi.status() == WL_CONNECTED) {
        completeConnection(_autoJoinInProgress);
        return;
    }

    if (_autoJoinInProgress) {
        if (now - _autoJoinStarted >= AutoJoinTimeoutMs) {
            Serial.printf("[WIFI] Znama sit '%s' se nepripojila, zkousim dalsi kandidat.\n", _ssid.c_str());
            WiFi.disconnect(false);
            _autoJoinInProgress = false;
            ++_autoJoinCandidateIndex;
            tryNextKnownNetwork();
        }
        return;
    }

    if (_autoScanRunning) {
        const int16_t networkCount = WiFi.scanComplete();
        if (networkCount == -1) return; // WIFI_SCAN_RUNNING

        _autoScanRunning = false;
        if (networkCount >= 0) {
            processKnownNetworkScan(networkCount);
        } else {
            Serial.println("[WIFI] Automaticky scan znamych siti selhal, zopakuji pozdeji.");
            WiFi.scanDelete();
            _nextKnownNetworkScan = now + KnownNetworkScanIntervalMs;
        }
        return;
    }

    if (static_cast<long>(now - _nextKnownNetworkScan) >= 0) {
        startKnownNetworkScan();
    }
}

void WifiManager::startKnownNetworkScan() {
    if (!_knownNetworkLookupCallback) {
        _nextKnownNetworkScan = millis() + KnownNetworkScanIntervalMs;
        return;
    }

    WiFi.scanDelete();
    Serial.println("[WIFI] AP fallback: hledam dostupne zname site...");
    const int16_t result = WiFi.scanNetworks(true, true);
    if (result == -1) { // WIFI_SCAN_RUNNING
        _autoScanRunning = true;
        return;
    }

    if (result >= 0) {
        processKnownNetworkScan(result);
        return;
    }

    Serial.println("[WIFI] Automaticky scan se nepodarilo spustit.");
    _nextKnownNetworkScan = millis() + KnownNetworkScanIntervalMs;
}

void WifiManager::processKnownNetworkScan(int16_t networkCount) {
    _autoJoinCandidateCount = 0;
    _autoJoinCandidateIndex = 0;

    for (int16_t index = 0; index < networkCount; ++index) {
        const String ssid = WiFi.SSID(index);
        if (ssid.isEmpty()) continue;

        String password;
        if (!_knownNetworkLookupCallback || !_knownNetworkLookupCallback(ssid, password)) continue;

        const int32_t rssi = WiFi.RSSI(index);
        bool duplicate = false;
        for (uint8_t candidateIndex = 0; candidateIndex < _autoJoinCandidateCount; ++candidateIndex) {
            if (_autoJoinCandidates[candidateIndex].ssid != ssid) continue;
            duplicate = true;
            if (rssi > _autoJoinCandidates[candidateIndex].rssi) {
                _autoJoinCandidates[candidateIndex].rssi = rssi;
                _autoJoinCandidates[candidateIndex].password = password;
            }
            break;
        }

        if (!duplicate && _autoJoinCandidateCount < MaxAutoJoinCandidates) {
            auto& candidate = _autoJoinCandidates[_autoJoinCandidateCount++];
            candidate.ssid = ssid;
            candidate.password = password;
            candidate.rssi = rssi;
        }
    }

    WiFi.scanDelete();

    // Nejsilnejsi znama sit se zkusi jako prvni.
    for (uint8_t i = 0; i < _autoJoinCandidateCount; ++i) {
        for (uint8_t j = i + 1; j < _autoJoinCandidateCount; ++j) {
            if (_autoJoinCandidates[j].rssi <= _autoJoinCandidates[i].rssi) continue;
            const AutoJoinCandidate swap = _autoJoinCandidates[i];
            _autoJoinCandidates[i] = _autoJoinCandidates[j];
            _autoJoinCandidates[j] = swap;
        }
    }

    if (_autoJoinCandidateCount == 0) {
        Serial.println("[WIFI] V okoli neni zadna znama Wi-Fi. AP zustava aktivni.");
        _nextKnownNetworkScan = millis() + KnownNetworkScanIntervalMs;
        return;
    }

    Serial.printf("[WIFI] Nalezeno %u znamych siti, zkousim je podle sily signalu.\n",
                  static_cast<unsigned>(_autoJoinCandidateCount));
    tryNextKnownNetwork();
}

void WifiManager::tryNextKnownNetwork() {
    if (_autoJoinCandidateIndex >= _autoJoinCandidateCount) {
        Serial.println("[WIFI] Zadna nalezena znama sit se nepripojila. AP zustava aktivni.");
        _autoJoinInProgress = false;
        _nextKnownNetworkScan = millis() + KnownNetworkScanIntervalMs;
        return;
    }

    const auto& candidate = _autoJoinCandidates[_autoJoinCandidateIndex];
    _ssid = candidate.ssid;
    _password = candidate.password;

    WiFi.disconnect(false);
    WiFi.setHostname(_hostname.c_str());
    WiFi.setAutoReconnect(false);
    Serial.printf("[WIFI] AP fallback: zkousim znamou sit '%s' (%ld dBm), AP zustava behem pokusu dostupny.\n",
                  _ssid.c_str(), static_cast<long>(candidate.rssi));
    WiFi.begin(_ssid.c_str(), _password.c_str());
    _autoJoinStarted = millis();
    _autoJoinInProgress = true;
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

    // Pokud prave bezi automaticky scan, kratce vyuzijeme jeho vysledek.
    // Nezaciname druhy scan soubezne, protoze ESP32 ma jen jedno Wi-Fi radio.
    int16_t networkCount = -1;
    if (_autoScanRunning) {
        const unsigned long started = millis();
        while ((networkCount = WiFi.scanComplete()) == -1 && millis() - started < 2500) {
            delay(25);
        }
        _autoScanRunning = false;
    } else if (_autoJoinInProgress) {
        return "[]";
    } else {
        networkCount = WiFi.scanNetworks(false, true);
    }

    if (networkCount < 0) {
        WiFi.scanDelete();
        _nextKnownNetworkScan = millis() + KnownNetworkScanIntervalMs;
        return "[]";
    }

    String response = "[";
    for (int16_t index = 0; index < networkCount; ++index) {
        if (index > 0) response += ",";
        response += "{\"ssid\":\"";
        String ssid = WiFi.SSID(index);
        for (size_t character = 0; character < ssid.length(); ++character) {
            const char value = ssid[character];
            if (value == '\\' || value == '"') response += '\\';
            response += value;
        }
        response += "\",\"rssi\":" + String(WiFi.RSSI(index));
        response += ",\"secure\":" + String(WiFi.encryptionType(index) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    response += "]";
    WiFi.scanDelete();

    if (_configAccessPoint) {
        _nextKnownNetworkScan = millis() + KnownNetworkScanIntervalMs;
    }
    return response;
}

int8_t WifiManager::getRssi() const {
    return isConnected() ? WiFi.RSSI() : 0;
}

String WifiManager::getIpAddress() const {
    if (_configAccessPoint) return WiFi.softAPIP().toString();
    return isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
}

void WifiManager::startConfigAccessPoint(uint32_t autoScanDelayMs) {
    _configAccessPoint = true;
    _connected = false;
    _autoScanRunning = false;
    _autoJoinInProgress = false;
    _autoJoinCandidateCount = 0;
    _autoJoinCandidateIndex = 0;
    _staFailureStarted = 0;

    MDNS.end();
    WiFi.scanDelete();
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.setHostname(_hostname.c_str());
    WiFi.persistent(false);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_ps(WIFI_PS_NONE);

    WiFi.softAPdisconnect(true);
    WiFi.softAP("Dashboard-Setup", "dashboard");
    _nextKnownNetworkScan = millis() + autoScanDelayMs;

    Serial.printf("[WIFI] Konfiguracni AP+STA: Dashboard-Setup | heslo: dashboard | IP: %s\n",
                  WiFi.softAPIP().toString().c_str());
    Serial.printf("[WIFI] Zname site budu hledat %s, dale kazdych %lu s.\n",
                  autoScanDelayMs == 0 ? "ihned" : "po odkladu",
                  KnownNetworkScanIntervalMs / 1000UL);
}
