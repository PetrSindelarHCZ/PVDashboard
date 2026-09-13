# Home Dashboard ESP32 (7.5" e-Paper 800x480)

Firmware domácího e-paper dashboardu pro **Waveshare ESP32 e-Paper Driver Board** a černobílý panel **Waveshare 7.5" (800x480 px)** vyvíjený v C++ s využitím PlatformIO.

---

## Klíčové vlastnosti
  - `home` – Hlavní souhrn (Venku, Energie/FVE, Uvnitř/čidla, předpověď a stav).
  - `solar` – Podrobný pohled na výrobu FVE, baterii, tok sítí, spotřebu domu a AZ Router (ohřev bojleru).
  - `pool` – Sledování bazénu (teploty, pH, chlor, stav filtrace/ohřevu).
  - `weather` – Předpověď počasí a výhled.
  - `diagnostics` – Systémová diagnostika ESP32, paměť RAM, Wi-Fi signál, stav spojení GoodWe UDP & AZ Router REST API.
  - `GET /api/status`
  - `GET /api/screens`
  - `POST /api/screens/{id}/activate`
  - `POST /api/display/refresh`
  - `POST /api/display/full-refresh`
  - `POST /api/system/restart`


### Obnova Wi-Fi přes konfigurační AP

Pokud se ESP32 po startu nepřipojí k uložené Wi-Fi do 8 sekund, vytvoří dočasnou síť:

- SSID: `Dashboard-Setup`
- Heslo: `dashboard`
- Konfigurační stránka: `http://192.168.4.1/`

Připoj se telefonem nebo počítačem k této síti. V části **Konfigurace Wi-Fi** použij **Vyhledat okolní Wi-Fi**, vyber nalezené SSID nebo ho zadej ručně, doplň heslo a ulož je. ESP32 údaje zapíše do paměti a restartuje se. Při běžném provozu se konfigurační AP nevysílá.
---

## Struktura projektu
```text
PVDashboard/
├── platformio.ini
├── include/
│   ├── AppConfig.h
│   └── Version.h
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── DashboardApp.h
│   │   └── DashboardApp.cpp
│   ├── display/
│   │   ├── IDisplay.h
│   │   ├── EpaperDisplay.h
│   │   ├── EpaperDisplay.cpp
│   │   ├── DisplayManager.h
│   │   └── DisplayManager.cpp
│   ├── screens/
│   │   ├── IScreen.h
│   │   ├── ScreenManager.h
│   │   ├── ScreenManager.cpp
│   │   ├── HomeScreen.h / .cpp
│   │   ├── SolarScreen.h / .cpp
│   │   ├── PoolScreen.h / .cpp
│   │   ├── WeatherScreen.h / .cpp
│   │   └── DiagnosticsScreen.h / .cpp
│   ├── data/
│   │   ├── DataSourceStatus.h
│   │   ├── DataPoint.h
│   │   ├── DataModel.h
│   │   └── DataModel.cpp
│   ├── network/
│   │   ├── WifiManager.h / .cpp
│   │   ├── TimeService.h / .cpp
│   │   └── WebServer.h / .cpp
│   └── config/
│       ├── ConfigSchema.h
│       └── ConfigManager.h / .cpp
└── docs/
    ├── Dashboard_VSCode_Start.md
    └── PV_DASHBOARD_SPEC.md
```
