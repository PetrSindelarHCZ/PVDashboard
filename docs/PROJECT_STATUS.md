# Aktuální stav projektu

> Stav k **20. 9. 2026**. Tento dokument je stručný provozní přehled toho, co je
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

Fyzický pětisměrný joystick je připojen přímo do stejného `NavigationController`
jako WebUI: UP GPIO16, DOWN GPIO17, LEFT GPIO18, RIGHT GPIO32 a OK GPIO33.
Vstupy jsou active LOW s interním `INPUT_PULLUP` a 20ms debounce. Směrová tlačítka
mají auto-repeat po 450 ms a potom po 140 ms; OK zůstává jednorázové. Fyzická
navigace používá rychlý display path: Sidebar a Page focus obnovují pouze
dotčenou oblast, mezikroky negenerují celý WebUI preview a přepnutí obrazovky
používá full-window differential partial refresh místo pomalého čistícího full
refreshu. Akce `OK` nad konkrétním prvkem zatím implementované nejsou.

### Dynamická viditelnost modulů

- [x] **Počasí** lze vypnout bez ztráty konfigurace; odstraní se jeho obrazovky,
  sidebar položka a Home widget a WeatherWorker uvolní runtime prostředky,
- [x] **Bazén** má persistentní přepínač Aktivní; při vypnutí zmizí Pool obrazovka,
  sidebar položka i bazénové informace a změna se projeví za běhu,
- [x] **GoodWe** a **AZRouter** lze zapínat nezávisle,
- [x] FVE obrazovka existuje, pokud je aktivní alespoň jeden z obou zdrojů,
- [x] při vypnutí obou zdrojů se FVE odstraní ze sidebaru; pokud byla právě
  otevřená, aktivuje se Domov,
- [x] FVE obrazovka používá pager **Přehled / GoodWe / AZRouter** podle aktivních zdrojů,
- [x] AZRouter podstránka zobrazuje master L1/L2/L3, vytěžování, energii, HDO, režim, Boost a teplotu jednotky,
- [x] device-level údaje AZRouteru (např. bojler) se na dashboardu nezobrazují,
- [x] stavové ikony v záhlaví se kreslí pouze pro zapnuté zdroje,
- [x] navigace se po dynamické registraci/odregistraci obrazovek synchronizuje.

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
- [x] AZRouter login přes WebUI credentials včetně Bearer token/session cookie a re-loginu po 401/403,
- [x] AZRouter heslo zůstává v NVS a není vraceno přes status API,
- [x] odstraněny startovní demo hodnoty GoodWe/AZRouter; před prvním validním pollingem UI zobrazuje nedostupnost,
- [x] AZRouter rozlišuje platnost master výkonu po fázích, sítě, energií a systémových stavů,
- [x] `status.system.temperature` je diagnostická teplota AZRouter masteru,
- [x] `devices` se dál přijímá, ale jeho výkon/teplota se nepoužívají pro master dashboard,
- [x] vývojový simulátor je v samostatném repozitáři `Dashboard.DeviceSimulator`.

Polling GoodWe a AZRouteru zůstává synchronní v hlavní smyčce; jednotlivý
timeout proto může krátce zvýšit odezvu WebUI.

WebUI používá jeden sdílený 5s `/api/status` poller. Weather/source-status
widgety odebírají jeho snapshot místo vlastních paralelních pollerů; skrytá
záložka polling zastaví. Běžný status je odlehčený, detailní performance a
AZRouter diagnostika jsou dostupné přes `/api/status?details=1`.

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

Pětisměrný ovladač má finální pinout UP=GPIO16, DOWN=GPIO17, LEFT=GPIO18,
RIGHT=GPIO32 a OK=GPIO33. Firmware používá interní pull-upy, active-LOW logiku a 20ms debounce.
Směry podporují auto-repeat; OK je jednorázové. Samostatná
tlačítka SET a RESET zatím nejsou do firmware připojena.

### Refresh politika

Krátké testy a chování fronty jsou ověřené. Stále chybí dlouhodobý alespoň
24hodinový test ghostingu při běžném minutovém provozu. Konfigurační položka
`DisplayConfig.fullRefreshIntervalMinutes` je stále definovaná, ale nepoužívá se.

## Nejbližší otevřené body

Nejbližší práce je dlouhodobý test ghostingu, rozšíření host-side testů,
ověření fyzického joysticku na zařízení a napojení dalších reálných čidel. Úplný budoucí plán je
v [ROADMAP.md](ROADMAP.md).

## Dokumentace

- [Architektura a provozní principy](ARCHITECTURE.md)
- [Displej a refresh strategie](DISPLAY.md)
- [Externí integrace](INTEGRATIONS.md)
- [Testování a diagnostika](TESTING.md)
- [433 MHz / CC1101 – stav průzkumu a další postup](RF_433_RESEARCH.md)
- [Roadmapa](ROADMAP.md)

Git historie je archivem starších specifikací, měření a jednorázových auditů.
