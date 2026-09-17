#include "TimeService.h"
#include "CzechNamedays.h"
#include <esp_sntp.h>
#include <ArduinoJson.h>
#include <atomic>

namespace {
// SNTP invokes this callback from the network task. Do not touch Strings there.
std::atomic<uint32_t> receivedNtpEpoch{0};
TimeService* activeTimeService = nullptr;

void onNtpSync(struct timeval* tv) {
    if (tv != nullptr && tv->tv_sec >= 1609459200) { // 2021-01-01 UTC
        receivedNtpEpoch.store(static_cast<uint32_t>(tv->tv_sec));
    }
}
}

TimeService::TimeService() : _synced(false) {
    activeTimeService = this;
}

void TimeService::begin(const String& timezone, const String& ntpServer) {
    esp_sntp_stop();
    _synced = false;
    _configuredAtMs = millis();
    _lastSyncMs = 0;
    _lastSyncEpoch = 0;
    receivedNtpEpoch.store(0);
    sntp_set_time_sync_notification_cb(onNtpSync);
    _timezone = timezone;
    _ntpServer = ntpServer;
    activeTimeService = this;

    Serial.printf("[NTP] Nastavuji TZ '%s' a NTP server '%s'...\n", _timezone.c_str(), _ntpServer.c_str());
    configTzTime(_timezone.c_str(), _ntpServer.c_str());
}

void TimeService::loop() {
    const uint32_t receivedEpoch = receivedNtpEpoch.exchange(0);
    if (receivedEpoch == 0) return;

    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && timeinfo.tm_year > (2020 - 1900)) {
        _synced = true;
        _lastSyncEpoch = static_cast<time_t>(receivedEpoch);
        _lastSyncMs = millis();
        Serial.printf("[NTP] Cas uspesne synchronizovan: %s %s | server: %s\n",
                      getDateStr().c_str(), getTimeStr().c_str(), _ntpServer.c_str());
    }
}

bool TimeService::isSynced() const {
    return _synced;
}

String TimeService::getNtpStatusJson(bool wifiConnected) {
    JsonDocument doc;
    TimeService* service = activeTimeService;
    if (service == nullptr) {
        doc["server"] = "";
        doc["state"] = "unavailable";
        doc["synced"] = false;
        doc["lastSyncEpoch"] = 0;
        doc["lastSyncAgeSeconds"] = nullptr;
        doc["configuredAgeSeconds"] = 0;
    } else {
        const unsigned long now = millis();
        const unsigned long configuredAgeSeconds = (now - service->_configuredAtMs) / 1000UL;
        const char* state = "unavailable";
        if (!wifiConnected) {
            state = "offline";
        } else if (service->_synced) {
            state = "ok";
        } else if (configuredAgeSeconds < 20UL) {
            state = "syncing";
        }

        doc["server"] = service->_ntpServer;
        doc["state"] = state;
        doc["synced"] = service->_synced;
        doc["lastSyncEpoch"] = static_cast<uint32_t>(service->_lastSyncEpoch);
        if (service->_lastSyncMs == 0) {
            doc["lastSyncAgeSeconds"] = nullptr;
        } else {
            doc["lastSyncAgeSeconds"] = (now - service->_lastSyncMs) / 1000UL;
        }
        doc["configuredAgeSeconds"] = configuredAgeSeconds;
    }

    String response;
    serializeJson(doc, response);
    return response;
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
    return "";
}

String TimeService::getDateStr() {
    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && _synced) {
        static const char* dayAbbreviations[] = {
            "Ne", "Po", "Út", "St", "Čt", "Pá", "So"
        };

        const uint8_t day = static_cast<uint8_t>(timeinfo.tm_mday);
        const uint8_t month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
        const char* nameday = CzechNamedays::get(day, month);

        char buf[96];
        if (nameday != nullptr && nameday[0] != '\0') {
            snprintf(buf, sizeof(buf), "%s %d.%d.%04d | %s",
                     dayAbbreviations[timeinfo.tm_wday % 7],
                     timeinfo.tm_mday,
                     timeinfo.tm_mon + 1,
                     timeinfo.tm_year + 1900,
                     nameday);
        } else {
            snprintf(buf, sizeof(buf), "%s %d.%d.%04d",
                     dayAbbreviations[timeinfo.tm_wday % 7],
                     timeinfo.tm_mday,
                     timeinfo.tm_mon + 1,
                     timeinfo.tm_year + 1900);
        }
        return String(buf);
    }
    return "";
}

String TimeService::getDayOfWeekStr() {
    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && _synced) {
        const char* days[] = {"Neděle", "Pondělí", "Úterý", "Středa", "Čtvrtek", "Pátek", "Sobota"};
        return String(days[timeinfo.tm_wday % 7]);
    }
    return "";
}
