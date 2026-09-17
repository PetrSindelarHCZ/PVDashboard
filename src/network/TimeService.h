#pragma once
#include <Arduino.h>
#include <time.h>

class TimeService {
public:
    TimeService();

    void begin(const String& timezone, const String& ntpServer);
    void loop();
    bool isSynced() const;
    String getTimeStr();    // HH:MM bez sekund
    String getDateStr();    // d. m. YYYY
    String getDayOfWeekStr();
    static String getNtpStatusJson(bool wifiConnected);

private:
    void startNtpSync(bool preserveValidTime, const char* reason);

    String _timezone;
    String _ntpServer;
    bool _synced = false;
    bool _networkAvailable = false;
    unsigned long _configuredAtMs = 0;
    unsigned long _lastSyncAttemptMs = 0;
    unsigned long _lastSyncMs = 0;
    time_t _lastSyncEpoch = 0;
};
