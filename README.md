# PVDashboard

Lokální domácí dashboard pro **Waveshare ESP32 e-Paper Driver Board** a černobílý
7,5" panel 800 × 480 px. Firmware zobrazuje data z měniče GoodWe a AZRouteru,
lokální BME280, stav akumulátoru přes MAX17048 a vybraná 433MHz čidla přes CC1101.
Součástí je mobilní WebUI, konfigurace přes NVS, recovery Wi-Fi AP, fyzické
ovládání a OTA aktualizace.

Aktuální firmware: **1.26.274.1**.

## Aktuální funkce

- obrazovky **home**, **solar**, **pool**, **weather** a **diagnostics**; FVE, bazén
  a počasí se za běhu registrují jen tehdy, když jsou příslušné moduly aktivní,
- GoodWe GW10K-ET přes Modbus RTU zapouzdřený v UDP na portu 8899,
- AZRouter přes HTTP endpointy **/api/v1/power**, **/api/v1/status** a **/api/v1/devices**,
- BME280 na společné I²C sběrnici GPIO21/GPIO22,
- MAX17048 na adrese 0x36: napětí, SoC, změna SoC a alert flags,
- CC1101 na 433 MHz, dekódování podporovaných čidel a správa uložených RF čidel,
- mobilní WebUI pro přepínání obrazovek, refresh, konfiguraci, náhled e-inku,
  správu RF čidel a OTA,
- společný **NavigationController** s režimy Sidebar / Pager / Page,
- fyzický pětisměrný joystick i virtuální joystick ve WebUI používají stejný
  navigační model,
- fyzický joystick: UP GPIO17, DOWN GPIO18, LEFT GPIO33, RIGHT GPIO16, OK GPIO32,
- doplňková tlačítka SET GPIO35 a RESET GPIO34 používají externí 10k pull-up;
  dlouhý SET přepíná soft ON/OFF displeje, krátký RESET vrací UI na Home/sidebar
  a krátký SET je zatím rezervovaný,
- NTP s časovou zónou pro Českou republiku,
- až **8 známých Wi-Fi sítí**, AP+STA recovery **Dashboard-Setup** a automatický
  návrat k dostupné povolené známé síti,
- živé počasí z Open-Meteo nebo MET Norway pro více uložených lokalit,
- konfigurovatelný Home layout včetně vlastních KPI/text/progress/sparkline prvků,
- dynamická viditelnost GoodWe/AZRouteru, bazénu a počasí bez ztráty konfigurace,
- asynchronní e-paper render přes DisplayWorker a regionální partial refresh
  pro vybrané změny dat a navigace.

Počasí načítá samostatná FreeRTOS úloha. BME280, MAX17048 a RF senzory jsou
lokální zdroje. Hodnoty bazénu zůstávají demonstrační, dokud nebude připojen
samostatný reálný uzel.

## Sestavení a nahrání

Požadavky:

- VS Code s PlatformIO nebo PlatformIO CLI,
- Waveshare ESP32 e-Paper Driver Board,
- USB port zařízení; v současném testovacím zapojení **COM5**.

~~~powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM5
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM5 --baud 115200
~~~

WebUI je po připojení dostupné přes **http://dashboard.local/** nebo IP adresu
zařízení. Testovací IP není součástí pevné konfigurace firmware.

Pokud se ESP32 nepřipojí k aktivní Wi-Fi, přejde do AP+STA recovery režimu,
vytvoří síť **Dashboard-Setup** s heslem **dashboard** a dál průběžně hledá
povolené známé sítě. Nastavení je dostupné na **http://192.168.4.1/**.

## Release a návrat verze

PlatformIO Core je připnuté v release workflow; platforma, framework, nástroje a
knihovny jsou připnuté v **platformio.ini**.

~~~powershell
python scripts/prepare-release.py --firmware .pio/build/esp32dev/firmware.bin --output dist --expected-version 1.26.274.1
~~~

Výstup obsahuje **firmware.bin**, **firmware.bin.sha256** a
**dashboard-manifest.json**. Aktuální OTA partition poskytuje dva sloty po
**1 966 080 B (1,875 MiB)**.

## REST API

Hlavní veřejné endpointy používané WebUI a diagnostikou:

