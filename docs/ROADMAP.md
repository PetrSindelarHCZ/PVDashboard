# Roadmapa

> Stav aktualizován 18. 9. 2026. Rychlý přehled aktuálního masteru je v
> [PROJECT_STATUS.md](PROJECT_STATUS.md).

Původní etapy P0–P6 zachovávají historii stabilizace projektu. Většina jejich
kritických bodů je už hotová; aktuální priority jsou dlouhodobé ověření refresh
politiky, testy, fyzický joystick a reálná čidla.

## P0 — uzavřít současný základ

- [x] dokončit vizuální kontrolu všech pěti obrazovek,
- [x] provést krátký test částečných a plných refreshů,
- [x] aktualizovat číslo verze,
- [x] rozdělit současné změny do logických commitů,
- [x] vytvořit nový známý funkční bod před změnami vzhledu.

Výsledek: reprodukovatelný firmware a dokumentovaný výchozí stav.

## P1 — odolnost datových zdrojů

### AZRouter

- [x] nastavit samostatný connect timeout,
- [x] po selhání /api/v1/power neprovádět další dva dotazy,
- [x] plánovat další pokus od dokončení předchozího,
- [x] přidat backoff s omezeným maximem,
- [x] po úspěchu obnovit běžný interval.

### GoodWe

- [x] zkrátit blokování při nedostupnosti,
- [x] přidat backoff po celé neúspěšné sadě UDP pokusů,
- [x] zahazovat odpovědi z jiného hostu nebo portu,
- [x] zachovat poslední platná data a samostatně zobrazit jejich stáří.

Implementace je hotová. Online smoke test 14. 9. 2026 potvrdil GoodWe přibližně
17 ms, AZ endpointy 35 až 39 ms a odezvu /api/status s mediánem 39 ms.

Při současném výpadku obou zdrojů byly naměřeny stabilní limity 1 406 ms pro
GoodWe a 407 ms pro AZRouter. Backoff po dalších chybách vzrostl z 40 na 80 s.
Všech 55 požadavků /api/status uspělo; medián byl 40,6 ms, p95 125,9 ms a
maximum 1 480,8 ms během pollingu zdrojů. Další maximum 1 797,4 ms způsobil
souběžný partial refresh displeje.

Akceptace:

- [x] výpadek obou zdrojů nezpůsobí dlouhé série timeoutů,
- [x] WebUI při výpadku běžně odpoví do 500 ms,
- [x] po návratu zdroje se komunikace automaticky obnoví.

Po řízeném zastavení společného lokálního simulátoru přešly oba zdroje
do stavu nedostupné za 10,7 s. Po opětovném spuštění simulátoru se GoodWe
i AZRouter obnovily automaticky bez změny konfigurace. Následné měření během
12 s zaznamenalo dva GoodWe cykly a jeden AZRouter cyklus, což potvrzuje návrat
k běžnému desetisekundovému intervalu.
## P2 — neblokující displej a WebUI

- [x] zavést jednu FreeRTOS úlohu vlastnící displej,
- [x] posílat refresh požadavky frontou,
- [x] vytvořit konzistentní snímek DataModel pro renderer,
- [x] slučovat opakované požadavky a zachovat prioritu plného refreshu,
- [x] publikovat stav displeje v /api/status,
- [x] v WebUI rozlišit přijetí příkazu a dokončení fyzického refreshu,
- [x] zakázat souběžné updateStatus požadavky a přidat klientský timeout.

Akceptace:

- [x] /api/status odpovídá i během 7,3sekundové plné obnovy,
- [x] tlačítka nejdou nechtěně spustit vícekrát,
- [x] WebUI zobrazí queued, rendering a dokončení.

Ověření na zařízení: během full refreshu trvajícího 7,1 s uspělo všech 70
statusových požadavků; medián byl 54,5 ms, p95 87,2 ms a maximum 142,8 ms.
Fronta během partial refreshu zachovala následný full požadavek a vykreslila
nejnovější obrazovku. Rezerva zásobníku display tasku byla přibližně 6 kB.

## P3 — Další krok — sjednocení UI e-paper obrazovek

Platí pro Domov, FVE, Bazén, Počasí a Diagnostiku.

### Společné záhlaví

- [x] odstranit název obrazovky ze záhlaví,
- [x] vlevo zobrazit standardní Wi-Fi symbol s oblouky ve čtyřech úrovních
  podle RSSI; při odpojení použít přeškrtnutý symbol,
