# Integrace GoodWe a AZRouteru

Tento dokument popisuje datové kontrakty aktuálního firmware. Podrobné mapování
je přímo v implementaci klientů; při změně protokolu se musí současně upravit
DataModel a tato dokumentace.

## GoodWe GW10K-ET

- transport: UDP, standardně port 8899,
- unit ID: 0xF7,
- Modbus funkce: 0x03,
- první registr: 35100,
- počet registrů: 125,
- request: F7 03 89 1C 00 7D 7A E7,
- odpověď může mít prefix AA 55,
- CRC se počítá nad vlastním RTU rámcem.

Mapované hodnoty:

| DataModel | Zdroj |
| --- | --- |
| productionPowerW | Součet ppv1 a ppv2 |
| energyTodayKWh | e_day, registr 35193, měřítko 0,1 kWh |
| batteryPowerW | pbattery1, registr 35182 |
| batterySocPercent | Zatím odhad z napětí baterie |
| gridPowerW | Active Power Meter, registr 35140 |
| houseConsumptionW | Výpočet z toků, případně load_ptotal |

Klient provede až tři pokusy, každý může čekat 1,2 sekundy. Při nedostupném
zařízení proto blokuje hlavní smyčku přibližně 3,6 sekundy.

## AZRouter

- transport: HTTP bez autentizace,
- standardní port v konfiguraci: 8081,
- /api/v1/power:
  - output.power, ID 3 nebo součet fází 0 až 2,
  - output.energy, ID 4,
  - input.power, součet fází 0 až 2,
- /api/v1/status:
  - system.temperature jako záložní teplota,
- /api/v1/devices:
  - power.temperature jako preferovaná teplota bojleru nebo zařízení.

Za úspěch celé aktualizace se považuje platná odpověď /api/v1/power. Klient
nyní provádí všechny tři dotazy i tehdy, když první selže. setTimeout(1500)
omezuje čtení, ale connect timeout zatím není nastavený samostatně.

## Znaménka

V komentářích DataModel a v některých obrazovkách historicky existovaly opačné
popisy směru toku. Za referenční se nyní považuje chování parseru a obrazovek:

- GoodWe gridPowerW: kladné číslo se zobrazuje jako přetok, záporné jako odběr,
- GoodWe batteryPowerW: kladné číslo se zobrazuje jako vybíjení, záporné jako nabíjení.

Před prvním stabilním release je vhodné tato znaménka ověřit proti reálnému
stavu měniče a opravit zavádějící komentáře v DataModel.

## Stav a stáří dat

Každý zdroj udržuje:

- available,
- lastSuccessMs,
- lastAttemptMs,
- errorCount,
- lastError.

Při chybě zůstávají poslední naměřené hodnoty v DataModel. UI proto musí spolu
s hodnotou zobrazovat dostupnost nebo stáří dat, aby stará hodnota nepůsobila
jako aktuální měření.