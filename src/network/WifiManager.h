#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <functional>

class WifiManager {
public:
    using WifiStatusCallback = std::function<void(bool connected, const String& ip)>;
    using KnownNetworkLookupCallback = std::function<bool(const String& ssid, String& password)>;
    using KnownNetworkAtCallback = std::function<bool(size_t enabledIndex, String& ssid, String& password)>;
    using AutoNetworkSelectedCallback = std::function<void(const String& ssid, const String& password)>;

    WifiManager();

    void begin(const String& ssid, const String& password, const String& hostname);
    bool waitForConnection(uint32_t timeoutMs = 8000);
    void disconnectToConfigAccessPoint();
    void loop();
    bool isConnected() const;
    bool isConfigAccessPoint() const;
    String scanNetworksJson();
    int8_t getRssi() const;
    String getIpAddress() const;
    String getSsid() const { return _ssid; }
    void onStatusChange(WifiStatusCallback callback);
    void onKnownNetworkLookup(KnownNetworkLookupCallback callback);
    void onKnownNetworkAt(KnownNetworkAtCallback callback);
    void onAutoNetworkSelected(AutoNetworkSelectedCallback callback);

private:
    struct AutoJoinCandidate {
        String ssid;
        String password;
        int32_t rssi = -127;
    };

    static constexpr uint8_t MaxAutoJoinCandidates = 8;
    static constexpr uint32_t ReconnectIntervalMs = 10000;
    static constexpr uint32_t StaFallbackTimeoutMs = 30000;
    static constexpr uint32_t AutoJoinTimeoutMs = 8000;
    static constexpr uint32_t KnownNetworkScanIntervalMs = 30000;

    String _ssid;
    String _password;
    String _hostname;
    unsigned long _lastReconnectAttempt = 0;
    unsigned long _staFailureStarted = 0;
    unsigned long _lastStatusLog = 0;
    bool _connected = false;
    bool _configAccessPoint = false;
    bool _eventsRegistered = false;
    WifiStatusCallback _statusCallback;
    KnownNetworkLookupCallback _knownNetworkLookupCallback;
    KnownNetworkAtCallback _knownNetworkAtCallback;
    AutoNetworkSelectedCallback _autoNetworkSelectedCallback;

    AutoJoinCandidate _autoJoinCandidates[MaxAutoJoinCandidates];
    uint8_t _autoJoinCandidateCount = 0;
    uint8_t _autoJoinCandidateIndex = 0;
    bool _autoScanRunning = false;
    bool _autoJoinInProgress = false;
    unsigned long _autoJoinStarted = 0;
    unsigned long _nextKnownNetworkScan = 0;

    const char* wlStatusToString(wl_status_t status);
    void startConfigAccessPoint(uint32_t autoScanDelayMs = 0);
    void serviceConfigAccessPoint();
    void startKnownNetworkScan();
    void processKnownNetworkScan(int16_t networkCount);
    void appendUnseenKnownNetworks();
    void tryNextKnownNetwork();
    void completeConnection(bool selectedFromFallback);
};
