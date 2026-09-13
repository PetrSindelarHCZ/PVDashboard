# Dashboard – startovní dokumentace pro VS Code / PlatformIO

**Stav:** základní integrace M0–M5 a webové OTA implementovány  
**Datum:** 13. 9. 2026  
**Cíl dokumentu:** převést dosavadní návrh domácího e-paper Dashboardu do praktického základu nového projektu ve VS Code a začít implementovat firmware samotného dashboardu.

---

## 1. Rozsah první etapy

V této etapě vzniká pouze firmware a webové rozhraní **Dashboardu**.

### Součást první etapy

- ESP32 firmware v C++.
- Vývoj ve VS Code + PlatformIO.
- Wi-Fi připojení.
- NTP a lokální čas.
- Ovladač 7,5" černobílého e-paper displeje 800 × 480 px.
- Základní vykreslovací vrstva.
- Více obrazovek dashboardu.
- Přepínání obrazovek z telefonu přes lokální webové rozhraní.
- Webová diagnostika.
- Konfigurace oddělená od firmware (aktuálně NVS; Wi-Fi, GoodWe a AZRouter).
- Wi-Fi recovery AP se scanem okolních sítí.
- OTA aktualizace firmware přes web.
- Připravená architektura pro GoodWe GW10K-ET a AZRouter.
- Git repozitář od začátku projektu.

### Není součástí této etapy

- simulátor GoodWe,
- simulátor AZRouteru,
- fyzická tlačítka,
- hardwarová navigace,
- Home Assistant,
- bazénová čidla,
- vzdálená venkovní čidla a jejich gateway,
- finální dynamický drag-and-drop editor layoutu.

Simulátory budou samostatný nástroj vytvořený později. Firmware dashboardu na nich nesmí být architektonicky závislý.

---

## 2. Cílový hardware prototypu

### Displej

- Waveshare e-paper,
- černobílý,
- 7,5",
- rozlišení 800 × 480 px,
- orientace landscape,
- čas bez sekund.

Před implementací konkrétního display driveru je stále nutné ověřit přesnou revizi panelu.

### Řídicí deska

První prototyp používá již vlastněný:

**Waveshare ESP32 e-Paper Driver Board**

Známá omezení současné desky:

- přibližně 4 MB flash,
- bez PSRAM,
- Wi-Fi,
- velikost firmware, webového UI, fontů, filesystemu a OTA partition je nutné sledovat.

Aktuální výchozí ESP32 partition layout obsahuje dvě OTA partition `app0` a `app1`, každou o velikosti `0x140000` (1 310 720 B). Aktuální firmware má přibližně 1,14 MB, takže se do obou partition vejde; OTA lze implementovat bez okamžité změny layoutu.

ESP32-S3 N16R8 zůstává možná budoucí náhrada, pokud současná deska začne být paměťově omezující.

Důležitý architektonický požadavek:

> Display driver a hardware-specific část nesmí být provázána s aplikační logikou.

Přechod na jiný ESP32 nebo jiný driver displeje proto nesmí znamenat přepis datových zdrojů, webového serveru ani obrazovek.

---

## 3. Vývojové prostředí

Použít:

- VS Code,
- PlatformIO,
- C++,
- Arduino framework pro ESP32,
- Git.

Doporučený název repozitáře:

```text
home-dashboard
```

Doporučený název PlatformIO projektu:

```text
dashboard-esp32
```

Přesný `board` profil v `platformio.ini` ověřit podle konkrétní revize Waveshare Driver Board před prvním uploadem. Nepředpokládat automaticky ESP32-S3.

---

## 4. Princip první verze

ESP32 bude fungovat jako samostatný lokální dashboard:

