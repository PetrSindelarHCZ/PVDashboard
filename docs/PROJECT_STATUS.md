# Aktuální stav projektu

> Stav k **1. 10. 2026**. Tento dokument je stručný provozní přehled toho, co je
> v aktuálním `masteru` hotové, co je pouze částečné a co zůstává plánem.

## Základ

- firmware: **1.26.261.1**,
- řídicí deska: Waveshare ESP32 e-Paper Driver Board / ESP32-WROOM-32,
- panel: černobílý 7,5" e-paper 800 × 480,
- dva OTA sloty po **1 966 080 B (1,875 MiB)**,
- konfigurace v NVS, interní **schemaVersion 12**,
- YAML backup/import: **pvdashboard-config v7**,
- základní provoz není závislý na cloudu ani na připojeném vývojovém PC.

## Hotovo a ověřeno

### Displej a WebUI

- [x] asynchronní `DisplayWorker`; e-paper neblokuje WebUI během refreshu,
- [x] serverový náhled e-inku přes `/api/display.bmp`,
- [x] memory-heavy gate mezi e-paper renderem/preview a Weather TLS,
- [x] startovní a explicitní čisticí full refresh,
- [x] differential partial refresh pro běžné změny,
- [x] regionální partial refresh pro vybrané změny navigace a Home dat,
- [x] dirty-region výpočet pro Weather, Energy, Indoor, Battery a RF skupiny,
- [x] více rychle po sobě jdoucích požadavků se slučuje.

### Navigace a fyzické ovládání

- [x] společný `NavigationController` pro WebUI i fyzický joystick,
- [x] oblasti Sidebar / Pager / Page,
- [x] virtuální joystick ve WebUI a ovládání klávesami,
- [x] fyzický pětisměrný joystick:
  - UP GPIO17,
  - DOWN GPIO18,
  - LEFT GPIO33,
  - RIGHT GPIO16,
  - OK GPIO32,
- [x] active LOW, interní pull-up, debounce 20 ms,
- [x] auto-repeat pouze pro UP/DOWN po 450 ms a dále po 140 ms,
- [x] LEFT/RIGHT/OK jsou jednorázové, aby během pomalého e-paper refreshu
  nepřeskakovaly více položek,
- [x] SET GPIO35 a RESET GPIO34 s externím 10k pull-up,
- [x] krátké stisky SET/RESET a long-press SET po 2,5 s jsou zpracované firmwarem,
- [x] pinout byl ověřen podle skutečného fyzického zapojení.

### Dynamická viditelnost modulů

- [x] počasí, bazén, GoodWe a AZRouter lze zapínat/vypínat bez ztráty konfigurace,
- [x] FVE existuje, pokud je aktivní GoodWe nebo AZRouter,
- [x] FVE pager Přehled / GoodWe / AZRouter se přizpůsobuje aktivním zdrojům,
- [x] navigace se po dynamické registraci/odregistraci synchronizuje.

### Počasí

- [x] Open-Meteo a MET Norway,
- [x] HTTPS s validací CA,
- [x] samostatný WeatherWorker,
- [x] až 8 lokalit,
- [x] vyhledávání, pořadí, aktivní lokalita a pager,
- [x] čtyřdenní předpověď a hodinové obrazovky,
- [x] cache podle lokality a bezpečný runtime lifecycle workeru.

### Wi-Fi, čas a konfigurace

- [x] více známých Wi-Fi sítí,
- [x] recovery AP `Dashboard-Setup`,
- [x] DHCP i statická konfigurace,
- [x] mDNS `dashboard.local`,
- [x] NTP a časová zóna,
- [x] export/import YAML `pvdashboard-config v7`,
- [x] export/import Home layoutu,
- [x] export/import uložených RF čidel,
- [x] tovární reset.

### GoodWe a AZRouter

