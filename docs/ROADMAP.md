# Roadmapa

Roadmapa navazuje na stabilizovaný základ verze 0.1.4. Prioritou je provozní stabilita; další datové zdroje a funkce následují
až po odstranění blokování WebUI.

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
- [ ] ověřit instalaci novějšího GitHub release se správným SHA-256,
- [x] zdokumentovat návrat ke známé funkční verzi,
- [x] hlídat velikost firmware vůči 1 310 720 B OTA partition.

Ověření na zařízení 15. 9. 2026: platný ruční upload o velikosti 1 152 560 B
prošel s odpovídajícím SHA-256 a po restartu zůstala zachována konfigurace.
Firmware s poškozeným bajtem uvnitř obrazu skončil HTTP 400 na neshodě SHA-256.
Stejně byly bez restartu odmítnuty chybějící checksum a obraz o velikosti
1 310 721 B. GitHub OTA stáhla release 0.1.3, při úmyslně chybném SHA-256 jej
odmítla před aktivací oddílu a běžící verze 0.1.4 pokračovala. Platnou instalaci
z GitHubu ponecháváme na ověření s novějším release.

Akceptace:

- [x] běžící firmware přežije ověřené chybové OTA scénáře,
- [x] stejný commit sestaví stejné hlavní verze nástrojů a knihoven,
- [ ] publikovaný release obsahuje firmware, manifest a kontrolní součet.

## P6 — reálná data dalších obrazovek

Rozhodnout, zda jsou další prioritou:

1. počasí z meteorologického API,
2. vnitřní teploty a CO2,
3. bazénová čidla a technologie.

Do té doby mají být demonstrační hodnoty v dokumentaci a WebUI jasně označené.
Pevný text typu „Vše v pořádku“ nesmí působit jako reálně změřený stav.

## P7 — testy a údržba

- host-side testy GoodWe CRC, délky rámce a mapování registrů,
- testy variant a chybných JSON odpovědí AZRouteru,
- testy validace konfigurace a porovnání verzí,
- automatický build a kontrola velikosti firmware,
- aktualizace README a changelogu při každém release.

## Doporučené pořadí nejbližší práce

1. P0: uzavřít současný vzhled a refresh strategii.
2. P1: timeouty, fail-fast a backoff.
3. P2: displej ve vlastní úloze a stav refreshu ve WebUI.
4. P3: Sjednotit UI: stavové záhlaví, levé ikonové menu a odstranění zápatí.
5. P5: ověřit OTA a připnout toolchain.
6. P4: 24hodinový test panelu a doladění refresh politiky.
7. P6 a P7 podle zvolených dalších datových zdrojů.
