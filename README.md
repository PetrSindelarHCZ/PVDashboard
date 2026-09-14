# PVDashboard

Lokální domácí dashboard pro **Waveshare ESP32 e-Paper Driver Board** a černobílý
7,5" panel 800 × 480 px. Firmware zobrazuje data z měniče GoodWe a AZRouteru,
poskytuje mobilní WebUI, konfiguraci přes NVS, recovery Wi-Fi AP a OTA aktualizaci.

Aktuální firmware: **0.1.3**. Pracovní strom obsahuje změny připravované pro další
verzi, zejména diagnostiku výkonu, upravenou obnovu panelu a sjednocený vzhled
obrazovek.

## Aktuální funkce

- obrazovky **home**, **solar**, **pool**, **weather** a **diagnostics**,
- GoodWe GW10K-ET přes Modbus RTU zapouzdřený v UDP na portu 8899,
- AZRouter přes HTTP endpointy **/api/v1/power**, **/api/v1/status** a **/api/v1/devices**,
- mobilní WebUI pro přepínání obrazovek, refresh, konfiguraci a OTA,
- NTP s časovou zónou pro Českou republiku,
- recovery AP **Dashboard-Setup**, pokud se zařízení nepřipojí k uložené Wi-Fi,
- měření dob hlavní smyčky, HTTP, integrací a e-paper refreshů.

Hodnoty počasí, vnitřních čidel a bazénu jsou zatím demonstrační. Obrazovky je
proto potřeba chápat jako připravené rozhraní pro budoucí zdroje dat.

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

## REST API

| Metoda | Endpoint | Účel |
| --- | --- | --- |
| GET | /api/status | Stav systému, zdrojů a výkonnostní metriky |
| GET | /api/screens | Seznam obrazovek |
| POST | /api/screens/{id}/activate | Aktivace obrazovky |
| POST | /api/display/refresh | Rychlá částečná obnova |
| POST | /api/display/full-refresh | Čisticí plná obnova |
| POST | /api/system/restart | Restart ESP32 |
| POST | /api/wifi/config | Uložení Wi-Fi konfigurace |
| GET | /api/wifi/scan | Vyhledání Wi-Fi sítí |
| POST | /api/config/sources | Uložení konfigurace datových zdrojů |
| GET | /api/update/check | Kontrola GitHub release |
| POST | /api/update/github | Instalace release firmware |
| POST | /api/update | Ruční upload firmware |

## Dokumentace

- [Aktuální specifikace](docs/PV_DASHBOARD_SPEC.md)
- [Architektura a datové toky](docs/ARCHITECTURE.md)
- [Panel a refresh strategie](docs/DISPLAY.md)
- [GoodWe a AZRouter](docs/FVE_INTEGRATION_HANDOFF.md)
- [Měření odezvy](docs/PERFORMANCE.md)
- [Další postup](docs/ROADMAP.md)

Výsledky měření z 14. 9. 2026 jsou historické snímky konkrétních testů:

- [měření s nedostupnými zdroji](docs/PERFORMANCE_RESULTS_2026-09-14.md),
- [měření s funkčními simulacemi](docs/PERFORMANCE_SOURCES_ONLINE_2026-09-14.md).

## Známá omezení

- Refresh displeje a čtení datových zdrojů běží synchronně v hlavní smyčce.
  WebUI proto během e-paper operace nebo jednotlivého síťového timeoutu nemusí
  odpovídat; zdroje už používají kratší timeouty a omezený backoff.
- WebUI může spustit další periodický statusový požadavek, i když předchozí běží.
- OTA je implementované, ale celý upgrade a chybové scénáře ještě nejsou
  provozně ověřené.
- Závislosti a platforma v platformio.ini nejsou připnuté na přesné verze.
- Firmware využívá přibližně 86,8 % OTA partition; velikost je nutné hlídat.