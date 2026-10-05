#include <Arduino.h>
#include "app/DashboardApp.h"
#include <new>

// Keep the reduced loopTask stack while isolating the startup reset.
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

DashboardApp* app = nullptr;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[BOOT-ALLOC] setup entered");

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
    Serial.println("[BOOT-ALLOC] loop OK");
    delay(1000);
}
