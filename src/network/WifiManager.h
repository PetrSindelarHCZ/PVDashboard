#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WifiManager {
public:
    WifiManager();

    void begin(const String& ssid, const String& password, const String& hostname);
    void loop();
    bool isConnected() const;
    int8_t getRssi() const;
    String getIpAddress() const;

private:
    String _ssid;
    String _password;
    String _hostname;
    unsigned long _lastReconnectAttempt = 0;
    bool _connected = false;
};
