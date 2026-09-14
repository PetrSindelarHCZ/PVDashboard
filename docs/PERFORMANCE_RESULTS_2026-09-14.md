# Výsledek měření 14. 9. 2026

> Historický záznam: měření vzniklo před pozdějšími úpravami plánování a
> dvoufázové plné obnovy displeje. Slouží jako reprodukovatelný výchozí stav.

Zařízení: `192.168.88.181`, USB `COM5`, firmware `0.1.3` doplněný o diagnostiku.
Sestavení i nahrání přes USB uspělo. Zachována uložená konfigurace a dřívější
rozpracované změny projektu. Žádné optimalizace chování zatím neprovedeny.

## Naměřeno

| Operace | Doba |
| --- | ---: |
| AZRouter `/power` | přibližně 5 006 ms |
| AZRouter `/status` | přibližně 5 004–5 007 ms |
| AZRouter `/devices` | přibližně 5 003–5 007 ms |
| GoodWe, první úspěšné dotazy | 20–23 ms |
| GoodWe, pozdější timeouty | 3 609–3 617 ms |
| Plný refresh displeje | 5 428 ms |
| Částečný refresh displeje | 1 980–1 984 ms |
| Status handler, poslední sériový souhrn | průměr 10,2 ms, maximum 14 ms |
| Nejdelší zaznamenaná mezera mezi obsluhami webu | 20 684 ms |

Minutový výchozí HTTP běh: 4 požadavky, všechny skončily timeoutem.
Pětiminutový běh s diagnostikou (13:11:05–13:16:11 CEST včetně doběhnutí
posledního požadavku): 19 požadavků, 18 timeoutů, jediná úspěšná odpověď
za 4 815 ms. Medián/p95 z jedné úspěšné odpovědi nejsou reprezentativní.
Timeout skriptu platí pro síťové operace; celková doba některých neúspěšných
dotazů proto přesáhla 15 sekund.

## Příčina a doporučené pořadí

AZRouter je nakonfigurovaný na `192.168.88.51:8081` a v tomto běhu nebyl
dostupný. Ani přímý GET z počítače nenavázal spojení do 3 sekund.
Tři synchronní pokusy ve firmwaru blokují hlavní smyčku dohromady asi 15 sekund.
V lokálně použitém Arduino-ESP32 2.0.17 má `HTTPClient` výchozí
`_connectTimeout = 5000`. `setTimeout(1500)` mění pouze `_tcpTimeout`;
navázání spojení používá samostatný `_connectTimeout`.
Ověřeno přímo v instalovaných `HTTPClient.h` a `HTTPClient.cpp`.

Polling má interval 10 sekund a jeho čas se ukládá před dotazy. Po dokončení
15sekundového čtení je další pokus již splatný. Pozdější timeouty GoodWe
a překreslování prodlužují blokování až nad 20 sekund. Samotné zpracování
statusu přitom zabírá řádově jen deset milisekund.

1. Ověřit správný endpoint a dostupnost AZRouteru; nepoužívaný zdroj vypnout.
2. Nastavit samostatný connect timeout, po výpadku vynechat navazující dotazy
   a prodloužit interval dalších pokusů; plánovat pauzu od dokončení pokusu.
3. Oddělit blokující čtení zdrojů a vykreslování od obsluhy webu.
4. Upravit WebUI tak, aby nemělo více současně čekajících statusových požadavků.

Zdroje nebyly pro tento běh vypínány ani přenastavovány. Nebyla provedena
celá srovnávací matice scénářů; výsledky popisují aktuální běh se selhávajícími
zdroji, nikoli zdravou síť nebo stav po opravě. Diagnostika sama přidává malou
režii a zvětšuje JSON statusu. Příčina nedostupnosti externích zdrojů nebyla určena.

## Artefakty

- Výchozí běh: `.pio/measurements/baseline/20260914-130851-489925/`
- Diagnostický běh: `.pio/measurements/instrumented/20260914-131105-856163/`
- Každý běh obsahuje `http.csv`, `status.jsonl`, `serial.log`, `summary.json`.
- Firmware SHA256: `FB35B677A96E9FD1C6A63DA08B9DFF9661CDE80AE3CF1B3F3A0C80E71F687735`.
- Postup opakování: [PERFORMANCE.md](PERFORMANCE.md).

Surová měření jsou v ignorované složce `.pio`, nejsou součástí Git historie.
