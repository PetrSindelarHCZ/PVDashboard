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

## HTTPS

Oba klienti používají WeatherTls a společný PEM seznam v
WeatherRootCertificates.h. Připojení nevyužívá režim `setInsecure()`.
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
