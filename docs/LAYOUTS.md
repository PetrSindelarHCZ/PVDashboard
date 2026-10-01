# Konfigurovatelné obrazovky

Tento dokument popisuje aktuální implementaci konfigurovatelného Home layoutu
a jeho datové vazby.

## Princip

Firmware nebude implementovat obecný HTML/CSS layout. Každá obrazovka má omezenou,
předem známou množinu widgetů. Layout pouze určuje jejich geometrii a parametry.
Stejný layout musí používat fyzický e-paper, Display Preview i navigační hitboxy.

Základní geometrie widgetu:

- `id` – stabilní identifikátor instance widgetu,
- `type` – podporovaný typ widgetu,
- `x`, `y` – levý horní roh v pixelech,
- `width`, `height` – rozměr v pixelech.

Souřadnice jsou v logickém prostoru displeje 800 × 480.

## Podporovaná sada

Aktuální implementace pokrývá obrazovku Home:

- `weather-card` / `HomeWeatherCard`,
- `energy-card` / `HomeEnergyCard`,
- `indoor-card` / `HomeIndoorCard`.

Home již nevytváří geometrii zvlášť pro renderer a zvlášť pro navigaci.
Obě vrstvy dostávají jeden společný `ScreenLayout`.

Výchozí layout zatím zachovává dnešní automatické rozložení podle toho, zda je
aktivní počasí a FVE. Tím se první krok scénáře D obejde bez změny vzhledu.

## Validace

Layout je persistovaný v NVS, editovatelný ve WebUI a je součástí YAML backupu.
Firmware validuje alespoň:

- widget leží v ploše obsahu a nepřekračuje 800 × 480,
- šířka a výška splňují minimum daného typu,
- na jedné obrazovce není duplicitní `id`,
- počet widgetů nepřekročí pevný limit,
- neznámý typ widgetu se odmítne místo tichého ignorování.


## D2 — persistence a REST API

Home layout je uložen v `AppConfig.display.homeLayout` a v NVS jako blob
`layout_blob`. Starší string `layout_home` se při načtení automaticky migruje.
Pokud `customized=false`, renderer používá moderní výchozí přehled se
samostatnými kartami **PŘEDPOVĚĎ**, **FVE / GOODWE**, **AZROUTER**, **UVNITŘ**,
**BAZÉN** a **STAV SYSTÉMU**. Karty zdrojů se zobrazují podle aktivace
příslušných modulů. Karta předpovědi je záměrně označena jako online
předpověď a uvádí poskytovatele dat, aby nebyla zaměnitelná s fyzickým
venkovním čidlem.

Vlastní editor nadále nabízí původní předdefinované typy Weather / Energy /
Indoor i vlastní widgety. Uložené layouty s `customized=true` nejsou změnou
výchozí Home obrazovky přepsány.

REST rozhraní (D2):

- `GET /api/layout/home` — uložený i právě efektivní layout a limity widgetů,
- `POST /api/layout/home` — validace a uložení vlastního layoutu,
- `POST /api/layout/home/reset` — návrat na automatickou výchozí šablonu.

POST používá JSON, například:

```json
{
  "customized": true,
  "widgets": [
    {
      "id": "weather-card",
      "type": "weather",
      "visible": true,
      "x": 75,
      "y": 63,
      "width": 225,
      "height": 402
    },
    {
      "id": "energy-card",
      "type": "energy",
      "visible": true,
      "x": 315,
      "y": 63,
      "width": 225,
      "height": 402
    },
    {
      "id": "indoor-card",
      "type": "indoor",
      "visible": true,
      "x": 555,
      "y": 63,
      "width": 230,
      "height": 402
    }
  ]
}
```

Firmware odmítá neznámé nebo duplicitní widgety, geometrii mimo obsahovou
plochu, příliš malé widgety a překryv viditelných widgetů.

Aktuální YAML záloha je **pvdashboard-config v7** a obsahuje
`layout.home_json`. Starší zálohy bez layoutu se importují s výchozím
automatickým Home layoutem.


## D3 — grafický editor Home layoutu

WebUI na stránce Obrazovky obsahuje první interaktivní editor Home layoutu.

