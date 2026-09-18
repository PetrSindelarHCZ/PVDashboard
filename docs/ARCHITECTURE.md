# Architektura

## Přehled

~~~text
GoodWe UDP ───────┐
AZRouter HTTP ────┼─> DataModel ─> Screen ─> DisplayWorker ─> DisplayManager ─> EpaperDisplay
Weather HTTPS ────┘       ^          ^              └──────────────> DisplayPreview
                           |          |
Telefon ─> WebServer ─> NavigationController ─> ScreenManager
                 ├─> ConfigManager/NVS
                 └─> OtaManager/Update
~~~

DashboardApp sestavuje moduly, plánuje periodické operace a přenáší události
mezi WebUI, datovými zdroji a displejem.

## Moduly

- **src/app**: životní cyklus a plánování.
- **src/data**: normalizovaný model a stav zdrojů.
- **src/integrations**: protokoly GoodWe, AZRouteru a poskytovatelů počasí.
- **src/screens**: čisté renderery bez síťové komunikace a jejich navigační layouty.
- **src/display**: abstrakce displeje, asynchronní worker, preview a refresh politika.
- **src/navigation**: společný stavový automat Sidebar / Pager / Page a geometrická navigace.
- **src/network**: Wi-Fi, NTP, HTTP API, virtuální joystick a vložené WebUI.
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

WeatherWorker načítá internetovou předpověď v samostatné FreeRTOS úloze.
Konkrétní klient je vybrán přes IWeatherProvider; Open-Meteo a MET Norway proto
publikují stejný WeatherData. Worker udržuje cache podle lokality a může obsloužit
nejvýše osm nakonfigurovaných míst. HTTPS klienti používají společný seznam
důvěryhodných kořenových certifikátů.

DisplayWorker a WeatherWorker sdílejí memory-heavy gate. Display jej drží během
inicializace/renderu včetně tvorby preview a WeatherWorker během HTTPS/TLS fetchu.
Tím se na ESP32 bez PSRAM nepřekrývají dva největší nároky na souvislou interní
DRAM. Lifecycle workeru je řízený: při vypnutí počasí se task bezpečně ukončí,
uvolní cache/mutex/stack a při opětovném zapnutí se vytvoří znovu.

## Požadavky na souběh

Pro současné worker úlohy platí:

- displej obsluhuje právě jedna úloha,
- před vykreslením vznikne konzistentní snímek DataModel,
- String ani stav zdrojů se nesmí číst současně se zápisem bez synchronizace,
- více refresh požadavků se slučuje do jednoho,
- plný požadavek nesmí být přepsán pozdějším částečným,
- render/preview a Weather TLS se nesmí překrýt,
- vypnutí WeatherWorkeru nesmí použít násilný `vTaskDelete()` uprostřed TLS operace,
- stav display úlohy je dostupný přes `/api/status`.

Publikované stavy displeje jsou **stopped**, **initializing**, **idle**, **queued**, **rendering_partial**,
**rendering_full** a **error**.

## Časování

Polling GoodWe a AZRouteru je konfigurovatelný a výchozí interval je 10 sekund.
Obraz se automaticky aktualizuje přibližně jednou za minutu a při změně
dostupnosti zdroje. Požadavky vzniklé krátce po sobě se slučují v pětisekundovém
okně.

Přepnutí obrazovky vždy vyžádá čisticí plnou obnovu. Běžné aktualizace stejné
obrazovky jsou částečné; počet částečných obnov už automatickou plnou obnovu
nevyvolává.

## Obrazovky a navigace

ScreenManager registruje moduly dynamicky podle konfigurace:

- `home` a `diagnostics` jsou vždy dostupné,
- `solar` je dostupná, pokud je zapnutý GoodWe nebo AZRouter,
- `pool` je dostupná pouze při `pool.enabled=true`,
- `weather` a hodinové weather obrazovky jsou dostupné pouze při
  `weather.enabled=true`.

Když se právě aktivní dynamická obrazovka vypne, aplikace se vrátí na Home a
NavigationController synchronizuje sidebar/focus s novou sadou obrazovek.

NavigationController má tři oblasti: **Sidebar**, obecný **Pager** a **Page**.
Každý renderer poskytuje aktuální `NavigationLayout` jako sadu focusovatelných
obdélníků. Sousedi se odvozují z geometrie, takže budoucí konfigurovatelný layout
nemusí mít ručně psaný navigační graf. Weather používá Pager pro lokality;
dočasná volba lokality na e-inku nemění persistentní aktivní lokalitu Home.


## Konfigurace a provozní zásady

Konfigurace se ukládá do ESP32 NVS v namespace **dashboard**. Zahrnuje systém,
síť, známé Wi-Fi, GoodWe, AZRouter, bazén a počasí. Dynamické moduly se po změně
konfigurace registrují nebo odregistrují za běhu a NavigationController se
následně synchronizuje.

Známé Wi-Fi sítě mají SSID, heslo a persistentní autoConnect. Ruční Odpojit
nastaví autoConnect=false; ruční Připojit síť znovu povolí. Pokud není dostupná
žádná povolená známá síť, zařízení udržuje recovery AP **Dashboard-Setup**.

Aktuální AppConfig schemaVersion je 6. YAML záloha používá vlastní formát
**pvdashboard-config v5** a obsahuje systém, aktuální Wi-Fi/IP, GoodWe,
AZRouter, bazén a počasí včetně lokalit. Zatím neobsahuje celý seznam známých
Wi-Fi sítí ani jejich autoConnect příznaky.

Základní provozní pravidla:

- hesla a tokeny se nesmí objevit v běžném API ani logu,
- vypnutý modul nesmí zanechat obrazovku ani neplatný navigační focus,
- neúspěšná OTA nesmí poškodit běžící firmware,
- firmware musí zůstat menší než jeden OTA slot,
- lokální dashboard musí fungovat bez cloudové služby,
- pomalý e-paper refresh nesmí blokovat WebUI.

## Omezení platformy

Deska má přibližně 4 MB flash a nemá PSRAM. Partition layout `min_spiffs.csv`
poskytuje dva OTA app sloty po **1 966 080 B (1,875 MiB)**. Skutečná velikost
firmware se kontroluje při každém release; procento využití se proto v této
architektuře záměrně nefixuje na jedno historické číslo.

Do návrhu stále nepatří velký frontend framework, filesystem s duplicitními
assety ani rozsáhlé fonty bez kontroly výsledné velikosti. Pro paměťově náročné
operace je nutné počítat nejen s celkovým free heapem, ale i s největším
souvislým blokem interní DRAM.