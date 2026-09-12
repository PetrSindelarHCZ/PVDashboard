#include "PoolScreen.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

void PoolScreen::render(IDisplay& display, const DataModel& dm) {
    // Header
    display.fillRect(0, 0, 800, 48, 1);
    display.setTextColor(0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 32);
    display.print("BAZEN - AKTUALNI STAV");

    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(680, 35);
    display.print(dm.system.timeStr);

    display.drawLine(0, 48, 800, 48, 0);

    // 4 Hlavní ukazatele (Karty nahoře)
    // 1. Teplota vody
    display.drawRoundRect(15, 60, 180, 120, 4, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(25, 85);
    display.print("Teplota vody");
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(25, 130);
    display.printf("%.1f C", dm.pool.waterTempC);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(25, 160);
    display.printf("Cil: %.1f C", dm.pool.targetTempC);

    // 2. pH
    display.drawRoundRect(210, 60, 180, 120, 4, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(220, 85);
    display.print("pH vody");
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(220, 130);
    display.printf("%.1f", dm.pool.ph);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(220, 160);
    display.print("Ideal: 7.0-7.4");

    // 3. Chlor
    display.drawRoundRect(405, 60, 180, 120, 4, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(415, 85);
    display.print("Volny chlor");
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(415, 130);
    display.printf("%.1f mg/l", dm.pool.freeChlorineMgL);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(415, 160);
    display.print("Ideal: 0.3-1.0");

    // 4. Vzduch
    display.drawRoundRect(600, 60, 185, 120, 4, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(610, 85);
    display.print("Teplota vzduchu");
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(610, 130);
    display.printf("%.1f C", dm.pool.airTempC);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(610, 160);
    display.printf("Vlhkost: %d %%", dm.pool.airHumidityPercent);

    // Sekce technologie (spodní polovina)
    display.drawRoundRect(15, 200, 480, 210, 4, 0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(25, 230);
    display.print("Stav technologie");
    display.drawLine(25, 238, 480, 238, 0);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(30, 275);
    display.print("Filtrace:");
    display.setCursor(200, 275);
    display.print(dm.pool.filtrationRunning ? "Bezi (automatika)" : "Vypnuto");

    display.setCursor(30, 315);
    display.print("Ohrev FVE:");
    display.setCursor(200, 315);
    display.print(dm.pool.heatingActive ? "Aktivni" : "Vypnuto");

    display.setCursor(30, 355);
    display.print("Davkovani pH / Cl:");
    display.setCursor(200, 355);
    display.print("OK");

    display.setCursor(30, 395);
    display.print("Osvetleni / UV:");
    display.setCursor(200, 395);
    display.print("Vypnuto");

    // Pravý info panel
    display.drawRoundRect(510, 200, 275, 210, 4, 0);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(525, 230);
    display.print("Informace");
    display.drawLine(525, 238, 770, 238, 0);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(525, 275);
    display.print("Objem: 32 m3");
    display.setCursor(525, 315);
    display.print("Posledni udrzba: 6.9.");
    display.setCursor(525, 355);
    display.print("Status: Vse v poradku");

    // Footer
    display.drawLine(0, 430, 800, 430, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 460);
    display.print("Modul Bazen - Placeholder pripraven pro senzory");
}
