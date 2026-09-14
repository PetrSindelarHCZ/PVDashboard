# Architektura

## Přehled

~~~text
GoodWe UDP ───────┐
                  ├─> DataModel ─> Screen ─> DisplayManager ─> EpaperDisplay
AZRouter HTTP ────┘                    ^
                                       |
Telefon ─> WebServer ─> ScreenManager ─┘
                 ├─> ConfigManager/NVS
                 └─> OtaManager/Update
~~~

DashboardApp sestavuje moduly, plánuje periodické operace a přenáší události
mezi WebUI, datovými zdroji a displejem.

## Moduly

- **src/app**: životní cyklus a plánování.
- **src/data**: normalizovaný model a stav zdrojů.
- **src/integrations**: protokoly GoodWe a AZRouteru.
- **src/screens**: čisté renderery bez síťové komunikace.
- **src/display**: abstrakce displeje a refresh politika.
- **src/network**: Wi-Fi, NTP, HTTP API a vložené WebUI.
- **src/config**: konfigurace uložená v NVS.
- **src/update**: příjem ručně nahraného OTA obrazu.
- **src/diagnostics**: kumulativní měření trvání operací.

Rozdělení hardwarové vrstvy od obrazovek je zachované přes IDisplay. Renderery
pracují pouze s DataModel a kreslicím rozhraním.

## Hlavní smyčka

Současná hlavní smyčka postupně:

1. obslouží Wi-Fi, NTP a jeden krok synchronního webového serveru,
2. aktualizuje systémová data,
3. případně provede celý e-paper refresh,
4. podle intervalů synchronně načte GoodWe a AZRouter,
5. aktualizuje diagnostiku a čeká 20 ms.

Toto uspořádání je jednoduché, ale refresh a timeouty zastaví obsluhu WebUI.
Cílová změna je ponechat WebServer v hlavní smyčce a přesunout displej a později
také síťové polling operace do řízených pracovních úloh.

## Požadavky na souběh

Při zavedení FreeRTOS úloh musí platit:

- displej obsluhuje právě jedna úloha,
- před vykreslením vznikne konzistentní snímek DataModel,
- String ani stav zdrojů se nesmí číst současně se zápisem bez synchronizace,
- více refresh požadavků se slučuje do jednoho,
- plný požadavek nesmí být přepsán pozdějším částečným,
- stav úlohy je dostupný přes /api/status.

Doporučené stavy displeje jsou **idle**, **queued**, **rendering_partial**,
**rendering_full** a **error**.

## Časování

Polling GoodWe a AZRouteru je konfigurovatelný a výchozí interval je 10 sekund.
Obraz se automaticky aktualizuje přibližně jednou za minutu a při změně
dostupnosti zdroje. Požadavky vzniklé krátce po sobě se slučují v pětisekundovém
okně.

Přepnutí obrazovky vždy vyžádá čisticí plnou obnovu. Po pěti běžných částečných
obnovách DisplayManager také vynutí plnou obnovu. Konfigurační časový interval
plné obnovy zatím není zapojený.

## Omezení platformy

Deska má přibližně 4 MB flash a nemá PSRAM. Aktuální build využívá přibližně
1,14 MB z 1 310 720 B dostupných v jedné OTA partition. Do návrhu proto nepatří
velký frontend framework, filesystem s duplicitními assety ani rozsáhlé fonty
bez kontroly výsledné velikosti.