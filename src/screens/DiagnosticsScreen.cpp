#include "DiagnosticsScreen.h"
#include "ScreenStyle.h"
#include "../../include/Version.h"

void DiagnosticsScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawHeader(display, "SYSTEMOVA DIAGNOSTIKA", dm);

    ScreenStyle::drawCard(display, 15, 65, 370, 350, "ESP32 A SIT");
    ScreenStyle::useBody(display);
    display.setCursor(30, 135);
    display.printf("Firmware: %s (v%s)", FIRMWARE_NAME, FIRMWARE_VERSION);
    display.setCursor(30, 170);
    display.printf("Build: %s", FIRMWARE_BUILD_DATE);
    display.setCursor(30, 205);
    display.printf("Volna RAM: %lu KB", (unsigned long)(dm.system.freeHeapBytes / 1024));
    display.setCursor(30, 240);
    display.printf("Uptime: %lu s", (unsigned long)dm.system.uptimeSeconds);
    display.setCursor(30, 275);
    display.printf("WiFi: %s", dm.system.wifiConnected ? "Pripojeno" : "Odpojeno");
    display.setCursor(30, 310);
    display.printf("IP: %s", dm.system.ipAddress.c_str());
    display.setCursor(30, 345);
    display.printf("Signal: %d dBm", dm.system.wifiRssi);
    display.setCursor(30, 380);
    display.printf("NTP: %s", dm.system.ntpSynced ? "Synchronizovano" : "Ceka na sync");

    ScreenStyle::drawCard(display, 410, 65, 375, 350, "INTEGRACE A SLUZBY");
    ScreenStyle::useStrongBody(display);
    display.setCursor(425, 135);
    display.print("GOODWE | UDP 8899");
    ScreenStyle::useBody(display);
    display.setCursor(440, 165);
    display.printf("Status: %s", dm.solar.status.available ? "Data dostupna" : "Nedostupne");
    display.setCursor(440, 195);
    display.printf("Chyby: %u", dm.solar.status.errorCount);

    ScreenStyle::useStrongBody(display);
    display.setCursor(425, 235);
    display.print("AZ ROUTER | HTTP");
    ScreenStyle::useBody(display);
    display.setCursor(440, 265);
    display.printf("Status: %s", dm.azrouter.status.available ? "Data dostupna" : "Nedostupne");
    display.setCursor(440, 295);
    display.printf("Chyby: %u", dm.azrouter.status.errorCount);

    ScreenStyle::useStrongBody(display);
    display.setCursor(425, 335);
    display.print("WEB SERVER");
    ScreenStyle::useBody(display);
    display.setCursor(440, 365);
    display.print("REST API / Mobile UI | port 80");
    display.setCursor(440, 395);
    display.printf("Obrazovka: %s", dm.system.currentScreenId.c_str());

    ScreenStyle::drawFooter(display, "Web: http://dashboard.local | http://<IP_adresa>/");
}