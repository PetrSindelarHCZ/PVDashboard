#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "app/DashboardApp.h"
#include <new>

// GitHub HTTPS checks and OTA downloads run on loopTask and need stack headroom.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

DashboardApp* app = nullptr;

void setup() {
    // Potlačení soft/hard resetu způsobeného krátkodobým poklesem napětí (Brownout) při startu Wi-Fi
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // DashboardApp is intentionally allocated only after Arduino has created
    // loopTask. Keeping the complete application object in global .bss can
    // leave too little contiguous internal DRAM for the 16 KiB loopTask stack,
    // causing a SW_RESET before setup() is ever entered.
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
