# Poskytovatelé počasí

WeatherWorker běží mimo hlavní smyčku a vybírá implementaci přes
IWeatherProvider. OpenMeteoClient a MetNorwayClient převádějí rozdílné odpovědi
do společného WeatherData, takže obrazovka počasí neobsahuje logiku konkrétního
API.

## Open-Meteo

- endpoint: `https://api.open-meteo.com/v1/forecast`,
- poskytuje aktuální hodnoty, denní agregace i hodinové body v jednom požadavku,
- konfigurace používá souřadnice a časovou zónu `auto`,
- vyhledání názvu místa ve WebUI používá geocoding Open-Meteo.

## MET Norway

- endpoint: `https://api.met.no/weatherapi/locationforecast/2.0/compact`,
- každý požadavek posílá identifikační User-Agent s verzí a adresou projektu,
- klient ukládá `Last-Modified` a při dalším požadavku posílá
  `If-Modified-Since`,
- další čtení neplánuje před časem určeným hlavičkami `Date` a `Expires`,
- symboly počasí převádí do společných kódů použitých rendererem,
- API nemusí poskytnout pravděpodobnost srážek. V takovém případě se zobrazuje
  pouze dostupné množství srážek.


## Životní cyklus WeatherWorkeru

Při vypnutém modulu počasí nemá WeatherWorker držet runtime prostředky ani
ponechat Weather obrazovky v navigaci:

- při startu s `weather.enabled=false` se FreeRTOS task ani mutex nevytvoří,
- při vypnutí za běhu dostane worker požadavek na korektní ukončení,
- pokud právě probíhá HTTP/TLS operace, nechá se bezpečně dokončit a task se
  nemaže násilně z jiného tasku,
- před ukončením se uvolní weather cache, mutex a stack tasku,
- uložená konfigurace provideru, lokalit i jejich pořadí zůstává v NVS,
- Weather a hodinové obrazovky se za běhu odregistrují ze ScreenManageru,
- při opětovném zapnutí se worker i obrazovky vytvoří/registrují znovu z uložené konfigurace.

Toto chování šetří přibližně 12 kB stacku plus cache a režii tasku v době, kdy
je modul počasí vypnutý. Implementace byla ověřena na fyzickém ESP32 scénářem
zapnuto → vypnout → znovu zapnout. Worker se korektně ukončil, uvolnil runtime
prostředky a po opětovném zapnutí znovu načetl všechny tři lokality.


## Více lokalit a pořadí

Konfigurace podporuje nejvýše **8 lokalit**. Každá má stabilní ID, název, stát a
souřadnice. `activeLocationId` určuje persistentní lokalitu používanou Home a
výchozími daty mimo Weather stránku.

WebUI umožňuje lokalitu vyhledat, přidat, vybrat, odstranit a změnit její pořadí.
Pořadí se ukládá do NVS a současně určuje pořadí podstránek v e-ink Weather
pageru. Přesun v seznamu proto není pouze kosmetická změna.

WeatherWorker udržuje cache podle ID lokality. Na hlavní Weather obrazovce lze
přes Pager dočasně zobrazit jinou uloženou lokalitu, aniž by se změnil
`activeLocationId`. Po návratu na Home se tedy dál používá uživatelem zvolená
aktivní lokalita.

Pokud je v pageru focus, levá Weather karta zobrazuje `MÍSTO: <název>`. Dole
se při více lokalitách zobrazují tečky a vyplněná tečka označuje právě zvolenou
podstránku. Poskytovatel aktuálních dat se zobrazuje také na Home obrazovce.

## HTTPS

Oba klienti používají WeatherTls a společný PEM seznam v
WeatherRootCertificates.h. Připojení nevyužívá režim `setInsecure()`.
HTTPS/TLS fetch navíc sdílí memory-heavy gate s DisplayWorkerem, takže se na
ESP32 bez PSRAM nepřekrývá TLS alokace s renderem/preview displeje.

Při změně certifikačního řetězce API je nutné kořenový certifikát ověřit vůči
důvěryhodnému řetězci, aktualizovat seznam, sestavit firmware a oba providery
otestovat na zařízení.

## ČHMÚ

Veřejná předpovědní data ČHMÚ jsou publikována jako adresáře regionálních
dávkových souborů s doprovodnými metadaty. Nejde o jednoduchý bodový endpoint
se souřadnicemi. Adaptér proto vyžaduje nejprve spolehlivě určit soubor a záznam
pro zadané místo a omezit objem načtených dat tak, aby se vešel do paměti ESP32.
Do té doby zůstává volba ČHMÚ ve WebUI vypnutá.

Zdroje: [MET Norway API](https://api.met.no/doc/TermsOfService),
[Locationforecast](https://api.met.no/weatherapi/locationforecast/2.0/documentation)
a [veřejná data ČHMÚ](https://opendata.chmi.cz/meteorology/weather/forecast/).

## Hodinový přehled na displeji

Pager lokalit na hlavní Weather obrazovce a hodinové podobrazovky jsou dvě
odlišné vrstvy: pager mění zobrazovanou lokalitu, zatímco
`weather-hourly-0` až `weather-hourly-3` vybírají den předpovědi.

Ve WebUI v části obrazovek lze zvolit datum u „Hodinová předpověď“ a otevřít
detail dne. Návrat na souhrnnou předpověď obstará tlačítko „Počasí“.
Každý den má nejvýše osm karet s časem, ikonou, teplotou, větrem, množstvím
srážek a případně jejich pravděpodobností. Body obsahují datum, aby se nemíchaly
hodiny různých dnů. MET Norway pro vzdálenější dny poskytuje řidší časové body;
chybějící body firmware nedoplňuje odhadem. Přepnutí dne vyvolá plný refresh.
