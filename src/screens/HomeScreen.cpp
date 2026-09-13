#include "HomeScreen.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

void HomeScreen::render(IDisplay& display, const DataModel& dm) {
    // 1. Horní záhlaví (Header)
    display.fillRect(0, 0, 800, 48, 1);
    display.setTextColor(0);
    
    // Titulek vlevo
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 32);
    display.print("DOMOV");

    // Datum uprostřed
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(240, 32);
    display.print(dm.system.dateStr.length() > 0 ? dm.system.dateStr : "12. 9. 2026");

    // Wi-Fi stav / IP adresa v záhlaví
    display.setFont(&FreeSans9pt7b);
    display.setCursor(440, 32);
    if (dm.system.wifiConnected) {
        display.printf("WiFi: %s (%d dBm)", dm.system.ipAddress.c_str(), dm.system.wifiRssi);
    } else {
        display.print("WiFi: Nepripojeno");
    }

    // Čas vpravo (bez sekund)
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(700, 35);
    display.print(dm.system.timeStr);

    // Dělící vodorovná linka pod hlavičkou
    display.drawLine(0, 48, 800, 48, 0);

    // ==========================================
    // 3 Hlavní sloupce (VENKU | ENERGIE | UVNITŘ)
    // ==========================================
    // Sloupec 1: x: 0 - 265
    // Sloupec 2: x: 266 - 533
    // Sloupec 3: x: 534 - 800
    display.drawLine(266, 48, 266, 420, 0);
    display.drawLine(534, 48, 534, 420, 0);

    // --- Sloupec 1: VENKU ---
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 80);
    display.print("VENKU");

    // Velká teplota
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(50, 160);
    display.printf("%.1f C", dm.weather.outdoorTempC);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 220);
    display.printf("Vlhkost: %d %%", dm.weather.outdoorHumidityPercent);

    display.drawLine(20, 250, 245, 250, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(20, 280);
    display.print("Dnes");

    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(50, 330);
    display.printf("%.0f / %.0f C", dm.weather.tempMaxTodayC, dm.weather.tempMinTodayC);

    // --- Sloupec 2: ENERGIE ---
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(285, 80);
    display.print("ENERGIE");

    // Výroba FVE
    display.setFont(&FreeSans9pt7b);
    display.setCursor(285, 120);
    display.print("Vyroba FVE:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(285, 145);
    display.printf("%.1f kW", dm.solar.productionPowerW / 1000.0f);

    // Spotřeba domu
    display.setFont(&FreeSans9pt7b);
    display.setCursor(285, 195);
    display.print("Spotreba domu:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(285, 220);
    display.printf("%.1f kW", dm.solar.houseConsumptionW / 1000.0f);

    // Distribuce / Síť (kladné = přetok do sítě, záporné = nákup)
    display.setFont(&FreeSans9pt7b);
    display.setCursor(285, 270);
    display.print("Sit (pretok/odber):");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(285, 295);
    if (dm.solar.gridPowerW >= 0) {
        display.printf("+%.1f kW (pretok)", dm.solar.gridPowerW / 1000.0f);
    } else {
        display.printf("-%.1f kW (nakup)", (-dm.solar.gridPowerW) / 1000.0f);
    }

    // Baterie
    display.setFont(&FreeSans9pt7b);
    display.setCursor(285, 345);
    display.print("Baterie:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(285, 370);
    display.printf("%.0f %% (%.0f W)", dm.solar.batterySocPercent, dm.solar.batteryPowerW);

    // --- Sloupec 3: UVNITŘ ---
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(550, 80);
    display.print("UVNITR");

    display.setFont(&FreeSans9pt7b);
    display.setCursor(550, 120);
    display.print("Obyvak:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(550, 145);
    display.printf("%.1f C", dm.inside.livingRoomTempC);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(550, 195);
    display.print("Loznice:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(550, 220);
    display.printf("%.1f C", dm.inside.bedroomTempC);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(550, 270);
    display.print("CO2 v mistnosti:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(550, 295);
    display.printf("%d ppm", dm.inside.co2Ppm);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(550, 345);
    display.print("Bazen voda:");
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(550, 370);
    display.printf("%.1f C", dm.inside.poolTempC);

    // ==========================================
    // Spodní stavová lišta (Footer)
    // ==========================================
    display.drawLine(0, 420, 800, 420, 0);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 455);
    display.printf("Predpoved: Ct 20 C | Pa 19 C | So 21 C");

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(560, 455);
    display.printf("[OK] %s", dm.system.statusMessage.c_str());
}
