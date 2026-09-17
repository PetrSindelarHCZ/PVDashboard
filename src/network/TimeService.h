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

private:
    String _timezone;
    String _ntpServer;
    bool _synced = false;
};