```text
                 domácí Wi-Fi
                      │
        ┌─────────────┴─────────────┐
        │                           │
      telefon                    ESP32
   webový prohlížeč               │
        │                         │
        └──── HTTP / REST ────────┤
                                  │
                         ┌────────┴─────────┐
                         │ Dashboard Core   │
                         └────────┬─────────┘
                                  │
                  ┌───────────────┼───────────────┐
                  │               │               │
             GoodWe client   AZRouter client   NTP/system
                  │               │
             UDP 8899         HTTP REST
                  │               │
              GW10K-ET         AZRouter
                                  │
                                  ▼
                            e-paper 800×480
```

Telefon není zdroj dat. Slouží pouze jako ovládací a administrační rozhraní dashboardu.

---

## 5. Navigace v první verzi

V první verzi **nejsou fyzická tlačítka**.

Proto nebude implementován `ButtonManager`, GPIO debounce ani obsluha tlačítek.

### Ovládání z telefonu

ESP32 poskytne jednoduchou mobilní stránku například:

```text
http://dashboard.local/
```

Na úvodní stránce budou velká tlačítka:

```text
┌─────────────────────────┐
│       DASHBOARD         │
├─────────────────────────┤
│ [ Hlavní souhrn ]       │
│ [ FVE ]                 │
│ [ Bazén ]               │
│ [ Počasí ]              │
│ [ Diagnostika ]         │
├─────────────────────────┤
│ [ Obnovit displej ]     │
│ [ Nastavení ]           │
└─────────────────────────┘
```

Stisknutí položky:

1. změní aktivní obrazovku,
2. požádá renderer o vykreslení,
3. provede potřebný refresh e-paperu,
4. web vrátí potvrzení aktuální stránky.

### Po restartu

Pro první implementaci:

- výchozí stránka = `home`,
- poslední stránku zatím není nutné ukládat,
- později lze přidat volbu `rememberLastPage`.

---

## 6. Výchozí obrazovky

Připravit identifikátory již nyní, i když jejich obsah bude zpočátku jednoduchý:

```text
home
solar
pool
weather
diagnostics
```

### Home

První testovací obsah:

- datum,
- čas,
- stav Wi-Fi,
- IP adresa,
- jednoduchá testovací hodnota,
- stav datových zdrojů.

### Solar

Budoucí data:

- aktuální výkon FVE,
- dnešní výroba,
- import/export,
- spotřeba domu,
- stav spojení GoodWe,
- vybrané informace z AZRouteru.

Baterie není v první zobrazovací verzi prioritou.

### Pool

Zatím pouze placeholder.

### Weather

Zatím pouze placeholder.

### Diagnostics

- firmware version,
- uptime,
- heap,
- velikost flash,
- síla Wi-Fi,
- IP,
- čas poslední synchronizace NTP,
- stav GoodWe,
- stav AZRouteru,
- čas posledního úspěšného čtení,
- počet chyb komunikace.

---

## 7. Navržená struktura projektu

