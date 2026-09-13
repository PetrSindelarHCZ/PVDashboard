#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <functional>

class WifiManager {
public:
    using WifiStatusCallback = std::function<void(bool connected, const String& ip)>;

    WifiManager();

    void begin(const String& ssid, const String& password, const String& hostname);
    bool waitForConnection(uint32_t timeoutMs = 8000);
    void loop();
    bool isConnected() const;
    bool isConfigAccessPoint() const;
    String scanNetworksJson();
    int8_t getRssi() const;
    String getIpAddress() const;
    void onStatusChange(WifiStatusCallback callback);

private:
    String _ssid;
    String _password;
    String _hostname;
    unsigned long _lastReconnectAttempt = 0;
    unsigned long _lastStatusLog = 0;
    bool _connected = false;
    bool _configAccessPoint = false;
    WifiStatusCallback _statusCallback;

    const char* wlStatusToString(wl_status_t status);
    void startConfigAccessPoint();
};
