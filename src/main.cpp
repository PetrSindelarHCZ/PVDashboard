#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "app/DashboardApp.h"
#include <new>

// Keep the reduced loopTask stack while isolating the startup reset.
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

DashboardApp* app = nullptr;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[BOOT-ALLOC] setup entered");

    // Diagnostic only: verify whether Wi-Fi startup is causing a supply brownout.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.println("[BOOT-POWER] brownout detector disabled for diagnostic test");

    Serial.println("[BOOT-ALLOC] before DashboardApp allocation");
    app = new (std::nothrow) DashboardApp();
    Serial.println("[BOOT-ALLOC] after DashboardApp allocation");

    if (app == nullptr) {
        Serial.println("[BOOT-ALLOC] DashboardApp allocation FAILED");
        return;
    }

    Serial.println("[BOOT-ALLOC] DashboardApp allocation OK");
    Serial.println("[BOOT-SETUP] before DashboardApp::setup");
    app->setup();
    Serial.println("[BOOT-SETUP] after DashboardApp::setup");
}

void loop() {
    if (app != nullptr)
        app->loop();
    else
        delay(1000);
}
