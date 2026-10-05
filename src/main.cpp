#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "app/DashboardApp.h"
#include <new>

// Reduced loopTask stack keeps more internal RAM available for the application.
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

DashboardApp* app = nullptr;

void setup() {
    Serial.begin(115200);
    delay(500);

    // Temporary workaround for the confirmed Wi-Fi startup brownout on the
    // current devboard. Hardware power integrity remains a documented TODO.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    app = new (std::nothrow) DashboardApp();
    if (app == nullptr) {
        Serial.println("[BOOT] CHYBA: DashboardApp nelze alokovat na heapu.");
        return;
    }

    app->setup();
}

void loop() {
    if (app != nullptr) {
        app->loop();
    } else {
        delay(1000);
    }
}