- editor pracuje s draftem pouze v prohlížeči; NVS se zapisuje až tlačítkem Uložit,
- widget lze tažením přesouvat a rohovými úchyty měnit jeho velikost,
- mřížka je volitelná v krocích 5, 10, 20 a 25 px,
- magnetismus lze vypnout; při zapnutí se souřadnice přichytávají k mřížce
  zarovnané na obsahovou plochu Home,
- editor zobrazuje X/Y/šířku/výšku vybraného widgetu,
- překrývající se widgety jsou označeny jako neplatné a nelze je uložit,
- jednotlivé widgety lze skrýt,
- tlačítko Výchozí používá REST reset z D2,
- tlačítko Zobrazit Home přepne fyzický displej/náhled na Home pouze na výslovný
  požadavek uživatele.


## Custom widgety — první základ

Vedle předdefinovaných Home widgetů může layout obsahovat uživatelský kontejner
s `type: custom`. ID vlastního kontejneru musí začínat `custom-`; na jedné
Home obrazovce může být celkem až 6 widgetů.

Vlastní kontejner má:

- vlastní `title`,
- geometrii na Home stejně jako ostatní widgety,
- až 8 vnitřních elementů,
- vnitřní souřadnice relativně k levému hornímu rohu kontejneru.

Prvních 38 px výšky je rezervováno pro záhlaví karty. Vnitřní elementy tedy
začínají od `y >= 40`.

První elementy:

- `text` — statický text,
- `kpi` — číselná hodnota ze zvoleného datového zdroje,
- `progress` — hodnota vykreslená do rozsahu min/max,
- `sparkline` — mini graf; v první verzi podporuje 24h historii výroby FVE
  a spotřeby domu.

Dynamické elementy používají stabilní klíč `source`, například
`solar.productionPowerW`, `azrouter.routedPowerW`,
`azrouter.routedL1PowerW`, `azrouter.routedEnergyTodayKWh`,
`azrouter.systemTempC`, `weather.outdoorTempC`, `inside.temperatureC`,
`inside.humidityPercent`, `inside.pressureHpa`, `battery.voltageV`,
`battery.socPercent`, `battery.changeRatePercentPerHour`,
`pool.waterTempC` a dynamické RF zdroje navázané na uložené `slotId`.
Katalog podporovaných zdrojů vrací
`GET /api/layout/home` v objektu `customWidget.dataSources`, takže WebUI
nemusí seznam datových vazeb duplikovat.

Vlastní widgety se ukládají ve stejném layout JSON jako předdefinované widgety.
NVS persistence používá blob `layout_blob`; starší D2/D3 string
`layout_home` se při načtení automaticky migruje.

WebUI umí vytvořit nový vlastní kontejner, upravovat jeho vnější geometrii
a po jeho výběru otevře vnořený editor obsahu.

Vnořený editor:

- používá stejnou volbu mřížky 5/10/20/25 px a stejný přepínač magnetismu,
- zarovnává vnitřní prvky od relativního počátku `x=8, y=40`,
- umí přidat `Text`, `KPI`, `Progress` a `Graf`,
- umožňuje drag/resize každého prvku a zároveň přesné zadání X/Y/šířky/výšky,
- u dynamických prvků nabízí zdroje z katalogu firmware,
- u `sparkline` nabízí pouze zdroje s historií,
- u KPI dovoluje změnit popisek, jednotku a počet desetinných míst,
- u progress baru navíc dovoluje nastavit minimum a maximum,
- zvýrazňuje neplatné/překrývající se prvky a blokuje hlavní uložení,
- zapisuje do NVS až při hlavním tlačítku `Uložit`.

Vnější rozměr custom widgetu nelze zmenšit pod prostor potřebný pro jeho
aktuální vnitřní elementy.


### Stylování vlastních prvků

Vlastní elementy mají společné volitelné parametry:

- `fontSize`: `auto` nebo číselná výška **7–64 px**; starší hodnoty
  `small/normal/large` se při importu převádějí na přibližně 16/18/22 px,
- `align`: `left | center | right` — zarovnání textu nebo popisku uvnitř šířky elementu,
- `showLabel`: možnost skrýt popisek u KPI, progress baru a grafu,
- `graphStyle`: `line | bars` pro sparkline.

