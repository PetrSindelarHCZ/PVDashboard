# Specifikace PVDashboardu

## Účel

PVDashboard je samostatné lokální zařízení pro přehled energetických toků domu.
Telefon nebo počítač slouží k ovládání a konfiguraci, nikoliv jako zdroj dat.
Zařízení musí po startu fungovat bez připojeného vývojového počítače a výpadek
jednoho datového zdroje nesmí zastavit WebUI ani ostatní funkce.

## Hardware

- řídicí deska: Waveshare e-Paper ESP32 Driver Board, ESP32-WROOM-32,
- displej: černobílý 7,5" e-paper, 800 × 480 px,
- označení panelu: **DEPG0750BNU790F30HP**,
- označení FPC: **FPC-8612**,
- driver board: přepínač **B**, druhý přepínač **ON**,
- firmware používá driver **GxEPD2_750_T7**,
- GoodWe: GW10K-ET,
- AZRouter: HTTP REST API.

Podrobnosti a ověřená refresh strategie jsou v [DISPLAY.md](DISPLAY.md).

## Funkční rozsah

### Energetika

Dashboard načítá a zobrazuje:

- aktuální výkon FVE,
- dnešní výrobu,
- spotřebu domu,
- tok do nebo ze sítě,
- stav a výkon baterie,
- výkon přesměrovaný AZRouterem,
- dnešní vytěženou energii,
- teplotu bojleru, pokud ji AZRouter poskytne,
- dostupnost a stáří dat obou zdrojů.

### Obrazovky

| ID | Stav | Obsah |
| --- | --- | --- |
| home | funkční | Souhrn domu, energie a demonstračních čidel |
| solar | funkční | GoodWe, baterie, síť a AZRouter |
| pool | demonstrační | Připravený layout, hodnoty nejsou z reálných čidel |
| weather | demonstrační | Připravený layout, předpověď je pevně zadaná |
| diagnostics | funkční | Firmware, uptime, heap, Wi-Fi a integrace |

Všechny obrazovky používají společnou typografii, černé záhlaví a zápatí,
datum a čas v záhlaví a jednotný vzhled karet.

### WebUI

WebUI musí umožnit:

- zobrazit stav zařízení a stáří dat,
- přepnout obrazovku,
- vyvolat částečný nebo plný refresh,
- změnit Wi-Fi a konfiguraci zdrojů,
- restartovat zařízení,
- provést ruční nebo GitHub OTA aktualizaci.

Přijetí příkazu a fyzické dokončení refreshu jsou nyní dvě různé události,
ale WebUI jejich stav zatím samostatně nezobrazuje.

## Datové zdroje

### GoodWe

- transport: UDP,
- výchozí port: 8899,
- protokol: Modbus RTU rámec přes UDP,
- unit ID: 0xF7,
- dotaz: funkce 0x03, registry od 35100, počet 125,
- validace: délka, unit ID, funkce a CRC16.

Znaménka používaná v aktuálním modelu:

- gridPowerW: kladné = přetok, záporné = odběr,
- batteryPowerW: podle parseru kladné = vybíjení, záporné = nabíjení.

### AZRouter

- transport: HTTP,
- výchozí port: 8081,
- /api/v1/power: výkon, energie a tok sítě,
- /api/v1/status: systémová teplota jako záložní hodnota,
- /api/v1/devices: teplota připojeného zařízení nebo bojleru.

Úspěch celého čtení se nyní řídí odpovědí /api/v1/power. Zbývající dva
dotazy mohou selhat, aniž by byl zdroj označen jako nedostupný.

## Konfigurace

Konfigurace se ukládá do ESP32 NVS přes Preferences, namespace **dashboard**.
Ukládají se Wi-Fi údaje a nastavení obou datových zdrojů. Po uložení se zařízení
restartuje. schemaVersion je nyní pouze hodnota v paměti a není uložená ani
migrovaná.

DisplayConfig.fullRefreshIntervalMinutes je definované, ale současná refresh
politika ho zatím nepoužívá.

## Provozní požadavky

- hesla a tokeny se nesmí objevit v běžném API ani logu,
- nedostupný zdroj nesmí vytvářet souvislou smyčku opakovaných pokusů,
- WebUI má odpovídat i během pomalého refreshu,
- na panelu se nesmí dlouhodobě hromadit ghosting,
- změna celé obrazovky používá čisticí plnou obnovu,
- firmware musí zůstat menší než jedna OTA partition,
- neúspěšná OTA aktualizace nesmí poškodit běžící firmware.

## Kritéria nejbližší stabilní verze

- oba zdroje lze samostatně vypnout a jejich výpadek nezablokuje WebUI,
- běžná odpověď /api/status má při výpadku zdroje zůstat pod 500 ms,
- přepnutí obrazovky zanechá čistý obraz bez duchů,
- WebUI jednoznačně ukáže probíhající refresh,
- konfigurace přežije restart,
- OTA je ověřené platným i poškozeným souborem,
- firmware projde alespoň 24hodinovým testem bez restartu a významného úbytku heapu.