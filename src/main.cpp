#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[BOOT-MIN] setup OK");
}

void loop() {
    Serial.println("[BOOT-MIN] loop OK");
    delay(1000);
}
