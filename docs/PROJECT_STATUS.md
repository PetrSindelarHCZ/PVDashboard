# Aktuální stav projektu

> Stav k **19. 9. 2026**. Tento dokument je stručný provozní přehled toho, co je
> v aktuálním `masteru` hotové, co je pouze částečné a co zůstává plánem.
> Technické detaily jsou v odkazovaných specializovaných dokumentech.

## Základ

- firmware: **1.26.261.1**,
- řídicí deska: Waveshare ESP32 e-Paper Driver Board / ESP32-WROOM-32,
- panel: černobílý 7,5" e-paper 800 × 480,
- partition layout: `min_spiffs.csv`, dva OTA sloty po **1 966 080 B (1,875 MiB)**,
- primární ovládání a konfigurace: lokální WebUI,
- základní provoz není závislý na cloudu ani na připojeném vývojovém PC.

## Hotovo a ověřeno

### Displej a WebUI

- [x] jednotný vzhled všech hlavních e-ink obrazovek,
- [x] asynchronní `DisplayWorker`; e-paper neblokuje WebUI během refreshu,
- [x] rozlišení stavů `queued`, `rendering_partial`, `rendering_full` a dokončení ve WebUI,
- [x] serverový náhled posledního vyrenderovaného e-inku přes `/api/display.bmp`,
- [x] paměťově úsporný render preview po pruzích,
- [x] společný memory-heavy gate mezi renderem displeje/preview a HTTPS/TLS počasí,
- [x] full refresh při startu, změně obrazovky a ručním požadavku,
- [x] běžné aktualizace stejné obrazovky používají partial refresh,
- [x] počet partial refreshů už automaticky nevynucuje full refresh.

### Navigace

- [x] společný `NavigationController` pro WebUI i budoucí fyzický joystick,
- [x] oblasti **Sidebar / Pager / Page**,
- [x] geometricky odvozený pohyb mezi focusovatelnými prvky bez pevného grafu sousedů,
- [x] virtuální pětisměrný joystick ve WebUI a ovládání klávesami,
- [x] `GET/POST /api/navigation`,
- [x] zvýraznění kurzoru/focusu přímo v e-ink renderu a jeho preview,
- [x] obecný pager pro vícepodstránkové obrazovky,
- [x] Weather pager používá pořadí uložených lokalit a zobrazuje právě volenou lokalitu.

Fyzické GPIO joysticku, debounce, auto-repeat a akce `OK` nad konkrétním
prvkem zatím implementované nejsou.

### Dynamická viditelnost modulů

- [x] **Počasí** lze vypnout bez ztráty konfigurace; odstraní se jeho obrazovky,
  sidebar položka a Home widget a WeatherWorker uvolní runtime prostředky,
- [x] **Bazén** má persistentní přepínač Aktivní; při vypnutí zmizí Pool obrazovka,
  sidebar položka i bazénové informace a změna se projeví za běhu,
- [x] **GoodWe** a **AZRouter** lze zapínat nezávisle,
- [x] FVE obrazovka existuje, pokud je aktivní alespoň jeden z obou zdrojů,
- [x] při vypnutí obou zdrojů se FVE odstraní ze sidebaru; pokud byla právě
  otevřená, aktivuje se Domov,
- [x] FVE obrazovka skládá obsah podle aktivních zdrojů,
- [x] stavové ikony v záhlaví se kreslí pouze pro zapnuté zdroje,
- [x] navigace se po dynamické registraci/odregistraci obrazovek synchronizuje.

Poznámka: stručný stavový panel ve WebUI ještě u vypnutého GoodWe/AZRouteru
používá text „Offline“. Konfigurace, e-ink diagnostika a dynamická viditelnost
už stav `enabled` respektují; jde o drobný UI dluh, ne o problém pollingu.

### Počasí

- [x] Open-Meteo,
- [x] MET Norway Locationforecast,
- [x] HTTPS s ověřováním CA, bez `setInsecure()`,
- [x] samostatný WeatherWorker mimo hlavní smyčku,
- [x] více uložených lokalit, aktuálně nejvýše 8,
- [x] vyhledávání, přidání, výběr a změna pořadí lokalit ve WebUI,
- [x] cache dat podle lokality,
- [x] čtyřdenní předpověď a hodinové podobrazovky,
- [x] pager lokalit na hlavní Weather obrazovce,
- [x] pager nemění persistentní aktivní lokalitu používanou Domovem,
- [x] zobrazení názvu poskytovatele počasí na Domově,
- [x] bezpečné vypnutí a opětovné vytvoření WeatherWorkeru za běhu,
- [x] zařízení otestováno s více lokalitami bez opakování TLS memory allocation chyby.

