# Diagnostika s funkčními simulacemi – 14. 9. 2026

> Historický záznam: měření zachycuje firmware před pozdější úpravou refresh
> strategie a sjednocením obrazovek. Naměřené příčiny blokování zůstávají platné.

Měření 13:33:08–13:38:08 CEST, zařízení `192.168.88.181`, sériový port COM5.
Oba zdroje směřovaly na `192.168.88.9` (GoodWe UDP 8899, AZRouter HTTP 8081).
Během běhu uživatel přepínal obrazovky a spouštěl refresh; zároveň běželo WebUI
a jeden postupný HTTP diagnostický požadavek s pauzou 1 sekunda po dokončení.
Firmware ani konfigurace nebyly při tomto měření měněny.

## Výsledek

| Metrika | Výsledek |
| --- | ---: |
| HTTP požadavky | 239 |
| Úspěšné | 231 |
| Chyby spojení (WinError 10054, reset protistranou) | 8 / 3,35 % |
| Medián úspěšných odpovědí | 46,64 ms |
| p95 úspěšných odpovědí | 1 516,15 ms |
| Maximum úspěšné odpovědi | 4 388,42 ms |
| Úspěšné odpovědi nad 1 sekundu | 20 |
| Průměrný čas status handleru | 9,49 ms |
| GoodWe – průměr, 29 čtení v intervalu mezi snímky | 25,62 ms |
| AZRouter – součet průměrů tří dotazů, 29 sad | 113,83 ms |
| Částečný refresh v zachyceném logu | 1 976–1 992 ms |
| Plný refresh v zachyceném logu, 4 výskyty | 5 110–5 121 ms |

Oba zdroje byly dostupné ve všech 231 přijatých statusových snímcích.
Ve snímcích nebyl zaznamenán restart. Průměry firmware byly dopočítané
z rozdílu kumulativních počtů a součtů prvního a posledního snímku, takže
nezahrnují starší běh ani boot. HTTP percentily nezahrnují chybové odpovědi.

## Závěr

S funkčními zdroji je běžná HTTP odezva v desítkách milisekund. Sekundová
zdržení odpovídají synchronnímu překreslování e-paperu v hlavní smyčce.
Například plný refresh 13:34:07–13:34:12 zabral 5,11 sekundy a překrývající
se HTTP požadavek skončil resetem spojení po 4,82 sekundy.
Ne všechny resety však nastaly během překreslování; přesný původ každého
resetu bez síťového záznamu není určen. Tento běh zahrnuje souběh WebUI
a diagnostického klienta, nejde o měření samotného prohlížeče.

V logu jsou i dvě částečná překreslení těsně za sebou kolem změny minuty
(např. 13:33:58–13:34:02). V aplikaci se čas aktualizuje až po vykreslení;
jeho změna pak požádá o další refresh. Periodický minutový refresh je další
nezávislý spouštěč. Tyto požadavky je vhodné sloučit.

Doporučené pořadí oprav:

1. Oddělit vykreslování od obsluhy HTTP, předávat snímek dat a příkazy bezpečnou
   frontou. Fyzická obnova e-paperu zůstane pomalá, web musí během ní odpovídat.
2. Sloučit požadavky na překreslení, aktualizovat čas před přípravou snímku.
3. Zajistit nejvýše jeden současný statusový požadavek ve WebUI a zobrazovat
   odděleně přijetí příkazu a dokončení překreslení.
4. Zachovat plán odolnosti vůči nedostupným zdrojům z předchozího měření:
   omezení connect timeoutu, pauzy po chybách, oddělení síťového pollingu.

Ukládání konfigurace tento běh netestoval; zde nebyl odeslán žádný konfigurační POST.

Surová data: `.pio/measurements/sources-online/20260914-133308-878970/`
(`http.csv`, `status.jsonl`, `serial.log`, `summary.json`). Složka `.pio` je
ignorovaná Gitem. Opakování měření popisuje [PERFORMANCE.md](PERFORMANCE.md).
