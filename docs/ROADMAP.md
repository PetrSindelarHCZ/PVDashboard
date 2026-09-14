# Roadmapa

Roadmapa navazuje na funkční prototyp 0.1.3 a na změny připravované v pracovním
stromu. Prioritou je provozní stabilita; další datové zdroje a funkce následují
až po odstranění blokování WebUI.

## P0 — uzavřít současný základ

- dokončit vizuální kontrolu všech pěti obrazovek,
- provést krátký test částečných a plných refreshů,
- aktualizovat číslo verze,
- rozdělit současné změny do logických commitů,
- vytvořit nový známý funkční bod před architektonickými změnami.

Výsledek: reprodukovatelný firmware a dokumentovaný výchozí stav.

## P1 — odolnost datových zdrojů

### AZRouter

- nastavit samostatný connect timeout,
- po selhání /api/v1/power neprovádět další dva dotazy,
- plánovat další pokus od dokončení předchozího,
- přidat backoff s omezeným maximem,
- po úspěchu obnovit běžný interval.

### GoodWe

- zkrátit blokování při nedostupnosti,
- přidat backoff po celé neúspěšné sadě UDP pokusů,
- zahazovat odpovědi z jiného hostu nebo portu,
- zachovat poslední platná data a samostatně zobrazit jejich stáří.

Akceptace:

- výpadek obou zdrojů nezpůsobí dlouhé série timeoutů,
- WebUI při výpadku běžně odpoví do 500 ms,
- po návratu zdroje se komunikace automaticky obnoví.

## P2 — neblokující displej a WebUI

- zavést jednu FreeRTOS úlohu vlastnící displej,
- posílat refresh požadavky frontou,
- vytvořit konzistentní snímek DataModel pro renderer,
- slučovat opakované požadavky a zachovat prioritu plného refreshu,
- publikovat stav displeje v /api/status,
- v WebUI rozlišit přijetí příkazu a dokončení fyzického refreshu,
- zakázat souběžné updateStatus požadavky a přidat klientský timeout.

Akceptace:

- /api/status odpovídá i během 7,3sekundové plné obnovy,
- tlačítka nejdou nechtěně spustit vícekrát,
- WebUI zobrazí queued, rendering a dokončení.

## P3 — dokončit refresh politiku

- zapojit fullRefreshIntervalMinutes nebo ho odstranit ze schématu,
- rozhodovat podle času i počtu částečných obnov,
- dlouhodobým testem určit bezpečný počet částečných obnov,
- teprve potom vyhodnotit lokální refresh data/času.

Akceptace:

- po 24 hodinách není viditelný progresivní ghosting,
- kontrast tmavého záhlaví a zápatí zůstává rovnoměrný,
- přepnutí obrazovky vždy odstraní předchozí rozložení.

## P4 — OTA a reprodukovatelné sestavení

- připnout verzi PlatformIO platformy a všech knihoven,
- ověřit ruční OTA platným firmwarem,
- ověřit odmítnutí poškozeného a příliš velkého souboru,
- ověřit GitHub OTA včetně SHA-256,
- zdokumentovat návrat ke známé funkční verzi,
- hlídat velikost firmware vůči 1 310 720 B OTA partition.

Akceptace:

- běžící firmware přežije každý chybový OTA scénář,
- stejný commit sestaví stejné hlavní verze nástrojů a knihoven,
- release obsahuje firmware, manifest a kontrolní součet.

## P5 — reálná data dalších obrazovek

Rozhodnout, zda jsou další prioritou:

1. počasí z meteorologického API,
2. vnitřní teploty a CO2,
3. bazénová čidla a technologie.

Do té doby mají být demonstrační hodnoty v dokumentaci a WebUI jasně označené.
Pevný text typu „Vše v pořádku“ nesmí působit jako reálně změřený stav.

## P6 — testy a údržba

- host-side testy GoodWe CRC, délky rámce a mapování registrů,
- testy variant a chybných JSON odpovědí AZRouteru,
- testy validace konfigurace a porovnání verzí,
- automatický build a kontrola velikosti firmware,
- aktualizace README a changelogu při každém release.

## Doporučené pořadí nejbližší práce

1. P0: uzavřít současný vzhled a refresh strategii.
2. P1: timeouty, fail-fast a backoff.
3. P2: displej ve vlastní úloze a stav refreshu ve WebUI.
4. P4: ověřit OTA a připnout toolchain.
5. P3: 24hodinový test panelu a doladění refresh politiky.
6. P5 a P6 podle zvolených dalších datových zdrojů.