```text
dashboard-esp32/
│
├─ platformio.ini
├─ README.md
├─ .gitignore
│
├─ include/
│  ├─ AppConfig.h
│  └─ Version.h
│
├─ src/
│  ├─ main.cpp
│  │
│  ├─ app/
│  │  ├─ DashboardApp.cpp
│  │  └─ DashboardApp.h
│  │
│  ├─ display/
│  │  ├─ IDisplay.h
│  │  ├─ EpaperDisplay.cpp
│  │  ├─ EpaperDisplay.h
│  │  ├─ DisplayManager.cpp
│  │  └─ DisplayManager.h
│  │
│  ├─ screens/
│  │  ├─ IScreen.h
│  │  ├─ ScreenManager.cpp
│  │  ├─ ScreenManager.h
│  │  ├─ HomeScreen.cpp
│  │  ├─ SolarScreen.cpp
│  │  ├─ PoolScreen.cpp
│  │  ├─ WeatherScreen.cpp
│  │  └─ DiagnosticsScreen.cpp
│  │
│  ├─ data/
│  │  ├─ DataModel.cpp
│  │  ├─ DataModel.h
│  │  ├─ DataPoint.h
│  │  └─ DataSourceStatus.h
│  │
│  ├─ integrations/
│  │  ├─ goodwe/
│  │  │  ├─ GoodWeClient.cpp
│  │  │  └─ GoodWeClient.h
│  │  └─ azrouter/
│  │     ├─ AZRouterClient.cpp
│  │     └─ AZRouterClient.h
│  │
│  ├─ network/
│  │  ├─ WifiManager.cpp
│  │  ├─ WifiManager.h
│  │  ├─ TimeService.cpp
│  │  ├─ TimeService.h
│  │  ├─ WebServer.cpp
│  │  └─ WebServer.h
│  │
│  ├─ config/
│  │  ├─ ConfigManager.cpp
│  │  ├─ ConfigManager.h
│  │  └─ ConfigSchema.h
│  │
│  ├─ update/
│  │  ├─ OtaManager.cpp
│  │  └─ OtaManager.h
│  │
│  └─ diagnostics/
│     ├─ Diagnostics.cpp
│     └─ Diagnostics.h
│
├─ data/
│  ├─ www/
│  │  ├─ index.html
│  │  ├─ app.js
│  │  └─ app.css
│  └─ default-config.json
│
├─ docs/
│  ├─ ARCHITECTURE.md
│  ├─ DATA_MODEL.md
│  ├─ WEB_API.md
│  └─ HARDWARE.md
│
└─ tools/
   └─ README.md
```

Adresář `tools/` je rezervovaný pro budoucí simulátory a pomocné PC nástroje. V první etapě zůstane prázdný.

---

## 8. Zodpovědnosti hlavních komponent

### `DashboardApp`

Centrální orchestrace aplikace.

Nemá obsahovat vlastní kreslení, HTML ani protokol GoodWe.

Zodpovídá za:

- inicializaci služeb,
- hlavní stav aplikace,
- plánování pravidelných úloh,
- reakci na požadavek změny obrazovky.

### `DisplayManager`

Jediná vrstva, která rozhoduje:

- full refresh,
- partial refresh,
- zda je refresh vůbec nutný.

Renderer jednotlivých obrazovek nemá přímo komunikovat s hardwarem displeje.

### `ScreenManager`

Spravuje:

- seznam dostupných obrazovek,
- právě aktivní obrazovku,
- přepnutí obrazovky,
- lookup podle ID.

Například:

```cpp
screenManager.show("solar");
```

Tento mechanismus později využijí i fyzická tlačítka, ale jejich implementace teď neexistuje.

### `DataModel`

Normalizovaná data nezávislá na zdroji.

Obrazovka například nesmí přímo číst GoodWe Modbus registr.

Správně:

```text
GoodWeClient
      │
      ▼
DataModel.solar.productionPower
      │
      ▼
SolarScreen
```

Nikoli:

```text
SolarScreen → UDP → GoodWe register
```

---

## 9. Normalizovaný datový model

První návrh:

```cpp
struct SolarData {
    bool available;
    float productionPowerW;
    float houseConsumptionW;
    float gridPowerW;
    float energyTodayKWh;
    uint32_t lastUpdateMs;
};

struct AZRouterData {
    bool available;
    float gridPowerW;
    float routedPowerW;
    float routedEnergyTodayKWh;
    uint32_t lastUpdateMs;
};
```

Později lze model rozšířit bez změny rendereru.

Každý zdroj musí mít alespoň:

```text
available
lastSuccess
lastAttempt
errorCount
lastError
```

---

## 10. GoodWe – rozhraní dashboardu

Známé zařízení:

```text
GoodWe GW10K-ET
```

Komunikace:

```text
UDP
port 8899
```

Dashboard musí mít GoodWe endpoint samostatně konfigurovatelný:

```json
{
  "goodwe": {
    "enabled": true,
    "host": "192.168.100.xxx",
    "port": 8899
  }
}
```

GoodWe client musí být samostatná komponenta.