- [x] GoodWe GW10K-ET přes Modbus RTU/UDP,
- [x] AZRouter přes lokální HTTP API,
- [x] timeouty, fail-fast a exponenciální backoff,
- [x] zachování posledních platných dat,
- [x] AZRouter podporuje anonymní režim i volitelnou autentizaci,
- [x] konfigurace má explicitní `authEnabled`,
- [x] při vypnuté autentizaci se credentials nepoužívají,
- [x] device-level AZRouter data se na master dashboardu nezobrazují,
- [x] vývojový simulátor je v samostatném repozitáři `Dashboard.DeviceSimulator`.

### BME280

- [x] společná I²C sběrnice SDA GPIO21 / SCL GPIO22,
- [x] detekce 0x76/0x77,
- [x] teplota, vlhkost a tlak v `InsideData`,
- [x] hodnoty jsou dostupné standardní Home kartě i vlastním KPI prvkům,
- [x] fyzický modul je zapojený a v reálném provozu poskytuje data.

### MAX17048 / baterie

- [x] MAX17048 na I²C adrese 0x36,
- [x] sdílí GPIO21/GPIO22 s BME280,
- [x] čtení napětí VCELL,
- [x] čtení SoC,
- [x] čtení změny SoC přes CRATE,
- [x] čtení alert flags,
- [x] polling po 10 s,
- [x] refresh bateriových Home hodnot nejvýše po 60 s,
- [x] hodnoty jsou dostupné vlastním Home widgetům,
- [ ] skutečný stav „nabíjí se“ zatím není určován; plán je číst CHG/STDBY
  z nabíjecího modulu.

### 433 MHz / CC1101

- [x] CC1101 fyzicky připojený a diagnostikovaný,
- [x] raw ASK/OOK capture,
- [x] dekodéry pro několik ověřených rodin,
- [x] `RfSensorManager` v produkčním masteru,
- [x] scan okolních podporovaných čidel,
- [x] přidání nalezeného čidla do konfigurace,
- [x] stabilní slot ID nezávislé na uživatelském jménu,
- [x] volitelné jméno čidla,
- [x] teplota/vlhkost/battery stav do `DataModel`,
- [x] offline timeout uloženého čidla po 5 minutách,
- [x] RF hodnoty mohou být použity ve vlastních Home widgetech,
- [x] konfigurace RF čidel je součástí YAML backupu v7.

Podrobný výzkum protokolů zůstává v [RF_433_RESEARCH.md](RF_433_RESEARCH.md).

## Částečně hotovo

### Bazén

Konfigurovatelná viditelnost a UI jsou hotové. Hodnoty Pool obrazovky jsou stále
demonstrační; reálný bazénový uzel zatím není připojen.

### Refresh politika

Regionální refresh výrazně omezuje zbytečné blikání, ale dlouhodobý alespoň
24hodinový test ghostingu stále chybí. Konfigurační položka
`DisplayConfig.fullRefreshIntervalMinutes` zůstává definovaná, ale není aktivně
používána jako automatický interval.

### RF výzkum

Produkční správa podporovaných RF čidel je hotová, ale průzkum dalších protokolů
a dlouhodobé rozhodnutí, zda dekódování ponechat přímo v dashboardu nebo přesunout
do samostatné gateway, zůstává otevřené.

## Nejbližší otevřené body

- dlouhodobý test e-paper ghostingu,
- detekce skutečného nabíjení z HW-357/TP4056 CHG/STDBY,
- rozšíření host-side testů,
- reálný bazénový uzel,
- další RF protokoly a případná samostatná RF gateway,
- síťové logování a robustnější dlouhodobá diagnostika GoodWe.

## Dokumentace

- [Architektura](ARCHITECTURE.md)
- [Displej](DISPLAY.md)
- [Integrace](INTEGRATIONS.md)
- [Layouty](LAYOUTS.md)
- [Testování](TESTING.md)
- [433 MHz / CC1101](RF_433_RESEARCH.md)
- [Roadmapa](ROADMAP.md)
