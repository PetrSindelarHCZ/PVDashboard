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
- partition layout: **min_spiffs.csv**, dva OTA app sloty po **1 966 080 B**,
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
| home | funkční | Souhrn domu; karty se skládají podle aktivních modulů, vnitřní čidla jsou zatím demonstrační |
| solar | funkční / dynamická | GoodWe a/nebo AZRouter; obrazovka existuje jen pokud je zapnutý alespoň jeden zdroj |
| pool | UI funkční / data demonstrační | Viditelnost je konfigurovatelná, hodnoty zatím nejsou z reálných čidel |
| weather | funkční / dynamická | Aktuální počasí, více lokalit, čtyřdenní předpověď a hodinové detaily |
| diagnostics | funkční | Firmware, uptime, heap, Wi-Fi, integrace a jejich stav |

Všechny obrazovky používají společnou typografii, černé záhlaví, levý sidebar,
datum a čas v záhlaví a jednotný vzhled karet. Zápatí bylo odstraněné ve prospěch
většího prostoru pro obsah. Sidebar i stavové ikony respektují aktivaci modulů.

### WebUI

WebUI umožňuje:

- zobrazit stav zařízení, dostupnost zdrojů a stáří dat,
- přepnout obrazovku a zobrazit náhled posledního vyrenderovaného e-inku,
- vyvolat částečný nebo plný refresh a sledovat stavy fronty/renderu,
- ovládat stejný `NavigationController` přes virtuální pětisměrný joystick
  nebo klávesnici,
- změnit hostname, NTP server, časové pásmo a síťovou konfiguraci,
- spravovat více známých Wi-Fi sítí včetně persistentního auto-connect příznaku,
- samostatně zapnout/vypnout GoodWe, AZRouter, bazén a počasí,
- vyhledávat, přidávat, vybírat a řadit více lokalit počasí a zvolit provider,
- restartovat zařízení,
- po dvojím potvrzení vymazat namespace konfigurace a obnovit tovární hodnoty,
- exportovat a importovat uživatelskou konfiguraci ve verzovaném YAML formátu,
- provést ruční nebo GitHub OTA aktualizaci.

Přijetí refresh příkazu a fyzické dokončení e-paper obnovy jsou dvě různé
události a WebUI jejich stav již samostatně zobrazuje.

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

### Počasí

- poskytovatelé: Open-Meteo a MET Norway Locationforecast,
- transport: HTTPS s ověřením společně spravovaných kořenových CA certifikátů,
- zpracování: samostatná FreeRTOS úloha a společné rozhraní poskytovatelů,
- při vypnutí modulu se WeatherWorker korektně ukončí a uvolní task, cache i mutex,
- HTTPS/TLS fetch je serializovaný s paměťově náročným renderem displeje/preview,
- konfigurace podporuje nejvýše 8 uložených lokalit, aktivní lokalitu a jejich pořadí,
- hlavní Weather obrazovka používá obecný pager: jedna podstránka na lokalitu;
  dočasné přepnutí pageru nemění persistentní aktivní lokalitu pro Home,
- MET Norway: identifikační User-Agent, podmíněné požadavky `If-Modified-Since`
  a interval odvozený z hlaviček `Date` a `Expires`,
- model: aktuální podmínky, čtyři dny a nejvýše 32 bodů s datem a časem,
- hodinové podobrazovky: `weather-hourly-0` až `weather-hourly-3`,
- časový krok: Open-Meteo po třech hodinách, MET Norway podle dostupnosti po
  třech až šesti hodinách,
- pravděpodobnost srážek je volitelná; pokud ji provider neposkytne, displej
  zobrazuje pouze množství srážek.

## Konfigurace

Konfigurace se ukládá do ESP32 NVS přes Preferences, namespace **dashboard**.
Obsahuje systémové nastavení, síťové parametry, seznam známých Wi-Fi sítí,
GoodWe, AZRouter, bazén a počasí včetně více lokalit a jejich pořadí.

GoodWe a AZRouter mají samostatné příznaky `enabled`, host, port a polling
interval. Bazén má zatím příznak `enabled`. Počasí má `enabled`, provider,
polling interval, seznam nejvýše osmi lokalit a `activeLocationId`.
Dynamické moduly se při změně konfigurace registrují nebo odregistrují za běhu
a navigace se následně synchronizuje.

Známé Wi-Fi sítě ukládají SSID, heslo a `autoConnect`. Ruční **Odpojit**
nastaví pro dané SSID `autoConnect=false`; po restartu se proto nepřipojí samo.
Ruční **Připojit** síť znovu povolí. Když není dostupná aktivní síť, zařízení
udržuje recovery AP **Dashboard-Setup** a v AP+STA režimu zkouší povolené známé
sítě.

Tovární reset smaže celý namespace `dashboard`; po restartu se použijí hodnoty
z `ConfigSchema.h`. Aktuální `AppConfig::schemaVersion` je 6, ale tato hodnota
je zatím runtime metadata a není samostatně ukládaná ani migrovaná v NVS.
YAML záloha používá vlastní formát `pvdashboard-config` verze 1, obsahuje Wi-Fi
hesla v čitelné podobě a před zápisem validuje úplné schéma. Import se aplikuje
jako celek a zařízení se po něm restartuje.

`DisplayConfig.fullRefreshIntervalMinutes` je stále definované, ale současná
refresh politika ho nepoužívá.

## Provozní požadavky

- hesla a tokeny se nesmí objevit v běžném API ani logu,
- nedostupný zdroj nesmí vytvářet souvislou smyčku opakovaných pokusů,
- WebUI má odpovídat i během pomalého refreshu,
- na panelu se nesmí dlouhodobě hromadit ghosting,
- změna celé obrazovky používá čisticí plnou obnovu,
- firmware musí zůstat menší než jeden OTA slot **1 966 080 B**,
- neúspěšná OTA aktualizace nesmí poškodit běžící firmware,
- vypnutí modulu nesmí zanechat jeho obrazovku v sidebaru ani neplatný navigační focus.

## Stav kritérií stabilního základu

- [x] oba energetické zdroje lze samostatně vypnout a jejich výpadek nezablokuje WebUI,
- [x] běžná odpověď `/api/status` při výpadku zůstává mimo okamžik synchronního pollingu rychlá,
- [x] e-paper refresh běží mimo hlavní smyčku a WebUI zůstává dostupné,
- [x] přepnutí obrazovky používá čisticí full refresh,
- [x] WebUI zobrazuje queued/rendering/dokončení refreshu,
- [x] konfigurace přežije restart a podporuje export/import,
- [x] ruční i GitHub OTA byly ověřené včetně chybných scénářů,
- [x] počasí, bazén a FVE obrazovky respektují aktivaci modulů za běhu,
- [x] softwarová navigace Sidebar/Pager/Page a WebUI joystick jsou implementované,
- [ ] dokončit alespoň 24hodinový test ghostingu a stability bez významného úbytku heapu,
- [ ] rozšířit host-side testy integračních parserů a konfigurace.

Podrobný stav a otevřené body jsou v [PROJECT_STATUS.md](PROJECT_STATUS.md) a
[ROADMAP.md](ROADMAP.md).
