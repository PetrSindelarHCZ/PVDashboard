# Měření odezvy

Metodika platí i pro současný firmware. Konkrétní výsledky jsou historické
snímky a jejich časy nemusí odpovídat pozdější dvoufázové obnově displeje.

## Související dokumenty

- [architektura](ARCHITECTURE.md),
- [displej a refresh strategie](DISPLAY.md),
- [roadmapa výkonových oprav](ROADMAP.md),
- [výsledek s nedostupnými zdroji](PERFORMANCE_RESULTS_2026-09-14.md),
- [výsledek s funkčními zdroji](PERFORMANCE_SOURCES_ONLINE_2026-09-14.md).

Firmware měří celou smyčku (včetně `delay(20)`), interval mezi voláními obsluhy
webu, samotné `handleClient`, status handler, inicializaci a oba režimy displeje,
GoodWe a jednotlivé dotazy AZRouteru včetně čtení a parsování odpovědi.
Statistiky jsou kumulativní od restartu: počet, průměr, maximum, poslední trvání
a čas jeho dokončení v milisekundách od startu. Jsou v `/api/status` pod
`performance`. Právě běžící operace se projeví až po dokončení; status handler
a obsluha webu tedy ve vlastní odpovědi obsahují předchozí měření.

Sériový výstup `[PERF]` uvádí souhrn každých přibližně 30 sekund a operace
dlouhé alespoň 500 ms. Časování má rozlišení 1 ms. Instrumentace, větší status
a sériový výpis přidávají režii; měření slouží k hledání sekundových záseků.
Statistiky nejsou připravené na současné zápisy z více úloh.

Z kořene projektu (Python z PlatformIO má i pyserial):

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" scripts/measure-response.py --url http://192.168.88.181 --serial COM5 --duration 300
```

Zavřít ostatní sériové monitory. Skript nenastavuje DTR/RTS pro záměrný reset,
ale otevření portu může podle ovladače/desky zařízení resetovat. Bez `--serial`
měří pouze HTTP a nepotřebuje pyserial. Odpovědi ukládá do `.pio/measurements`
do CSV, stavové snímky do JSONL a sériový výstup do logu s UTC časem.
Požadavky běží postupně s pauzou 1 sekunda po dokončení. Timeout síťových
operací je 15 sekund, nikoli pevný limit celkové doby dotazu: připojení a čtení
odpovědi mohou dohromady trvat déle. Poslední požadavek může přesáhnout délku
měření. Medián, p95 (nearest rank)
a maximum zahrnují pouze úspěšné HTTP/JSON odpovědi, chyby se počítají zvlášť.

Nejprve měřit 5 minut bez zásahů a se zavřeným WebUI. Pak zvlášť měřit
přepínání obrazovek a refresh. Následně porovnat zdroje jednotlivě a vypnuté,
případně nedostupný zdroj. Skript sám konfiguraci ani obrazovku nemění.
Při porovnávání kumulativních maxim zohlednit restart; pro konkrétní zásek
korelovat UTC čas HTTP se sériovým logem a `lastEndMs` s `performance.uptimeMs`.
