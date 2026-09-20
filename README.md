# PVDashboard

Lokální domácí dashboard pro **Waveshare ESP32 e-Paper Driver Board** a černobílý
7,5" panel 800 × 480 px. Firmware zobrazuje data z měniče GoodWe a AZRouteru,
poskytuje mobilní WebUI, konfiguraci přes NVS, recovery Wi-Fi AP a OTA aktualizaci.

Aktuální firmware: **1.26.261.1**. Obsahuje stabilizované načítání počasí přes HTTPS,
paměťově úsporný náhled e-paperu ve WebUI, diagnostiku výkonu, společnou navigaci
pro budoucí joystick a dynamické zobrazování modulů podle konfigurace.

## Aktuální funkce

- obrazovky **home**, **solar**, **pool**, **weather** a **diagnostics**; FVE, bazén
  a počasí se za běhu registrují jen tehdy, když jsou příslušné moduly aktivní,
- GoodWe GW10K-ET přes Modbus RTU zapouzdřený v UDP na portu 8899,
- AZRouter přes HTTP endpointy **/api/v1/power**, **/api/v1/status** a **/api/v1/devices**,
- mobilní WebUI pro přepínání obrazovek, refresh, konfiguraci, náhled e-inku a OTA,
- společný **NavigationController** s režimy Sidebar / Pager / Page, virtuálním
  pětisměrným joystickem ve WebUI a geometricky odvozenou navigací prvků,
- NTP s časovou zónou pro Českou republiku,
- více známých Wi-Fi sítí, AP+STA recovery **Dashboard-Setup** a automatický
  návrat k dostupné povolené známé síti,
- měření dob hlavní smyčky, HTTP, integrací a e-paper refreshů,
- živé počasí z Open-Meteo nebo MET Norway pro více uložených lokalit,
  čtyřdenní předpověď, hodinový přehled pro vybraný den, pager lokalit na e-inku,
  změnu pořadí lokalit a vyhledání místa ve WebUI,
- ověřené HTTPS pro oba poskytovatele a respektování serverové cache MET Norway,
- dynamickou viditelnost GoodWe/AZRouteru, bazénu a počasí bez ztráty uložené konfigurace.

Počasí načítá samostatná FreeRTOS úloha a při nedostupnosti API se na displeji
nezobrazují náhradní čísla. Hodnoty vnitřních čidel a bazénu jsou zatím
demonstrační a firmware i WebUI je tak označují. Podrobnosti datových zdrojů jsou v [INTEGRATIONS.md](docs/INTEGRATIONS.md).

## Sestavení a nahrání

Požadavky:

- VS Code s PlatformIO nebo PlatformIO CLI,
- Waveshare ESP32 e-Paper Driver Board,
- USB port zařízení, v současném testovacím zapojení **COM5**.

~~~powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM5
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM5 --baud 115200
~~~

WebUI je po připojení dostupné přes **http://dashboard.local/** nebo IP adresu
zařízení. V současném testovacím zapojení zařízení používá **192.168.88.181**;
nejde o pevnou adresu firmware.

Pokud se ESP32 nepřipojí k aktivní Wi-Fi, přejde do AP+STA recovery režimu,
vytvoří síť **Dashboard-Setup** s heslem **dashboard** a dál průběžně hledá
povolené známé sítě. Nastavení je dostupné na **http://192.168.4.1/**. Ruční
**Odpojit** zakáže auto-connect daného SSID i přes restart; ruční **Připojit**
jej znovu povolí.

## Release a návrat verze

PlatformIO Core je připnuté v release workflow; platforma, framework, nástroje a knihovny jsou
připnuté v **platformio.ini**. Po sestavení připraví validované artefakty tento
příkaz:

~~~powershell
python scripts/prepare-release.py --firmware .pio/build/esp32dev/firmware.bin --output dist --expected-version 1.26.261.1
~~~

Výstup obsahuje **firmware.bin**, **firmware.bin.sha256** a
**dashboard-manifest.json**. Aktuální `min_spiffs.csv` poskytuje dva OTA sloty
po **1 966 080 B (1,875 MiB)**; release skript i firmware větší obraz odmítnou.
GitHub workflow provádí stejné kontroly a tag ve formátu **v1.YY.denRoku.pořadí** musí odpovídat
**FIRMWARE_VERSION**. Ruční upload ve WebUI vyžaduje vložit 64znakový SHA-256 ze souboru **firmware.bin.sha256** a ověří jej ještě před aktivací oddílu.

