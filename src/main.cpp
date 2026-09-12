#include <Arduino.h>
#include "app/DashboardApp.h"

DashboardApp app;

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
