#include "TimeService.h"

TimeService::TimeService() : _synced(false) {
}

void TimeService::begin(const String& timezone, const String& ntpServer) {
    _timezone = timezone;
    _ntpServer = ntpServer;

    Serial.printf("[NTP] Nastavuji TZ '%s' a NTP server '%s'...\n", _timezone.c_str(), _ntpServer.c_str());
    configTzTime(_timezone.c_str(), _ntpServer.c_str());
}

void TimeService::loop() {
    unsigned long now = millis();
    if (now - _lastSyncCheck > 5000) {
        _lastSyncCheck = now;
        
        time_t nowTime;
        time(&nowTime);
        struct tm timeinfo;
        if (localtime_r(&nowTime, &timeinfo) && timeinfo.tm_year > (2020 - 1900)) {
            if (!_synced) {
                _synced = true;
                Serial.printf("[NTP] Cas uspesne synchronizovan: %s %s\n", 
                              getDateStr().c_str(), 
                              getTimeStr().c_str());
            }
        }
    }
}

bool TimeService::isSynced() const {
    return _synced;
}

String TimeService::getTimeStr() {
    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && _synced) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        return String(buf);
    }
    return "--:--";
}

String TimeService::getDateStr() {
    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && _synced) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d. %d. %d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
        return String(buf);
    }
    return "--.--.----";
}

String TimeService::getDayOfWeekStr() {
    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && _synced) {
        const char* days[] = {"Nedele", "Pondeli", "Utery", "Streda", "Ctvrtek", "Patek", "Sobota"};
        return String(days[timeinfo.tm_wday % 7]);
    }
    return "";
}