- [x] vedle Wi-Fi umístit stavové ikonky označené GW a AZ pro GoodWe a AZRouter,
- [x] dostupnost převzít z existujících výsledků komunikace a na černobílém
  panelu ji rozlišit fajfkou a křížkem; bez Wi-Fi označit oba zdroje jako nedostupné,
- [x] zachovat datum a čas vpravo.

### Levé postranní menu

- [x] pod záhlavím vytvořit společný levý pruh široký přibližně 60 px,
- [x] rovnoměrně rozmístit ikony obrazovek: domeček (Domov), solární panel
  (FVE), vlnky (Bazén), slunce za mrakem (Počasí) a ozubené kolečko (Diagnostika),
- [x] aktivní obrazovku zvýraznit bílou ikonou na černém zaobleném pozadí;
  ostatní ikony vykreslit černě na bílém,
- [x] menu používat jako přehled obrazovek a indikaci aktuálního výběru;
  přepínání zachovat přes stávající WebUI.

### Obsah a odstranění zápatí

- [x] odstranit spodní pruh na všech pěti obrazovkách,
- [x] uvolněných 50 px využít pro obsah a upravit výšky karet a rozestupy,
- [x] posunout obsahové karty doprava a přizpůsobit jejich šířky postrannímu menu,
- [x] IP adresu a číselné RSSI zobrazovat pouze na stavové obrazovce Diagnostika,
- [x] společné vykreslování soustředit do ScreenStyle.h a upravit volání
  ve všech pěti rendererech.

### Obnova a ověření

- [x] napojit indikátory na existující mechanismus obnovy displeje,
- [x] změny síly Wi-Fi zobrazovat při běžném překreslení, aby drobné kolísání
  RSSI nevyvolávalo další obnovy e-paperu,
- [x] sestavit firmware a zkontrolovat jeho velikost,
- [x] vizuálně ověřit všech pět obrazovek: čitelnost ikon, odstupy od data
  a času, rozložení karet a odstranění zápatí,
- [x] ověřit zvýraznění aktivní ikony při přepínání přes WebUI,
- [x] ověřit indikátory při výpadku a obnovení Wi-Fi i jednotlivých integrací.

Ověření na zařízení 14. 9. 2026: všech pět obrazovek postupně dokončilo
plný refresh se správným aktivním ID. Jednotlivé obnovy trvaly 7,06 až 7,09 s.
Stavové indikátory reagovaly na řízený výpadek obou integrací a po automatickém
zotavení zobrazily GoodWe i AZRouter znovu jako dostupné.
Výsledek: jednotné záhlaví se stavem připojení, levé ikonové menu a větší
prostor pro obsah bez zápatí.

## P4 — dokončit refresh politiku

- [ ] odstranit nepoužívané fullRefreshIntervalMinutes ze schématu,
- [x] nevynucovat plný refresh podle počtu částečných obnov,
- [ ] dlouhodobě ověřit politiku: full při startu, přepnutí obrazovky a ručním
  požadavku; ostatní obnovy partial,
- [ ] teprve potom vyhodnotit lokální refresh data/času.

Ověření na zařízení 14. 9. 2026: šest po sobě vyžádaných partial refreshů
zvýšilo jejich čítač ze 4 na 10, zatímco čítač full refreshů zůstal na 1.
Následné přepnutí z Domova na Solar provedlo full refresh a zvýšilo čítač na 2.

Akceptace:

- po 24 hodinách není viditelný progresivní ghosting,
- kontrast tmavého záhlaví a zvýraznění aktivní ikony menu zůstává rovnoměrný,
- přepnutí obrazovky vždy odstraní předchozí rozložení.

## P5 — OTA a reprodukovatelné sestavení

- [x] připnout verzi PlatformIO Core, platformy, frameworku, nástrojů a knihoven,
- [x] ověřit ruční OTA platným firmwarem a povinným SHA-256,
- [x] ověřit odmítnutí chybějícího SHA-256, poškozeného a příliš velkého souboru,
- [x] ověřit stažení GitHub release a odmítnutí nesprávného SHA-256,
- [x] ověřit instalaci novějšího GitHub release se správným SHA-256,
- [x] zdokumentovat návrat ke známé funkční verzi,
- [x] hlídat velikost firmware vůči aktuálnímu OTA slotu **1 966 080 B**.

