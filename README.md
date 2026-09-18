# PVDashboard

Lokální domácí dashboard pro **Waveshare ESP32 e-Paper Driver Board** a černobílý
7,5" panel 800 × 480 px. Firmware zobrazuje data z měniče GoodWe a AZRouteru,
poskytuje mobilní WebUI, konfiguraci přes NVS, recovery Wi-Fi AP a OTA aktualizaci.

Aktuální firmware: **1.26.261.1**. Obsahuje stabilizované načítání počasí přes HTTPS,
paměťově úsporný náhled e-paperu ve WebUI, diagnostiku výkonu a sjednocený vzhled obrazovek.

## Aktuální funkce

- obrazovky **home**, **solar**, **pool**, **weather** a **diagnostics**,
- GoodWe GW10K-ET přes Modbus RTU zapouzdřený v UDP na portu 8899,
- AZRouter přes HTTP endpointy **/api/v1/power**, **/api/v1/status** a **/api/v1/devices**,
- mobilní WebUI pro přepínání obrazovek, refresh, konfiguraci a OTA,
- NTP s časovou zónou pro Českou republiku,
- recovery AP **Dashboard-Setup**, pokud není nastavené SSID nebo se zařízení nepřipojí k uložené Wi-Fi,
- měření dob hlavní smyčky, HTTP, integrací a e-paper refreshů,
- živé počasí z Open-Meteo nebo MET Norway pro více uložených lokalit,
  čtyřdenní předpověď, hodinový přehled pro vybraný den, pager lokalit na e-inku
  a vyhledání místa ve WebUI,
- ověřené HTTPS pro oba poskytovatele a respektování serverové cache MET Norway.

Počasí načítá samostatná FreeRTOS úloha a při nedostupnosti API se na displeji
nezobrazují náhradní čísla. Hodnoty vnitřních čidel a bazénu jsou zatím
demonstrační a firmware i WebUI je tak označují. Podrobnosti poskytovatelů
a další postup pro ČHMÚ jsou v [WEATHER_PROVIDERS.md](docs/WEATHER_PROVIDERS.md).

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

Pokud se ESP32 nepřipojí k uložené Wi-Fi, vytvoří konfigurační síť
**Dashboard-Setup** s heslem **dashboard**. Nastavení je pak dostupné na
**http://192.168.4.1/**.

## Release a návrat verze

PlatformIO Core je připnuté v release workflow; platforma, framework, nástroje a knihovny jsou
připnuté v **platformio.ini**. Po sestavení připraví validované artefakty tento
příkaz:

~~~powershell
python scripts/prepare-release.py --firmware .pio/build/esp32dev/firmware.bin --output dist --expected-version 1.0.0
~~~

Výstup obsahuje **firmware.bin**, **firmware.bin.sha256** a
**dashboard-manifest.json**. Skript odmítne nesoulad verze a obraz větší než
1 310 720 B. GitHub workflow provádí stejné kontroly a tag **vX.Y.Z** musí
odpovídat **FIRMWARE_VERSION**. Ruční upload ve WebUI vyžaduje vložit 64znakový SHA-256 ze souboru **firmware.bin.sha256** a ověří jej ještě před aktivací oddílu.

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
| GET | /api/status | Stav systému, zdrojů a výkonnostní metriky |
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
| POST | /api/config/sources | Uložení konfigurace energetických zdrojů |
| POST | /api/config/weather | Uložení provideru, souřadnic a intervalu počasí |
| POST | /api/screens/weather-hourly-0/activate | Hodinový přehled prvního dne (indexy 0–3) |
| GET | /api/update/check | Kontrola GitHub release |
| POST | /api/update/github | Instalace release firmware |
| POST | /api/update | Ruční upload firmware s polem `sha256` |

## Dokumentace

- [Aktuální specifikace](docs/PV_DASHBOARD_SPEC.md)
- [Architektura a datové toky](docs/ARCHITECTURE.md)
- [Panel a refresh strategie](docs/DISPLAY.md)
- [GoodWe a AZRouter](docs/FVE_INTEGRATION_HANDOFF.md)
- [Měření odezvy](docs/PERFORMANCE.md)
- [Další postup](docs/ROADMAP.md)
- [Zachycené projektové záměry a neimplementované funkce](docs/PROJECT_INTENT_BACKLOG.md)
- [Audit starých Dashboard chatů před ručním mazáním](docs/CHAT_CLEANUP_AUDIT_2026-09-18.md)

Výsledky měření z 14. 9. 2026 jsou historické snímky konkrétních testů:

- [měření s nedostupnými zdroji](docs/PERFORMANCE_RESULTS_2026-09-14.md),
- [měření s funkčními simulacemi](docs/PERFORMANCE_SOURCES_ONLINE_2026-09-14.md).

## Známá omezení

- Čtení datových zdrojů stále běží synchronně v hlavní smyčce. WebUI proto může
  během jednotlivého síťového timeoutu čekat přibližně 1,8 sekundy.
- E-paper obsluhuje samostatná FreeRTOS úloha; WebUI během partial ani full
  refreshu zůstává dostupné a zobrazuje stav vykreslení.
- Platná ruční i GitHub OTA a chybové scénáře jsou ověřené na zařízení.
- Firmware využívá přibližně 93,7 % OTA partition; release skript i zařízení
  odmítnou obraz větší než 1 310 720 B.