Před OTA je vhodné ponechat si poslední známý funkční **firmware.bin**. Chyba
uploadu nebo SHA-256 vrátí HTTP 400 a běžící partition zůstane aktivní. Pokud
zařízení po platné aktualizaci nenaběhne, připojte USB, zvolte odpovídající
známý funkční commit a obnovte jej sériově:

~~~powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM5
~~~

Tento postup nemaže NVS. Úplné mazání flash není součástí běžného návratu verze.

WebUI umožňuje ručně zkontrolovat poslední GitHub release. Nabídku instalace zobrazí
jen pro novější verzi a před stažením vyžádá potvrzení uživatele. Zařízení používá
URL a SHA-256 získané přímo z GitHub release, ověří celý obraz a potom se restartuje.

## REST API

| Metoda | Endpoint | Účel |
| --- | --- | --- |
| GET | /api/status | Odlehčený stav systému a zdrojů |
| GET | /api/status?details=1 | Stav systému včetně výkonnostních a podrobných AZRouter metrik |
| GET | /api/screens | Seznam obrazovek |
| POST | /api/screens/{id}/activate | Aktivace obrazovky |
| GET | /api/navigation | Stav navigace, focus a geometrie focusovatelných prvků |
| POST | /api/navigation | Navigační akce `up/down/left/right/ok` |
| POST | /api/display/refresh | Rychlá částečná obnova |
| POST | /api/display/full-refresh | Čisticí plná obnova |
| POST | /api/system/restart | Restart ESP32 |
| POST | /api/config/factory-reset | Vymazání konfigurace po potvrzení `confirmation=RESET` |
| GET | /api/config/export | Stažení konfigurace ve formátu YAML |
| POST | /api/config/import | Validace a import těla `application/yaml` |
| POST | /api/config/system | Uložení hostname, NTP serveru a časového pásma |
| POST | /api/wifi/config | Uložení Wi-Fi konfigurace |
| GET | /api/wifi/scan | Vyhledání Wi-Fi sítí |
| POST | /api/config/sources | Uložení konfigurace GoodWe/AZRouteru a živá aktualizace FVE obrazovky |
| POST | /api/config/pool | Uložení viditelnosti bazénového modulu |
| POST | /api/config/weather | Uložení provideru, aktivace a intervalu počasí |
| POST | /api/weather/locations | Přidání, výběr, odstranění nebo změna pořadí lokalit |
| GET | /api/wifi/known | Seznam známých Wi-Fi sítí |
| POST | /api/wifi/disconnect | Ruční odpojení a zakázání auto-connectu aktivního SSID |
| GET | /api/display.bmp | BMP náhled posledního vyrenderovaného e-inku |
| POST | /api/screens/weather-hourly-0/activate | Hodinový přehled prvního dne (indexy 0–3) |
| GET | /api/update/check | Kontrola GitHub release |
| POST | /api/update/github | Instalace release firmware |
| POST | /api/update | Ruční upload firmware s polem `sha256` |

## Dokumentace

- [Aktuální stav projektu](docs/PROJECT_STATUS.md)
- [Architektura a provozní principy](docs/ARCHITECTURE.md)
- [Displej a refresh strategie](docs/DISPLAY.md)
- [GoodWe, AZRouter a počasí](docs/INTEGRATIONS.md)
- [Testování a diagnostika](docs/TESTING.md)
- [Roadmapa](docs/ROADMAP.md)

Starší specifikace, výsledky jednorázových měření a pracovní audity zůstávají
dostupné v Git historii místo samostatného archivu v aktuálním stromu.

## Známá omezení

- Čtení datových zdrojů stále běží synchronně v hlavní smyčce. WebUI proto může
  během jednotlivého síťového timeoutu čekat přibližně 1,8 sekundy.
- E-paper obsluhuje samostatná FreeRTOS úloha; WebUI během partial ani full
  refreshu zůstává dostupné a zobrazuje stav vykreslení.
- Platná ruční i GitHub OTA a chybové scénáře jsou ověřené na zařízení.
- Aktuální OTA partition má **1 966 080 B**. Release skript i zařízení odmítnou
  větší obraz; skutečné procento využití se mění s každým buildem a má se
  kontrolovat v CI/release výstupu.
- Fyzický joystick zatím není připojen; WebUI už používá stejný navigační model.
- Reálná bazénová a vnitřní čidla zatím nejsou připojená; jejich hodnoty jsou
  stále označené jako demonstrační.
- YAML export používá `pvdashboard-config` v5 a zálohuje aktuální Wi-Fi/IP
  konfiguraci, ale zatím ne celý seznam známých Wi-Fi sítí ani jejich
  `autoConnect` příznaky.