| Metoda | Endpoint | Účel |
| --- | --- | --- |
| GET | /api/status | Odlehčený stav systému a zdrojů |
| GET | /api/status?details=1 | Detailní stav a performance diagnostika |
| GET | /api/screens | Seznam obrazovek |
| POST | /api/screens/{id}/activate | Aktivace obrazovky |
| GET | /api/navigation | Stav navigace, pageru a focusu |
| POST | /api/navigation | Akce up/down/left/right/ok |
| GET | /api/display | Metadata posledního preview |
| GET | /api/display.bmp | BMP náhled posledního vyrenderovaného e-inku |
| POST | /api/display/refresh | Differential partial refresh |
| POST | /api/display/full-refresh | Čisticí full refresh |
| GET | /api/layout/home | Home layout, limity a katalog datových zdrojů |
| POST | /api/layout/home | Validace a uložení Home layoutu |
| POST | /api/layout/home/reset | Reset Home layoutu |
| GET | /api/config/export | Export konfigurace YAML |
| POST | /api/config/import | Import konfigurace YAML |
| POST | /api/config/factory-reset | Tovární reset |
| GET | /api/config/timezone | Dostupné časové zóny |
| POST | /api/config/system-v2 | Systém/časová konfigurace |
| POST | /api/config/system-network | Kombinovaná systémová a síťová konfigurace |
| GET | /api/config/wifi | Bezpečný Wi-Fi stav bez vracení hesla |
| GET | /api/network/config | DHCP/statická IP konfigurace a aktuální lease |
| POST | /api/network/config | Uložení DHCP/statické IP konfigurace |
| GET | /api/wifi/scan | Scan Wi-Fi sítí |
| GET | /api/wifi/known | Známé Wi-Fi sítě |
| POST | /api/wifi/config-v2 | Připojení/uložení Wi-Fi s možností zachovat heslo |
| POST | /api/wifi/connect-known | Připojení ke známé síti |
| POST | /api/wifi/disconnect | Odpojení a zákaz auto-connectu aktivního SSID |
| POST | /api/wifi/forget | Zapomenutí známé sítě |
| POST | /api/config/sources | GoodWe/AZRouter konfigurace |
| POST | /api/sources/test | Diagnostický test hostu/portu před uložením |
| GET | /api/diagnostics/device?source=... | Diagnostika nakonfigurovaného GoodWe/AZRouter cíle |
| POST | /api/config/pool | Nastavení bazénového modulu |
| POST | /api/config/weather | Nastavení počasí |
| POST | /api/weather/locations | Správa lokalit počasí |
| GET | /api/ntp/status | Stav NTP synchronizace |
| GET/POST | /api/ntp/custom | Správa vlastních NTP serverů |
| GET | /api/rf-sensors | Stav uložených a nalezených RF čidel |
| POST | /api/rf-sensors/scan | Scan RF čidel |
| POST | /api/rf-sensors/add | Přidání nalezeného čidla |
| POST | /api/rf-sensors/rename | Přejmenování uloženého čidla |
| POST | /api/rf-sensors/rebind | Převázání slotu na nové rádiové ID |
| POST | /api/rf-sensors/remove | Odebrání čidla |
| POST | /api/system/restart | Restart ESP32 |
| GET | /api/update/check | Kontrola GitHub release |
| POST | /api/update/github | Instalace GitHub release |
| POST | /api/update | Ruční upload firmware |

Starší kompatibilní endpointy `/api/config/system` a `/api/wifi/config`
zůstávají v kódu, ale aktuální WebUI používá rozšířené varianty.

## Konfigurace

Interní **AppConfig schemaVersion je 12**. YAML export/import používá
**pvdashboard-config v7**. Export zahrnuje také Home layout a uložená RF čidla.
Celý seznam známých Wi-Fi sítí včetně jejich autoConnect příznaků zatím exportován není.

## Dokumentace

- [Aktuální stav projektu](docs/PROJECT_STATUS.md)
- [Architektura a provozní principy](docs/ARCHITECTURE.md)
- [Displej a refresh strategie](docs/DISPLAY.md)
- [GoodWe, AZRouter a počasí](docs/INTEGRATIONS.md)
- [Layouty](docs/LAYOUTS.md)
- [433 MHz / CC1101](docs/RF_433_RESEARCH.md)
- [Testování a diagnostika](docs/TESTING.md)
- [Roadmapa](docs/ROADMAP.md)

## Známá omezení

- GoodWe a AZRouter polling stále běží synchronně v hlavní smyčce a timeout může
  krátce zdržet WebUI.
- E-paper obsluhuje samostatná FreeRTOS úloha.
- Dlouhodobý test ghostingu regionálních a diferenciálních partial refreshů
  ještě není uzavřený.
- Bazénové hodnoty zatím nejsou napojené na reálný senzorický uzel.
- MAX17048 spolehlivě měří napětí a SoC, ale sám neurčuje jistě stav nabíjení.
  Detekce z HW-357/TP4056 CHG je vedena v roadmapě.
- RF dekódování a správa čidel jsou funkční, ale výzkum dalších protokolů pokračuje.