V první implementaci není nutné číst všechny dostupné registry. Začít pouze daty skutečně potřebnými na obrazovku FVE.

Komunikace musí být:

- read-only,
- s timeoutem,
- s omezeným retry,
- neblokující hlavní aplikaci po dlouhou dobu,
- odolná proti ztracenému UDP paketu.

---

## 11. AZRouter – rozhraní dashboardu

AZRouter používá lokální HTTP JSON API.

Známé endpointy:

```text
/api/v1/status
/api/v1/power
/api/v1/devices
/api/v1/settings
```

Pro běžný dashboard není potřeba `/settings` pravidelně číst.

Důležitá bezpečnostní poznámka:

> `/api/v1/settings` může obsahovat Wi-Fi heslo v otevřeném textu.

Proto se jeho obsah nesmí logovat ani ukládat do diagnostického exportu.

Konfigurace musí mít samostatný endpoint:

```json
{
  "azrouter": {
    "enabled": true,
    "host": "192.168.100.xxx",
    "port": 80
  }
}
```

V reálné instalaci mají GoodWe a AZRouter dvě různé IP adresy.

Dashboard proto nikdy nesmí předpokládat společnou IP pro FVE.

---

## 12. Konfigurace

Konfigurace musí být oddělena od firmware.

Doporučené rozdělení:

```text
system
network
display
sources
screens
```

První návrh:

```json
{
  "schemaVersion": 1,
  "system": {
    "hostname": "dashboard",
    "timezone": "Europe/Prague",
    "ntpServer": "pool.ntp.org"
  },
  "display": {
    "defaultScreen": "home",
    "fullRefreshIntervalMinutes": 1440
  },
  "sources": {
    "goodwe": {
      "enabled": false,
      "host": "",
      "port": 8899,
      "pollIntervalSeconds": 10
    },
    "azrouter": {
      "enabled": false,
      "host": "",
      "port": 80,
      "pollIntervalSeconds": 10
    }
  }
}
```

Wi-Fi heslo neukládat do exportované konfigurace v čitelné podobě.

---

## 13. Webové API – první verze

Mobilní webové UI používat nad jednoduchým REST API.

### Stav

```text
GET /api/status
```

Vrací například:

```json
{
  "firmware": "0.1.0",
  "uptime": 12345,
  "wifi": {
    "connected": true,
    "rssi": -57
  },
  "screen": "solar",
  "goodwe": {
    "available": true
  },
  "azrouter": {
    "available": false
  }
}
```

### Seznam obrazovek

```text
GET /api/screens
```

### Přepnutí obrazovky

```text
POST /api/screens/home/activate
POST /api/screens/solar/activate
POST /api/screens/pool/activate
POST /api/screens/weather/activate
POST /api/screens/diagnostics/activate
```

### Ruční refresh

```text
POST /api/display/refresh
```

### Full refresh

```text
POST /api/display/full-refresh
```

### Restart

```text
POST /api/system/restart
```

Restart má být ve webovém UI chráněn proti náhodnému stisknutí potvrzením.

---

## 14. Mobilní webové rozhraní

První UI nemá být složité.

Cíl:

- funguje dobře na telefonu,
- žádný frontend framework,
- minimum JavaScriptu,
- minimum flash,
- žádné externí CDN,
- vše funguje i bez internetu.

Preferovat:

```text
HTML + CSS + vanilla JavaScript
```

Na telefonu se UI použije pro:

- přepnutí aktivní obrazovky,
- zobrazení aktuálního stavu,
- ruční refresh,
- základní konfiguraci,
- později OTA.

Neimplementovat nyní drag-and-drop editor.

---

## 15. Wi-Fi a první spuštění

Pořadí:

```text
BOOT
 │
 ├─ načíst konfiguraci
 │
 ├─ inicializovat display
 │
 ├─ pokus připojit Wi-Fi
 │
 ├─ synchronizovat čas
 │
 ├─ spustit web server
 │
 ├─ inicializovat zdroje dat
 │
 └─ zobrazit HOME
```

