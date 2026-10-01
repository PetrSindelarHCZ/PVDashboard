# Testování a diagnostika

Tento dokument popisuje aktuální způsob sestavení, host-side testů, měření
odezvy a provozního ověření PVDashboardu. Jednorázové historické výsledky jsou
dostupné v Git historii a nejsou součástí živé dokumentace.

## Build a host-side testy

Běžný firmware:

~~~powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
~~~

Host-side testy:

~~~powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" test -e native
~~~

Aktuálně je host-side pokrytí stále omezené; v `tests/native` je dnes pouze
test logiky Wi-Fi signal levelu. Roadmapa proto správně počítá s rozšířením
o parser GoodWe, AZRouter JSON varianty, konfiguraci, layout validaci,
RF binding/rebind logiku a porovnávání verzí.

## Release kontrola

Release artefakty připravuje:

~~~powershell
python scripts/prepare-release.py --firmware .pio/build/esp32dev/firmware.bin --output dist --expected-version 1.26.274.1
~~~

Kontroluje se zejména:

- shoda očekávané verze s firmware,
- velikost obrazu vůči OTA slotu **1 966 080 B**,
- SHA-256 firmware,
- vytvoření manifestu.

GitHub release tag používá formát **v1.YY.denRoku.pořadí** (např. **v1.26.274.1**) a musí odpovídat FIRMWARE_VERSION.

## Měření odezvy

Firmware publikuje běžný odlehčený stav přes `/api/status`. Kumulativní
výkonnostní diagnostiku přidá `/api/status?details=1` pod objektem
`performance`. Měří hlavní smyčku, obsluhu webu, status handler, GoodWe,
jednotlivé operace AZRouteru a oba režimy e-paper refreshu.

Doporučené měření:

~~~powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" scripts/measure-response.py --url http://192.168.88.181 --serial COM5 --duration 300
~~~

IP adresa a sériový port jsou pouze příklad aktuálního testovacího zapojení.
Bez --serial se měří jen HTTP. Výstup se ukládá do ignorované složky
.pio/measurements jako CSV/JSONL/log a souhrn.

Pro srovnatelné měření:

1. nejprve 5 minut bez zásahů a se zavřeným WebUI,
2. zvlášť otestovat přepínání obrazovek a partial/full refresh,
3. zvlášť otestovat GoodWe a AZRouter aktivní, vypnuté a nedostupné,
4. při záseku korelovat HTTP čas se sériovým PERF logem,
5. po změně firmware porovnávat celé scénáře, ne jednotlivé náhodné maximum.

## Displej

Krátké testy mají ověřit:

- WebUI odpovídá během partial i full refreshu,
- více rychlých požadavků se sloučí a full požadavek nezanikne,
- start a explicitní full-refresh provedou čisticí full refresh,
- přepnutí obrazovky fyzickou navigací může použít full-window differential
  partial refresh,
- běžná aktualizace stejné obrazovky používá partial refresh,
- změna focusu a vybrané Home datové změny používají regionální dirty refresh,
- preview odpovídá poslednímu vyrenderovanému snímku,
- Weather TLS a render se díky memory-heavy gate nepřekrývají.

Stále chybí alespoň **24hodinový provozní test** minutových aktualizací.
Během něj zaznamenat ghosting, případné chyby renderu/TLS, heap a okolní teplotu.
Počet partial refreshů se nemá používat jako automatický důvod pro full refresh.

## Wi-Fi a konfigurace

Před release ověřit alespoň:

- připojení k uložené Wi-Fi,
- recovery AP Dashboard-Setup,
- přechod na jinou známou síť,
- persistentní ruční Odpojit a opětovné ruční Připojit,
- DHCP i statickou konfiguraci,
- export/import YAML,
- tovární reset pouze po explicitním potvrzení.

## Dynamické moduly

Pro Počasí, Bazén a FVE ověřit změnu aktivace za běhu:

- obrazovka a sidebar položka se registrují/odregistrují správně,
- vypnutí aktivní obrazovky vrátí dashboard na Home,
- NavigationController nezůstane na neplatném focusu,
- vypnutý GoodWe/AZRouter se nepolluje,
- vypnutý WeatherWorker uvolní runtime prostředky a po zapnutí se znovu vytvoří.

## OTA

Ověřit oba směry aktualizace:

- ruční upload se správným SHA-256,
- GitHub release OTA,
- odmítnutí chybného SHA-256,
- odmítnutí obrazu většího než OTA partition,
- běžící firmware zůstane použitelný po neúspěšné aktualizaci.

Před experimenty s OTA ponechat dostupný poslední známý funkční firmware pro
obnovu přes USB.


## BME280 a MAX17048

Před release ověřit:

- BME280 na 0x76/0x77 a správné hodnoty teploty/vlhkosti/tlaku,
- odpojení a opětovné připojení BME280 bez nutnosti restartu zařízení,
- MAX17048 na 0x36,
- napětí, SoC a CRATE v rozumném rozsahu,
- chování po odpojení MAX17048 a následné nové inicializaci,
- že lokální I²C polling neblokuje WeatherWorker ani e-paper worker,
- že bateriové hodnoty aktualizují pouze příslušný Home region.

## 433 MHz / RF senzory

Pro správu RF čidel ověřit:

- scan 30–180 s,
- nalezení podporovaného čidla,
- přidání čidla a stabilní `slotId`,
- přejmenování,
- odstranění,
- rebind po změně rádiového ID,
- obnovu teploty/vlhkosti/battery dat,
- přechod do nedostupného stavu po 5 minutách bez paketu,
- použití RF hodnot ve vlastním Home widgetu,
- export/import RF konfigurace přes YAML v7.

Při RF testu současně sledovat, zda e-paper aktivita nevytváří nežádoucí
výpadky capture nebo falešné vícenásobné vstupy.

## Layout editor

Ověřit minimálně:

- defaultní automatický Home layout,
- uložení vlastního layoutu a přežití restartu,
- reset celé Home obrazovky na výchozí stav,
- reset jednotlivého předdefinovaného panelu,
- drag/resize s mřížkou i bez ní,
- validaci minimálních rozměrů a hranic,
- max. 6 Home widgetů a 8 elementů v custom widgetu,
- překryvy uvnitř custom widgetu a správný Z-order,
- numerický `fontSize` 7–64 px,
- battery a RF datové zdroje,
- YAML export/import layoutu.

## AZRouter autentizace

Ověřit oba režimy:

- `authEnabled=false`: pole user/password nejsou použita a anonymní API funguje,
- `authEnabled=true`: login, token/cookie a re-login po 401/403.

Pozor: aktuální YAML v7 neexportuje `authEnabled` ani credentials. Při importu
se existující lokální username/password zachovají, ale `authEnabled` se nastaví
na výchozí `false`, takže autentizace se sama nezapne.


## HTTP/API diagnostika

Při release nebo větší změně WebUI ověřit také:

- `GET /api/display` vrací metadata preview a `/api/display.bmp` odpovídající BMP,
- `GET /api/diagnostics/device?source=goodwe|azrouter` vrací stav DNS/ping/port probe,
- `POST /api/sources/test` testuje zadaný host/port před uložením konfigurace,
- `GET /api/ntp/status` a `GET/POST /api/ntp/custom` fungují bez úniku citlivých údajů,
- `GET /api/config/wifi` nevrací heslo,
- `GET/POST /api/network/config` korektně rozlišuje DHCP a statickou IPv4,
- RF management endpointy add/rename/rebind/remove odmítají neplatné sloty a duplicity.
