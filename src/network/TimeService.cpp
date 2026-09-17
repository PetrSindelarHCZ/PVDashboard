#include "TimeService.h"
#include "CzechNamedays.h"
#include <esp_sntp.h>
#include <atomic>

namespace {
// SNTP invokes this callback from the network task. Do not touch Strings there.
std::atomic<bool> receivedNtpTime{false};
void onNtpSync(struct timeval* tv) {
    if (tv != nullptr && tv->tv_sec >= 1609459200) { // 2021-01-01 UTC
        receivedNtpTime.store(true);
    }
}
}

TimeService::TimeService() : _synced(false) {
}

void TimeService::begin(const String& timezone, const String& ntpServer) {
    esp_sntp_stop();
    _synced = false;
    receivedNtpTime.store(false);
    sntp_set_time_sync_notification_cb(onNtpSync);
    _timezone = timezone;
    _ntpServer = ntpServer;

    Serial.printf("[NTP] Nastavuji TZ '%s' a NTP server '%s'...\n", _timezone.c_str(), _ntpServer.c_str());
    configTzTime(_timezone.c_str(), _ntpServer.c_str());
}

void TimeService::loop() {
    // A plausible RTC date is not evidence of a successful NTP request.
    if (_synced || !receivedNtpTime.load()) return;
    time_t nowTime;
    time(&nowTime);
    struct tm timeinfo;
    if (localtime_r(&nowTime, &timeinfo) && timeinfo.tm_year > (2020 - 1900)) {
        _synced = true;
        Serial.printf("[NTP] Cas uspesne synchronizovan: %s %s\n",
                      getDateStr().c_str(), getTimeStr().c_str());
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
