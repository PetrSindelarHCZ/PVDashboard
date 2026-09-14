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
3. předá připravený snímek dat display workeru,
4. podle intervalů synchronně načte GoodWe a AZRouter,
5. aktualizuje diagnostiku a čeká 20 ms.

DisplayWorker je jediným vlastníkem DisplayManageru a e-paper ovladače. Požadavky
ukládá do chráněného jednopolohového bufferu, slučuje změny a renderuje konzistentní
kopii DataModel v samostatné FreeRTOS úloze. Síťové polling operace zatím zůstávají
v hlavní smyčce a při timeoutu mohou krátce zdržet WebUI.

## Požadavky na souběh

Pro současnou FreeRTOS display úlohu platí:

- displej obsluhuje právě jedna úloha,
- před vykreslením vznikne konzistentní snímek DataModel,
- String ani stav zdrojů se nesmí číst současně se zápisem bez synchronizace,
- více refresh požadavků se slučuje do jednoho,
- plný požadavek nesmí být přepsán pozdějším částečným,
- stav úlohy je dostupný přes /api/status.

Publikované stavy displeje jsou **stopped**, **initializing**, **idle**, **queued**, **rendering_partial**,
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