Ověření na zařízení 15. 9. 2026: platný ruční upload o velikosti 1 152 560 B
prošel s odpovídajícím SHA-256 a po restartu zůstala zachována konfigurace.
Firmware s poškozeným bajtem uvnitř obrazu skončil HTTP 400 na neshodě SHA-256.
Stejně byly bez restartu odmítnuty chybějící checksum a obraz o velikosti
1 310 721 B. Tento konkrétní test proběhl ještě s původním 1,25MiB OTA slotem;
aktuální `min_spiffs.csv` používá dva sloty po **1 966 080 B**. GitHub OTA stáhla release 0.1.3, při úmyslně chybném SHA-256 jej
odmítla před aktivací oddílu a běžící verze 0.1.4 pokračovala. Release v0.1.4 obsahuje firmware, SHA-256 a manifest s commitem 46c721b.
GitHub OTA tohoto release skončila HTTP 200, restartovala zařízení a zachovala
konfiguraci zdrojů.

Akceptace:

- [x] běžící firmware přežije ověřené chybové OTA scénáře,
- [x] stejný commit sestaví stejné hlavní verze nástrojů a knihoven,
- [x] publikovaný release obsahuje firmware, manifest a kontrolní součet.

## P6 — reálná data dalších obrazovek

Priorita byla stanovena takto:

1. počasí z meteorologického API,
2. vnitřní teploty a CO2,
3. bazénová čidla a technologie.

- [x] přidat společný model aktuálního počasí, čtyřdenní předpovědi a
  hodinových bodů větru a srážek,
- [x] načítat Open-Meteo mimo hlavní smyčku, s timeouty a omezenou frekvencí,
- [x] uložit provider, souřadnice a interval do NVS,
- [x] umožnit ve WebUI ruční zadání souřadnic i vyhledání názvu místa;
  výchozí místo je Praha,
- [x] při nedostupnosti počasí nezobrazovat demonstrační hodnoty,
- [x] zobrazit u aktuálního počasí a čtyřdenní předpovědi výrazné vektorové
  symboly pro jasno, oblačnost, mlhu, déšť, sníh a bouřku,
- [x] zbývající hodnoty vnitřních čidel a bazénu označit jako demonstrační,
- [x] oddělit poskytovatele společným rozhraním a umožnit jejich volbu ve WebUI,
- [x] přidat podobrazovku s hodinovou předpovědí pro každý ze čtyř dnů,
  s výběrem dne ve WebUI, ikonami, teplotou, větrem a srážkami,
- [x] prověřit veřejná data ČHMÚ; současná distribuce je založena na regionálních
  dávkových souborech, proto před adaptérem doplnit bezpečný výběr správného souboru,
- [ ] implementovat adaptér veřejných dat ČHMÚ a ověřit jeho paměťové nároky,
- [x] implementovat MET Norway Locationforecast s identifikačním User-Agent,
  HTTPS, cache a požadovanou atribucí,
- [x] doplnit společnou správu důvěryhodných CA certifikátů pro internetové
  providery,
- [ ] po připojení hardware načítat vnitřní teplotu, vlhkost a tlak z BME280.

Ověření 15. 9. 2026: Forecast API pro čtyři dny a 96 hodin vrátilo pro testovací
místo odpověď 4 956 B. Firmware ukládá pouze čtyři denní a osm tříhodinových
bodů prvního dne. JavaScript WebUI prošel kontrolou syntaxe a release build v tehdejším partition layoutu
využíval 31,0 % RAM a 89,7 % tehdejšího OTA slotu. Na zařízení načetl Open-Meteo pro
Český Brod, konfigurace přežila restart a 12 požadavků /api/status mělo při
online GoodWe i AZRouteru odezvu 30–70 ms, průměrně 47,2 ms.
Ověření MET Norway 15. 9. 2026: odpověď Locationforecast Compact pro testovací
místo měla 40 265 B. Parser ukládá jen pole potřebná pro společný model a zachází
s pravděpodobností srážek jako s volitelnou hodnotou. Release build po přidání
společného ověřeného TLS a druhého provideru využívá 31,0 % RAM a 91,4 % OTA
partition. Na zařízení zůstalo přibližně 160 kB volné haldy; MET Norway, GoodWe
i AZRouter byly současně dostupné. Dvanáct požadavků /api/status při zobrazení
počasí trvalo 64–108 ms, průměrně 83,8 ms.

Ověření hodinové předpovědi 15. 9. 2026: model rozšířen na nejvýše 32 bodů
s datem i časem. Open-Meteo načetl osm tříhodinových bodů pro každý ze čtyř dnů.
MET Norway načetl 21 dostupných bodů; první den je neúplný a vzdálenější body
má API po šesti hodinách. Všechny čtyři podobrazovky byly aktivovány na zařízení,
přepnutí používá plný refresh (přibližně 7,1 s). GoodWe a AZRouter zůstaly
dostupné. Build využíval 31,9 % statické RAM a 92,0 % tehdejšího OTA slotu; při
vykreslování zůstalo přibližně 146 kB volné haldy. JavaScript WebUI prošel
kontrolou syntaxe. Pro větší snímky modelu byly zvětšeny zásobníky display
workeru a weather workeru.

