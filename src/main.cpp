#include <Arduino.h>
#include "app/DashboardApp.h"

// Keep the reduced loopTask stack while isolating the pre-setup reset.
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[BOOT-LINK] DashboardApp header linked; setup OK");
}

void loop() {
    Serial.println("[BOOT-LINK] loop OK");
    delay(1000);
}
