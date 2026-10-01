# Architektura

## Přehled

~~~text
GoodWe UDP ───────┐
AZRouter HTTP ────┤
Weather HTTPS ────┤
BME280 I²C ───────┤
MAX17048 I²C ─────┼─> DataModel ─> Screen ─> DisplayWorker ─> DisplayManager ─> EpaperDisplay
CC1101 / RF ──────┘       ^          ^              └──────────────> DisplayPreview
                           |          |
Joystick ───────────────> NavigationController ─> ScreenManager
Telefon ─> WebServer ────┘
                 ├─> ConfigManager/NVS
                 └─> OtaManager/Update
~~~

`DashboardApp` sestavuje moduly, plánuje periodické operace a přenáší události
mezi WebUI, vstupy, datovými zdroji a displejem.

## Moduly

- **src/app**: životní cyklus a plánování,
- **src/data**: normalizovaný model a stav zdrojů,
- **src/integrations**: GoodWe, AZRouter, počasí, BME280, MAX17048 a CC1101/RF,
- **src/screens**: renderery obrazovek,
- **src/display**: e-paper abstrakce, worker, preview a refresh politika,
- **src/navigation**: stavový automat Sidebar / Pager / Page,
- **src/input**: fyzický joystick a SET/RESET,
- **src/network**: Wi-Fi, NTP, HTTP API a WebUI,
- **src/config**: NVS konfigurace a YAML backup,
- **src/update**: OTA,
- **src/diagnostics**: výkonová a provozní diagnostika.

## Hlavní smyčka

Současná hlavní smyčka zejména:

1. obsluhuje Wi-Fi, NTP a webový server,
2. načítá pětisměrný joystick a SET/RESET,
3. aktualizuje lokální systémová data,
4. obsluhuje BME280, MAX17048 a RF sensor manager,
5. připravuje display požadavky a dirty regiony,
6. podle intervalů synchronně načítá GoodWe a AZRouter,
7. předává data asynchronnímu DisplayWorkeru,
8. aktualizuje diagnostiku.

GoodWe a AZRouter zůstávají synchronní a při timeoutu mohou krátce zdržet WebUI.
WeatherWorker běží samostatně.

## Displej a refresh

`DisplayWorker` je jediným vlastníkem fyzického e-paperu. Požadavky se slučují a
full požadavek má přednost.

Aktuální strategie:

- start a explicitní full refresh používají čisticí full waveform,
- běžné aktualizace používají differential partial refresh,
- změny focusu používají regionální dirty refresh tam, kde lze oblast bezpečně
  omezit,
- Sidebar může obnovit pouze levý pruh,
- vybrané automatické Home aktualizace obnovují jen oblast datové skupiny
  Weather / Energy / Indoor / Battery / RF,
- přepnutí obrazovky fyzickou navigací může použít full-window differential
  partial refresh místo pomalého čistícího full refreshu.

Počet partial refreshů sám o sobě automatický full refresh nevyvolává.

## BME280 a MAX17048

Oba senzory sdílejí I²C sběrnici:

- SDA GPIO21,
- SCL GPIO22,
- 100 kHz.

BME280 zkouší adresy 0x76 a 0x77. MAX17048 používá 0x36 a čte VCELL, SOC, CRATE,
VERSION a STATUS. Při chybě se při dalším pollu provede nová inicializace.

MAX17048 neurčuje spolehlivě skutečný stav nabíjení. Ten bude případně doplněn
samostatným digitálním signálem z nabíjecího modulu.

## 433 MHz / CC1101

CC1101 sdílí SCK/MOSI s e-paperem, používá vlastní CS a samostatný MISO/GDO piny.
Raw receiver publikuje rozpoznané `RfSensorObservation` do `RfSensorManager`.

`RfSensorManager`:

- během scanu eviduje nalezená podporovaná čidla,
- páruje je podle protocol + sensorId + channel,
- ukládá stabilní `slotId`,
- zapisuje aktuální teplotu, vlhkost a stav baterie do DataModelu,
- po 5 minutách bez paketu označí uložené čidlo jako nedostupné.

RF dekódování je dnes součástí produkčního masteru, přestože dokument
RF_433_RESEARCH.md obsahuje i historické experimenty a otevřené protokoly.

## Navigace a fyzické vstupy

Pětisměrný joystick:

- UP GPIO17,
- DOWN GPIO18,
- LEFT GPIO33,
- RIGHT GPIO16,
- OK GPIO32.

Používá active LOW, interní pull-up a debounce 20 ms. Auto-repeat je záměrně
pouze pro UP/DOWN.

Doplňková tlačítka:

- SET GPIO35,
- RESET GPIO34,
- externí 10k pull-up na 3,3 V,
- GPIO34/35 nemají interní pull-up,
- firmware rozeznává krátké stisky a long-press SET 2,5 s.

## Počasí a paměť

WeatherWorker načítá internetovou předpověď v samostatné FreeRTOS úloze.
DisplayWorker a WeatherWorker sdílejí memory-heavy gate, aby se na ESP32 bez
PSRAM nepřekrývaly velké nároky na interní DRAM.

## Konfigurace

Konfigurace se ukládá v ESP32 NVS namespace **dashboard**.

Aktuální:

- **AppConfig schemaVersion = 12**,
- YAML backup/import: **pvdashboard-config v7**.

YAML obsahuje systém, aktuální Wi-Fi/IP, GoodWe, AZRouter, bazén, počasí,
Home layout a uložená RF čidla. Zatím neobsahuje celý seznam známých Wi-Fi sítí
ani jejich `autoConnect` příznaky.

## Provozní zásady

- hesla a tokeny nesmí být v běžném API ani logu,
- vypnutý modul nesmí zanechat neplatnou obrazovku ani focus,
- neúspěšná OTA nesmí poškodit běžící firmware,
- firmware musí zůstat menší než OTA slot,
- lokální dashboard musí fungovat bez cloudové služby,
- pomalý e-paper refresh nesmí blokovat WebUI,
- automatické datové změny mají pokud možno obnovovat jen nezbytnou část panelu.

## Omezení platformy

ESP32-WROOM-32 nemá PSRAM. Kritická je nejen celková velikost heapu, ale i
největší souvislý blok interní DRAM. Z tohoto důvodu zůstává frontend úsporný a
paměťově náročné operace se serializují.