## P7 — testy a údržba

- [ ] host-side testy GoodWe CRC, délky rámce a mapování registrů,
- [ ] testy variant a chybných JSON odpovědí AZRouteru,
- [ ] testy validace konfigurace a porovnání verzí,
- [x] host-side test logiky stabilizace Wi-Fi RSSI,
- [x] automatický release build a kontrola maximální velikosti firmware,
- [x] validace shody release tagu s `FIRMWARE_VERSION`,
- [ ] doplnit samostatný běžný CI test job pro host-side testy,
- [ ] zavést pravidelnou aktualizaci changelogu/release notes podle zvolené release politiky.

Aktuálně je v `tests/native` pouze test `wifi_signal_level.cpp`; integrační
parsery a konfigurace tedy stále potřebují širší automatické pokrytí.

## P8 — zachycené budoucí funkce z projektových diskusí

Podrobné důvody, omezení a původní návrhová rozhodnutí jsou zachovány v
[PROJECT_INTENT_BACKLOG.md](PROJECT_INTENT_BACKLOG.md). Tento oddíl je zkrácený
akční seznam, aby se následující funkce neztratily při úklidu starých chatů.

- [ ] konfigurovatelný layout e-ink obrazovek a editor ve WebUI,
- [x] společná softwarová navigace Sidebar / Pager / Page a virtuální joystick ve WebUI,
- [ ] fyzické připojení pětisměrného joysticku, debounce a long-press/auto-repeat,
- [ ] reálné vnitřní čidlo BME280 a pozdější výběr CO2 senzoru,
- [ ] reálná bazénová čidla; první prototyp počítá s DS18B20 a Wi-Fi uzlem,
- [ ] 433MHz/CC1101 gateway pro vzdálená čidla,
- [ ] dlouhodobá historie KPI, agregace a lokální buffer při výpadku internetu,
- [ ] nezávislá cloudová vrstva pro historii bez závislosti základních funkcí na cloudu,
- [ ] zachovat možnost samostatného hostu/portu GoodWe a AZRouteru i když simulátor běží na jedné IP,
- [ ] Home Assistant ponechat pouze jako volitelné budoucí rozšíření.
- [x] lifecycle WeatherWorkeru + memory-heavy gate mezi Display/TLS ověřeny na zařízení: vypnout/zapnout funguje, všechny lokality se načtou a `SSL - Memory allocation failed` se neopakuje.
- [x] dynamická viditelnost počasí, bazénu a FVE podle `enabled` konfigurace,
- [x] pořadí Weather lokalit se persistuje a používá obecný pager obrazovky,
- [x] serverový e-ink preview je součástí WebUI.


## P9 — dynamické moduly a navigace — HOTOVO 18. 9. 2026

- [x] společný NavigationController pro WebUI a budoucí HW ovladač,
- [x] geometrická navigace focusovatelných prvků,
- [x] obecný Pager a Weather lokality jako podstránky,
- [x] změna pořadí Weather lokalit ve WebUI i na e-inku,
- [x] persistentní a runtime přepínač Bazén,
- [x] nezávislé zapnutí/vypnutí GoodWe a AZRouteru,
- [x] odstranění FVE ze sidebaru při vypnutí obou zdrojů,
- [x] adaptivní FVE/Home layout bez prázdných karet vypnutých modulů,
- [x] dynamická registrace/odregistrace obrazovek a následná synchronizace navigace.

Zůstává pouze drobný UI dluh: stručný stavový panel WebUI zatím u explicitně
vypnutého GoodWe/AZRouteru používá text **Offline** místo **Vypnuto**.

## Doporučené pořadí nejbližší práce

1. **P4:** provést alespoň 24hodinový test současné partial/full refresh politiky
   a podle výsledku odstranit nebo využít `fullRefreshIntervalMinutes`.
2. **P7:** rozšířit host-side testy GoodWe, AZRouteru, konfigurace a verzování.
3. Dokončit drobnou konzistenci WebUI stavu **Vypnuto vs. Offline**.
4. Zvolit finální GPIO a připojit fyzický pětisměrný joystick nad hotový
   NavigationController.
5. **P6:** připojit první reálné čidlo (BME280) a následně bazénový DS18B20 uzel.
6. Teprve potom otevírat větší budoucí celky: editor layoutu, 433MHz gateway,
   historii KPI/cloud a případný ČHMÚ adaptér.
