#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "app/DashboardApp.h"
#include <new>

// Temporary 8 KiB loopTask: 16 KiB currently causes a pre-setup SW_RESET on the target ESP32.\n// Restore the larger stack only after the startup memory/layout issue is resolved.
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

DashboardApp* app = nullptr;

void setup() {
    // Temporary workaround: Wi-Fi startup can trigger a supply brownout on the
    // current USB-powered hardware. Hardware verification is tracked in ROADMAP.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // DashboardApp is intentionally allocated only after Arduino has created
    // loopTask. Keeping the complete application object in global .bss can
    // leave too little contiguous internal DRAM for the 16 KiB loopTask stack.
    app = new (std::nothrow) DashboardApp();
    if (app == nullptr) {
        Serial.begin(115200);
        delay(100);
        Serial.println("[BOOT] CHYBA: DashboardApp nelze alokovat na heapu.");
        return;
    }

    app->setup();
}

void loop() {
    if (app != nullptr)
        app->loop();
    else
        delay(1000);
}