### Wi-Fi, čas a konfigurace

- [x] více známých Wi-Fi sítí,
- [x] AP+STA recovery režim `Dashboard-Setup`,
- [x] průběžné hledání známých sítí a prioritizace viditelných kandidátů podle RSSI,
- [x] ruční **Odpojit** persistentně zakáže auto-connect daného SSID,
- [x] ruční **Připojit** síť znovu povolí,
- [x] DHCP i statická konfigurace,
- [x] mDNS `dashboard.local`,
- [x] NTP a časová zóna; čas v záhlaví se nezobrazuje před první platnou synchronizací,
- [x] export/import konfigurace v YAML (`pvdashboard-config` v5),
- [x] tovární reset,
- [x] změny počasí, bazénu a energetických zdrojů se aplikují za běhu tam,
  kde není nutný restart síťové vrstvy.

### GoodWe a AZRouter

- [x] GoodWe GW10K-ET přes Modbus RTU/UDP,
- [x] AZRouter přes lokální HTTP API,
- [x] timeouty, fail-fast a exponenciální backoff,
- [x] zachování posledních platných dat a samostatný stav dostupnosti/stáří,
- [x] automatické obnovení pollingu po návratu zdroje,
- [x] samostatný host a port pro každý zdroj,
- [x] vývojový simulátor je v samostatném repozitáři `Dashboard.DeviceSimulator`.

Polling GoodWe a AZRouteru zůstává synchronní v hlavní smyčce; jednotlivý
timeout proto může krátce zvýšit odezvu WebUI.

### OTA a release

- [x] ruční OTA se SHA-256,
- [x] GitHub release OTA,
- [x] odmítnutí poškozeného nebo příliš velkého obrazu,
- [x] připnutý PlatformIO/toolchain a knihovny,
- [x] release workflow vytváří firmware, SHA-256 a manifest,
- [x] tag release musí odpovídat `FIRMWARE_VERSION`,
- [x] limit obrazu je sjednocen na **1 966 080 B** podle aktuální OTA partition.

## Částečně hotovo

### Bazén

Konfigurovatelná viditelnost a UI jsou hotové. Hodnoty Pool obrazovky jsou ale
stále demonstrační; reálný DS18B20/Wi-Fi uzel zatím není připojen.

### Vnitřní prostředí

Firmware má připravenou podporu BME280 přes I²C na GPIO21/GPIO22, automatickou
detekci adres 0x76/0x77, periodický polling a napojení teploty, vlhkosti a tlaku
do `InsideData`. Teplota, vlhkost a tlak se zobrazují ve standardní Home kartě
Uvnitř, jsou dostupné vlastním KPI prvkům a také v `/api/status`.

Fyzické zapojení konkrétního 4pinového modulu ještě není na zařízení ověřené,
proto tato část zůstává částečně hotová. Ložnice a další hodnoty vnitřního
prostředí jsou nadále demonstrační. Měření CO₂ není součástí aktuálního plánu;
jde pouze o možnou budoucí úvahu.

### Fyzické ovládání

Softwarový model navigace a WebUI joystick jsou hotové. Fyzický pětisměrný
ovladač zatím nemá finální GPIO/pinout ani obsluhu tlačítek.

### Refresh politika

Krátké testy a chování fronty jsou ověřené. Stále chybí dlouhodobý alespoň
24hodinový test ghostingu při běžném minutovém provozu. Konfigurační položka
`DisplayConfig.fullRefreshIntervalMinutes` je stále definovaná, ale nepoužívá se.

## Nejbližší otevřené body

Nejbližší práce je dlouhodobý test ghostingu, rozšíření host-side testů,
fyzický joystick a napojení prvních reálných čidel. Úplný budoucí plán je
v [ROADMAP.md](ROADMAP.md).

## Dokumentace

- [Architektura a provozní principy](ARCHITECTURE.md)
- [Displej a refresh strategie](DISPLAY.md)
- [Externí integrace](INTEGRATIONS.md)
- [Testování a diagnostika](TESTING.md)
- [Roadmapa](ROADMAP.md)

Git historie je archivem starších specifikací, měření a jednorázových auditů.
