#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"

void DiagnosticsScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    ScreenStyle::drawCard(display, 75, 63, 342, 402, "ESP32 A SIT");
    ScreenStyle::useBody(display);
    display.setCursor(90, 135);
    display.printf("Firmware: %s (v%s)", FIRMWARE_NAME, FIRMWARE_VERSION);
    display.setCursor(90, 170);
    display.printf("Build: %s", FIRMWARE_BUILD_DATE);
    display.setCursor(90, 205);
    display.printf("Volna RAM: %lu KB", (unsigned long)(dm.system.freeHeapBytes / 1024));
    display.setCursor(90, 240);
    display.printf("Uptime: %lu s", (unsigned long)dm.system.uptimeSeconds);
    display.setCursor(90, 275);
    display.printf("WiFi: %s", dm.system.wifiConnected ? "Pripojeno" : "Odpojeno");
    display.setCursor(90, 310);
    display.printf("IP: %s", dm.system.ipAddress.c_str());
    display.setCursor(90, 345);
    display.printf("Signal: %d dBm", dm.system.wifiRssi);
    display.setCursor(90, 380);
    display.printf("NTP: %s", dm.system.ntpSynced ? "Synchronizovano" : "Ceka na sync");

    ScreenStyle::drawCard(display, 427, 63, 358, 402, "INTEGRACE A SLUZBY");
    ScreenStyle::useStrongBody(display);
    display.setCursor(442, 135);
    display.print("GOODWE | UDP 8899");
    ScreenStyle::useBody(display);
    display.setCursor(457, 165);
    display.printf("Status: %s", dm.solar.status.available ? "Data dostupna" : "Nedostupne");
    display.setCursor(457, 195);
    display.printf("Chyby: %u", dm.solar.status.errorCount);

    ScreenStyle::useStrongBody(display);
    display.setCursor(442, 235);
    display.print("AZ ROUTER | HTTP");
    ScreenStyle::useBody(display);
    display.setCursor(457, 265);
    display.printf("Status: %s", dm.azrouter.status.available ? "Data dostupna" : "Nedostupne");
    display.setCursor(457, 295);
    display.printf("Chyby: %u", dm.azrouter.status.errorCount);

    ScreenStyle::useStrongBody(display);
    display.setCursor(442, 335);
    display.print("WEB SERVER");
    ScreenStyle::useBody(display);
    display.setCursor(457, 365);
    display.print("REST API / Mobile UI | port 80");
    display.setCursor(457, 395);
    display.printf("Obrazovka: %s", dm.system.currentScreenId.c_str());

}
