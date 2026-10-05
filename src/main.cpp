#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "app/DashboardApp.h"
#include <new>

// Keep the reduced loopTask stack while isolating the startup reset.
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

DashboardApp* app = nullptr;

// Diagnostic A/B: keep DashboardApp::loop() and its dependency tree linked,
// but do not execute it. The volatile runtime condition prevents the compiler
// and linker from proving the call unreachable.
volatile bool runDashboardLoop = false;

void setup() {
    Serial.begin(115200);
    delay(500);

    // Diagnostic only: verify whether Wi-Fi startup is causing a supply brownout.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    app = new (std::nothrow) DashboardApp();

    if (app == nullptr) {
        return;
    }

    app->setup();
}

void loop() {
    if (runDashboardLoop && app != nullptr)
        app->loop();

    delay(1000);
}
