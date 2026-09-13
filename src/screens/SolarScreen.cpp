#include "SolarScreen.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

void SolarScreen::render(IDisplay& display, const DataModel& dm) {
    // Header
    display.fillRect(0, 0, 800, 48, 0);
    display.setTextColor(1);
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(20, 33);
    display.print("FVE & AZ ROUTER MONITOR");

    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(680, 35);
    display.print(dm.system.timeStr);

    display.setTextColor(0);

    // Box 1: Panely / Výroba FVE (vlevo nahoře)
    display.drawRoundRect(15, 65, 245, 165, 6, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(25, 90);
    display.print("SOLAR VYROBA");
    display.drawLine(25, 98, 245, 98, 0);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(30, 145);
    display.printf("%.0f W", dm.solar.productionPowerW);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(30, 185);
    display.printf("Dnes: %.1f kWh", dm.solar.energyTodayKWh);
    display.setCursor(30, 210);
    display.printf("Status: %s", dm.solar.status.available ? "Online" : "Nedostupne");

    // Box 2: Baterie (uprostřed nahoře)
    display.drawRoundRect(275, 65, 245, 165, 6, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(285, 90);
    display.print("BATERIE");
    display.drawLine(285, 98, 505, 98, 0);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(290, 145);
    display.printf("%.0f %%", dm.solar.batterySocPercent);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(290, 185);
    display.printf("Tok: %+.0f W", dm.solar.batteryPowerW);
    display.setCursor(290, 210);
    display.printf("Stav: %s", dm.solar.batteryPowerW < 0 ? "Nabijeni" : (dm.solar.batteryPowerW > 0 ? "Vybijeni" : "Klid"));

    // Box 3: Distribuce / Síť (vpravo nahoře)
    display.drawRoundRect(535, 65, 250, 165, 6, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(545, 90);
    display.print("DISTRIBUCE (SIT)");
    display.drawLine(545, 98, 770, 98, 0);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(550, 145);
    if (dm.solar.gridPowerW >= 0) {
        display.printf("+%.0f W", dm.solar.gridPowerW);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(550, 185);
        display.print("Pretok do site");
    } else {
        display.printf("-%.0f W", -dm.solar.gridPowerW);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(550, 185);
        display.print("Nakup ze site");
    }

    // Box 4: Spotřeba domu (vlevo dole)
    display.drawRoundRect(15, 245, 245, 165, 6, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(25, 270);
    display.print("SPOTREBA DOMU");
    display.drawLine(25, 278, 245, 278, 0);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(30, 325);
    display.printf("%.0f W", dm.solar.houseConsumptionW);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(30, 365);
    display.printf("Okamzity prikon");

    // Box 5: AZ Router / Bojler (uprostřed dole)
    display.drawRoundRect(275, 245, 245, 165, 6, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(285, 270);
    display.print("AZ ROUTER (BOJLER)");
    display.drawLine(285, 278, 505, 278, 0);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(290, 325);
    display.printf("%.0f W", dm.azrouter.routedPowerW);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(290, 365);
    display.printf("Bojler: %.1f C", dm.azrouter.boilerTempC);
    display.setCursor(290, 390);
    display.printf("Vytezeno dnes: %.1f kWh", dm.azrouter.routedEnergyTodayKWh);

    // Box 6: Celková bilance (vpravo dole)
    display.drawRoundRect(535, 245, 250, 165, 6, 0);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(545, 270);
    display.print("ENERGETICKA BILANCE");
    display.drawLine(545, 278, 770, 278, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(550, 310);
    display.printf("Samospotreba: OK");
    display.setCursor(550, 340);
    display.printf("GoodWe UDP: %s", dm.solar.status.available ? "OK (8899)" : "Offline");
    display.setCursor(550, 370);
    display.printf("AZRouter HTTP: %s", dm.azrouter.status.available ? "OK (80)" : "Offline");

    // Footer
    display.drawLine(0, 430, 800, 430, 0);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 460);
    display.printf("GoodWe GW10K-ET + AZ Router Smart Control");
}
