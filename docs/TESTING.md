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

Aktuálně je host-side pokrytí omezené. Roadmapa počítá s testy parseru GoodWe,
AZRouter JSON variant, konfigurace a porovnávání verzí.

## Release kontrola

Release artefakty připravuje:

~~~powershell
python scripts/prepare-release.py --firmware .pio/build/esp32dev/firmware.bin --output dist --expected-version 1.26.261.1
~~~

Kontroluje se zejména:

- shoda očekávané verze s firmware,
- velikost obrazu vůči OTA slotu **1 966 080 B**,
- SHA-256 firmware,
- vytvoření manifestu.

GitHub release tag používá formát **v1.YY.denRoku.pořadí** (např. **v1.26.261.1**) a musí odpovídat FIRMWARE_VERSION.

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
- změna obrazovky provede čisticí full refresh,
- běžná aktualizace stejné obrazovky používá partial refresh,
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