Pokud Wi-Fi připojení selže:

- dashboard se nesmí restartovat ve smyčce,
- displej zobrazí stav `Wi-Fi unavailable`,
- webová konfigurace použije recovery/AP režim.

AP provisioning je implementován: po neúspěšném připojení se vytvoří síť `Dashboard-Setup` na `192.168.4.1`; okolní sítě lze vyhledat nebo SSID zadat ručně.

---

## 16. Čas a NTP

Požadavky:

- timezone `Europe/Prague`,
- automatické DST,
- čas bez sekund,
- NTP synchronizace po startu,
- následná periodická resynchronizace.

Displej nesmí provádět full refresh každou minutu jen kvůli času.

Časovou oblast později aktualizovat partial refreshem, pokud to konkrétní panel spolehlivě podporuje.

---

## 17. E-paper refresh strategie

První implementace:

### Full refresh

- boot,
- změna celé obrazovky,
- ruční požadavek z telefonu,
- pravidelný servisní refresh.

### Partial refresh

Později:

- čas,
- jednotlivé změněné hodnoty,
- malé stavové ikony.

Nejdříve musí být ověřeno, jak se chová konkrétní revize 7,5" panelu.

První milestone může používat pouze full refresh. Je lepší nejprve stabilní jednoduchá implementace než předčasná optimalizace partial refreshe.

---

## 18. Webový OTA update

OTA je implementováno přes `OtaManager` a webový upload firmware na `POST /api/update`. Formulář je dostupný na hlavní webové stránce. Zařízení se restartuje pouze po úspěšném ověření nahraného obrazu.

Požadované budoucí chování:

```text
Web UI
  │
  └─ Firmware update
       ├─ výběr .bin
       ├─ kontrola
       ├─ upload
       ├─ flash
       └─ reboot
```

Důležité kvůli 4 MB flash:

- ověřit velikost obou OTA partition,
- filesystem a firmware nesmí znemožnit dual-slot OTA,
- sledovat velikost firmware od první verze.

Pokud se dual OTA + web UI nevejde rozumně do 4 MB, je to jeden z hlavních důvodů pro přechod na ESP32-S3 N16R8.

---

## 19. Logování

Použít několik úrovní:

```text
ERROR
WARN
INFO
DEBUG
```

Serial output při vývoji.

Každý důležitý modul má vlastní prefix:

```text
[APP]
[WIFI]
[NTP]
[DISPLAY]
[WEB]
[GOODWE]
[AZROUTER]
[CONFIG]
[OTA]
```

Nikdy nelogovat:

- Wi-Fi heslo,
- autentizační token,
- celé `/api/v1/settings` z AZRouteru.

---

## 20. První implementační milestone

### M0 – Skeleton

Cíl:

- PlatformIO projekt se sestaví,
- běží `setup()` + `loop()`,
- sériový log,
- `DashboardApp`,
- základní adresářová struktura,
- Git initial commit.

### M1 – Wi-Fi + webové ovládání

Cíl:

- ESP se připojí k Wi-Fi,
- `dashboard.local` nebo IP otevře web,
- `/api/status`,
- mobilní stránka,
- přepínání interního ID obrazovky z telefonu.

Displej ještě může být pouze stub.

### M2 – e-paper

Cíl:

- inicializace skutečného panelu,
- test full refresh,
- Home obrazovka,
- Solar placeholder,
- Diagnostics obrazovka,
- přepnutí stránky z telefonu skutečně překreslí displej.

### M3 – konfigurace

**Stav: implementováno částečně.** Wi-Fi, GoodWe a AZRouter se ukládají do NVS; web umožňuje měnit hosty, porty, aktivaci a intervaly.

Cíl:

- filesystem,
- `config.json`,
- načtení defaultu,
- validace `schemaVersion`,
- konfigurace IP GoodWe/AZRouteru.

