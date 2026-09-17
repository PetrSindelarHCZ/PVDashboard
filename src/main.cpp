#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "app/DashboardApp.h"

// GitHub HTTPS checks and OTA downloads run on loopTask and need stack headroom.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

DashboardApp app;

void setup() {
    // Potlačení soft/hard resetu způsobeného krátkodobým poklesem napětí (Brownout) při startu Wi-Fi
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    app.setup();
}

void loop() {
    app.loop();
}