Výchozí hodnoty zachovávají chování starších custom layoutů:
`fontSize=auto`, `align=left`, `showLabel=true`, `graphStyle=line`.

Dlouhý text, popisek nebo KPI hodnota se na e-paperu ořízne na šířku elementu
a doplní `...`, aby nepřetékal do sousedního prvku. WebUI načítá podporované
volby stylů z `GET /api/layout/home`.


### Vrstvy, překryvy a vzhled karty

Vnitřní elementy custom widgetu se mohou překrývat. Pořadí v poli
`elements[]` je zároveň Z-order:

- první element je nejníže,
- poslední element je nejvýše.

WebUI nabízí pro vybraný prvek `Duplikovat`, posun o vrstvu nahoru/dolů a
přesun úplně navrch/dospodu. Překryv již není validační chyba; firmware stále
odmítá duplicitní ID, neplatnou geometrii, nepodporovaný zdroj nebo prvek mimo
hranice karty.

Pro budoucí partial refresh platí konzervativní pravidlo: změna libovolného
elementu custom widgetu invaliduje celý obdélník této custom karty. Karta se
znovu složí od pozadí přes všechny vrstvy ve správném pořadí. Tím jsou překryvy
bezpečné i bez složitého výpočtu průniků jednotlivých prvků.

Každý Home panel má společné parametry vzhledu:

- `showFrame` — zapnutí/vypnutí rámečku,
- `background` — `white | black`,
- `inverseText` — bílý/inverzní obsah.

Tyto parametry fungují u vlastních i předdefinovaných Home panelů. WebUI při
volbě černého pozadí automaticky zapne inverzní text jako bezpečný výchozí stav;
uživatel jej může následně změnit.

Předdefinované panely `Počasí`, `Energie` a `Uvnitř` lze znovu vybrat,
měnit jejich geometrii a vzhled a samostatně je vrátit na aktuální automatickou
výchozí geometrii/styl tlačítkem `Výchozí` u daného panelu. Jejich interní
obsah zůstává zatím pevně definovaný rendererem; skládání vlastních KPI/textů
probíhá přes custom widgety.

Vnořený editor custom widgetu zobrazuje browserový živý náhled textu, KPI,
progress baru a grafu včetně zvoleného zarovnání, velikosti písma, pozadí a
inverze. Skutečný fyzický render po uložení zůstává autoritativní.


## Aktuální limity a chování

- maximálně 8 Home widgetů,
- maximálně 8 elementů v jednom custom widgetu,
- minimální rozměry se liší podle typu widgetu a elementu,
- vlastní widget musí mít alespoň jeden element,
- viditelné top-level Home widgety se nesmí překrývat,
- vnitřní elementy custom widgetu se naopak překrývat mohou a jejich pořadí
  určuje Z-order,
- custom widget invaliduje při změně dat celý svůj obdélník,
- předdefinované Home karty používají při automatickém refreshi užší dirty
  region zaměřený hlavně na datový obsah,
- katalog datových zdrojů vrací firmware přes `GET /api/layout/home`; WebUI
  si nemá držet vlastní pevnou kopii podporovaných zdrojů.


## RF sensor karta

Home podporuje předdefinovaný typ `rf-sensor`. Karta je navázaná na stabilní
`slotId` uloženého 433 MHz čidla, nikoli na aktuální radio-side ID. Tím zůstává
vazba platná i po re-assignu čidla.

Konfigurace karty obsahuje:

- `title` — vlastní název, např. VENKU / SKLENÍK / MRAZÁK,
- `rfSensorSlotId` — stabilní slot uloženého RF čidla,
- `rfShowHumidity` — zobrazení vlhkosti, pokud ji čidlo poskytuje,
- `rfShowLastSeen` — zobrazení stáří posledního přijatého vzorku.

Editor Home umožňuje karty přesouvat, měnit jejich velikost a vzhled, skrývat je
nebo je úplně odebrat. Odebranou předdefinovanou kartu lze znovu přidat přes její
výchozí šablonu; nové RF karty se přidávají tlačítkem **RF čidlo**.