### M4 – GoodWe

**Stav: implementováno a ověřeno proti simulátoru.**

Cíl:

- read-only UDP client,
- minimální sada dat,
- mapování do `DataModel`,
- SolarScreen se skutečnými hodnotami.

### M5 – AZRouter

**Stav: implementováno a ověřeno proti simulátoru.**

Cíl:

- HTTP JSON client,
- `/power` + `/status`,
- mapování do `DataModel`,
- rozšíření SolarScreen.

OTA je implementováno jako další samostatný milestone; zbývá ověřit upgrade platným firmware obrazem v provozu.

---

## 21. Co nyní záměrně neimplementovat

Aby se začátek projektu nerozbujel:

- žádné fyzické button GPIO,
- žádný debounce,
- žádný automatický carousel obrazovek,
- žádný simulátor,
- žádný MQTT,
- žádný Home Assistant,
- žádný grafický layout editor,
- žádný komplexní framework webového UI,
- žádná databáze,
- žádná historie na flash,
- žádné bazénové senzory,
- žádná vzdálená gateway.

---

## 22. Definition of Done pro první skutečně použitelnou verzi

První základ je hotový, pokud:

1. ESP32 nabootuje bez PC.
2. Připojí se k domácí Wi-Fi.
3. Získá správný místní čas.
4. Na e-paperu zobrazí Home obrazovku.
5. Z telefonu lze otevřít lokální web.
6. Z telefonu lze přepnout `Home / Solar / Diagnostics`.
7. Změna obrazovky provede správný refresh.
8. Po výpadku datového zdroje se firmware nezablokuje.
9. GoodWe a AZRouter mají samostatnou konfigurovatelnou IP.
10. Konfigurace je oddělena od firmware.
11. Projekt je verzovaný v Gitu.
12. Žádné heslo nebo token se neobjeví v logu ani běžném exportu.

---

## 23. Doporučený první commit

```text
chore: initialize ESP32 dashboard project
```

Obsah:

```text
platformio.ini
README.md
src/main.cpp
src/app/DashboardApp.*
src/screens/ScreenManager.*
src/data/DataModel.*
docs/
.gitignore
```

Poté:

```text
feat: add WiFi and mobile dashboard control
```

---

## 24. Zadání pro první práci agenta ve VS Code

> V tomto repozitáři vytváříme firmware domácího e-paper Dashboardu pro stávající Waveshare ESP32 e-Paper Driver Board a černobílý Waveshare 7,5" panel 800×480. Projekt používá C++/Arduino a PlatformIO. Začni vytvořením čisté modulární kostry projektu podle této dokumentace. Nyní neimplementuj simulátor, GoodWe ani AZRouter protokol a neimplementuj fyzická tlačítka. Vytvoř DashboardApp, ScreenManager, DataModel a základní WebServer. Připrav obrazovky `home`, `solar`, `pool`, `weather`, `diagnostics`. Webové rozhraní musí být mobile-first a umožnit změnit aktivní obrazovku přes REST API. Display vrstvu zatím vytvoř jako rozhraní/stub, dokud nebude ověřena přesná revize panelu a driver boardu. Nepřidávej těžký frontend framework ani externí CDN. Projekt musí jít sestavit v PlatformIO a všechny hardware-specific části musí být oddělené od aplikační logiky.

---

## 25. Nejbližší konkrétní postup

Po založení repozitáře postupovat v tomto pořadí:

```text
1. PlatformIO skeleton
2. Git
3. DashboardApp
4. ScreenManager
5. DataModel
6. Wi-Fi
7. HTTP server
8. mobilní přepínání obrazovek
9. ověření přesné revize e-paper hardware
10. skutečný display driver
11. konfigurace
12. GoodWe
13. AZRouter
14. OTA
```

Toto pořadí umožní začít programovat i bez fyzických tlačítek a zároveň se neblokovat neověřenou revizí e-paper driveru.
