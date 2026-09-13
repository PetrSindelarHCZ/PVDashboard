#include "DiagnosticsScreen.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "../../include/Version.h"

void DiagnosticsScreen::render(IDisplay& display, const DataModel& dm) {
    display.fillRect(0, 0, 800, 48, 0);
    display.setTextColor(1);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 33);
    display.print("SYSTEMOVA DIAGNOSTIKA");

    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(680, 35);
    display.print(dm.system.timeStr);

    display.setTextColor(0);

    // Levý panel: ESP32 & Síť
    display.drawRoundRect(15, 65, 370, 350, 4, 0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(30, 95);
    display.print("ESP32 & Sitovy stav");
    display.drawLine(30, 103, 360, 103, 0);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(30, 135);
    display.printf("Firmware: %s (v%s)", FIRMWARE_NAME, FIRMWARE_VERSION);

    display.setCursor(30, 170);
    display.printf("Build: %s", FIRMWARE_BUILD_DATE);

    display.setCursor(30, 205);
    display.printf("Volna RAM (Heap): %lu KB", (unsigned long)(dm.system.freeHeapBytes / 1024));

    display.setCursor(30, 240);
    display.printf("Uptime: %lu sekund", (unsigned long)dm.system.uptimeSeconds);

    display.setCursor(30, 275);
    display.printf("WiFi Status: %s", dm.system.wifiConnected ? "Pripojeno" : "Odpojeno");

    display.setCursor(30, 310);
    display.printf("IP adresa: %s", dm.system.ipAddress.c_str());

    display.setCursor(30, 345);
    display.printf("WiFi RSSI: %d dBm", dm.system.wifiRssi);

    display.setCursor(30, 380);
    display.printf("NTP Sync: %s", dm.system.ntpSynced ? "Synchronizovano" : "Ceka na sync");

    // Pravý panel: Datové zdroje (GoodWe, AZRouter, čidla)
    display.drawRoundRect(410, 65, 375, 350, 4, 0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(425, 95);
    display.print("Stav integraci a zdroju");
    display.drawLine(425, 103, 760, 103, 0);

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(425, 135);
    display.print("1. GoodWe Inverter (UDP 8899)");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(440, 160);
    display.printf("Status: %s", dm.solar.status.available ? "OK - Data dostupna" : "Pripraveno k napojeni");
    display.setCursor(440, 185);
    display.printf("Pocet chyb: %u", dm.solar.status.errorCount);
    display.setCursor(440, 210);
    if (dm.solar.status.lastSuccessMs == 0) {
        display.print("Aktualizace: Nikdy");
    } else {
        display.printf("Aktualizace: pred %lu s", (unsigned long)((millis() - dm.solar.status.lastSuccessMs) / 1000));
    }

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(425, 230);
    display.print("2. AZ Router (HTTP REST)");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(440, 255);
    display.printf("Status: %s", dm.azrouter.status.available ? "OK - Data dostupna" : "Pripraveno k napojeni");
    display.setCursor(440, 280);
    display.printf("Pocet chyb: %u", dm.azrouter.status.errorCount);
    display.setCursor(440, 305);
    if (dm.azrouter.status.lastSuccessMs == 0) {
        display.print("Aktualizace: Nikdy");
    } else {
        display.printf("Aktualizace: pred %lu s", (unsigned long)((millis() - dm.azrouter.status.lastSuccessMs) / 1000));
    }

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(425, 325);
    display.print("3. Web Server & Ovladani");
    display.setFont(&FreeSans9pt7b);
    display.setCursor(440, 350);
    display.print("Port: 80 (REST API / Mobile UI)");
    display.setCursor(440, 375);
    display.printf("Aktivni obrazovka: %s", dm.system.currentScreenId.c_str());

    // Footer
    display.drawLine(0, 430, 800, 430, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 460);
    display.print("Web rozhrani dostupne na: http://dashboard.local nebo http://<IP_adresa>/");
}
