# Audit chatů projektu Dashboard před ručním mazáním

> Stav k 18. 9. 2026. Účel: umožnit bezpečný ruční úklid historie ChatGPT bez
> ztráty projektových rozhodnutí.

## Legenda

- **SMAZAT** — obsah je již zachycen v aktuálním kódu nebo dokumentaci.
- **PONECHAT DO OVĚŘENÍ** — chat ještě může obsahovat jedinou kopii zdrojového
  projektu nebo rozhodnutí, které není definitivně uzavřené.
- **PONECHAT** — aktuální/rozpracovaná práce.

Hlavní záchytné dokumenty:

- `README.md`
- `docs/PV_DASHBOARD_SPEC.md`
- `docs/ARCHITECTURE.md`
- `docs/DISPLAY.md`
- `docs/FVE_INTEGRATION_HANDOFF.md`
- `docs/WEATHER_PROVIDERS.md`
- `docs/ROADMAP.md`
- `docs/PROJECT_INTENT_BACKLOG.md`

---

## A. Chaty přímo v projektu Dashboard

| Datum | Chat | Stav | Proč |
| --- | --- | --- | --- |
| 7. 9. | **Návrh hlavní jednotky** | **SMAZAT** | Výběr základní architektury, GoodWe, HA, senzory a gateway jsou zachycené ve specifikaci, architektuře a PROJECT_INTENT_BACKLOG. |
| 9. 9. | **Posouzení eink displejů** | **SMAZAT** | Současný 7,5" BW panel, Waveshare board, limity RAM/flash a budoucí možnost migrace jsou zdokumentované. |
| 11. 9. | **Vytvoření projektu Dashboard** | **SMAZAT** | Původní bootstrap projektu je překonaný aktuálním masterem, README a release dokumentací. |
| 11. 9. | **GUI pro bazén** | **SMAZAT** | PoolScreen existuje a budoucí reálná bazénová data/DS18B20 jsou zachycená v backlogu. |
| 11. 9. | **Přenos informací do dashboardů** | **SMAZAT** | Průzkum neidentifikované/nelokální jednotky nepřinesl potvrzenou současnou integraci. Aktuální zdroje jsou GoodWe + AZRouter. |
| 12. 9. | **Připojení k GoodWe UI** | **SMAZAT** | Pátrání po lokálním UI/hf-lpt230 již není zdrojem aktuálního protokolu. GoodWe transport je popsán v FVE_INTEGRATION_HANDOFF. |
| 12. 9. | **Simulace AZRouteru a střídače** | **SMAZAT** | Zdrojový projekt i dokumentace jsou v samostatném repu `PetrSindelarHCZ/Dashboard.DeviceSimulator`; obsahuje GoodWe UDP, AZRouter HTTP, fixtures, testy, API dokumentaci a VS Code workspace. |
| 13. 9. | **Příprava simulátoru projektu** | **SMAZAT** | Finální simulátor je uložen v `PetrSindelarHCZ/Dashboard.DeviceSimulator` a repo obsahuje kompletní Python projekt, dokumentaci, testy, workspace i startovací skript. |
| 17. 9. | **Nahrání v0.1.6 bez IDE** | **SMAZAT** | Historický postup prvního OTA testu. Aktuální OTA/release workflow je v README a masteru. |
| 17. 9. | **Příprava OTA testu** | **SMAZAT** | Testovací release řada 0.1.x je překonaná prvním plným release 1.0.0; testovací rozhodnutí jsou v release dokumentaci. |
| 17. 9. | **Kontrola větvení GitHubu** | **SMAZAT** | Jednorázový Git/branch troubleshooting; výsledný stav je v historii repozitáře. |
| 17. 9. | **Strategie obnovy displeje** | **SMAZAT** | Wi-Fi/AP ikony, NTP-validita, české fonty, záhlaví a refresh strategie jsou zachycené v DISPLAY.md a backlogu. |
| 17. 9. | **Probuzení ESP přes 433MHz** | **SMAZAT** | Myšlenka wake přes 433 MHz je zachycená jako volitelná v PROJECT_INTENT_BACKLOG. |
| 18. 9. | **Změna šipky karet** | **SMAZAT** | Hotová čistě vizuální změna WebUI; branch byla ukončená a výsledek je v masteru. |
| 18. 9. | **Nový branch pro WebUI e ink preview** | **SMAZAT** | Preview je v masteru přes `/api/display.bmp`; zásada „náhled na stránce Obrazovky“ je zachycená v backlogu. |
| 18. 9. | **Oprava pádu weather tasku** | **PONECHAT** | Aktuální debugging. Nechat minimálně do uzavření chyby a ověření na zařízení. |

### První bezpečná vlna

Můžeš hned ručně smazat těchto **15 chatů**:

1. Návrh hlavní jednotky
2. Posouzení eink displejů
3. Vytvoření projektu Dashboard
4. GUI pro bazén
5. Přenos informací do dashboardů
6. Připojení k GoodWe UI
7. Simulace AZRouteru a střídače
8. Příprava simulátoru projektu
9. Nahrání v0.1.6 bez IDE
10. Příprava OTA testu
11. Kontrola větvení GitHubu
12. Strategie obnovy displeje
13. Probuzení ESP přes 433MHz
14. Změna šipky karet
15. Nový branch pro WebUI e ink preview

Po této vlně má z uvedených projektových chatů zůstat minimálně:

- Oprava pádu weather tasku

---

## B. Dashboardové chaty mimo projekt / s nejasným zařazením

### „jaké gpio bys použil pro BM280“ — SMAZAT

Zachycené informace:

- současné e-paper GPIO,
- I²C návrh GPIO21/22,
- dříve zvažovaná GPIO pro tlačítka/joystick,
- možnost budoucí ESP32-S3 N32R16V + externí e-paper driver,
- možnost GT911 dotykového panelu.

Vše je nyní v `PROJECT_INTENT_BACKLOG.md`.

### Chat z 15. 9. kolem UTF-8 / `feature/eink-ui-utf8-graphs` — SMAZAT

Poznávací body:

- česká diakritika na invertovaném záhlaví,
- `EInkGraph`,
- grafy počasí/FVE,
- větev `feature/eink-ui-utf8-graphs`.

UTF-8 problém je dokumentovaný v `DISPLAY.md`; `EInkGraph` je implementovaný
v masteru a jeho budoucí použití je zachycené v PROJECT_INTENT_BACKLOG.

### „GitHub Actions limity a ceny“ — SMAZAT z pohledu Dashboardu

Dashboardová část tohoto chatu obsahovala:

- NTP strategii,
- návrh cloudové historie KPI,
- 5min vzorky a hourly/daily/monthly agregace,
- lokální frontu při výpadku,
- Azure/AWS úvahy.

Projektové rozhodnutí je nyní zachycené v `PROJECT_INTENT_BACKLOG.md`.
Konkrétní cloudová platforma nebyla definitivně zvolena a má se při implementaci
znovu posoudit podle aktuálních podmínek.

### Chat z 17. 9. o Wi-Fi/NTP/živém nastavení — PONECHAT UŽ JEN KVŮLI WEATHER WORKERU

Poznávací body:

- více známých Wi-Fi sítí,
- AP+STA fallback,
- ruční **Odpojit/Připojit**,
- DHCP/statická IP,
- NTP,
- vypínání počasí,
- přejmenování „Datové zdroje“ na „Fotovoltaika“.

Wi-Fi chování je nyní definitivně uzavřené: ruční **Odpojit** má persistentně
nastavit `autoConnect=false` a dané SSID se nesmí automaticky připojit ani po
restartu. Znovu se povolí až ruční akcí **Připojit**. Současný master je tedy
správně.

Zůstává už jen jeden otevřený bod: původní požadavek říkal, že vypnuté počasí
odstraní worker; aktuální master ponechá WeatherWorker task vytvořený, ale
dormantní a bez HTTPS requestů. Tento bod je zachycený v
`PROJECT_INTENT_BACKLOG.md` a `ROADMAP.md`.

Chat lze smazat po uzavření tohoto jediného WeatherWorker rozhodnutí.

---

## C. Podmínky pro druhou vlnu mazání

### Simulátor — OVĚŘENO

Samostatný repozitář `PetrSindelarHCZ/Dashboard.DeviceSimulator` byl ověřen.
Obsahuje:

- zdrojový projekt `simulator/`,
- GoodWe UDP/8899 emulaci,
- AZRouter HTTP/8081 emulaci,
- ovládací WebUI/API na 8080,
- `README.md`,
- `docs/PROTOCOLS.md`, `docs/API.md`, OpenAPI, architekturu a handoff,
- `fixtures/`,
- `tests/`,
- VS Code workspace a `start.ps1`.

Oba chaty **Simulace AZRouteru a střídače** a **Příprava simulátoru projektu**
jsou proto bezpečně zařazené do **SMAZAT**.

### Weather runtime

Wi-Fi část je uzavřená a odpovídá aktuálnímu masteru. Pro chat o
Wi-Fi/NTP/živém nastavení zbývá už jen rozhodnutí, zda při vypnutém počasí může
WeatherWorker zůstat jako dormantní task, nebo se má skutečně rušit/vytvářet
podle stavu modulu.

### Weather crash

Chat **Oprava pádu weather tasku** lze smazat až tehdy, když:

- příčina je známá,
- oprava je v masteru,
- build projde,
- firmware proběhne na zařízení bez opakování pádu,
- relevantní omezení/příčina je případně zapsaná do kódu nebo dokumentace.

---

## D. Co není důvodem chat ponechávat

Chat není potřeba uchovávat jen proto, že obsahuje:

- staré sériové logy,
- jednorázové PlatformIO příkazy,
- čísla starých testovacích verzí,
- staré branche,
- již sloučený CSS/JS fix,
- postup, který je vidět z Git historie,
- screenshot hotové UI změny,
- dřívější návrh, který je již zachycený v PROJECT_INTENT_BACKLOG.

Git, aktuální dokumentace a zdrojový kód jsou pro tyto informace autoritativnější
než historický chat.

---

## E. Doporučený postup ručního úklidu

1. Smazat 13 chatů z **První bezpečné vlny**.
2. Mimo projekt případně smazat chat **jaké gpio bys použil pro BM280**,
   UTF-8/EInkGraph chat a **GitHub Actions limity a ceny**, pokud je nepotřebuješ
   kvůli jiným tématům mimo Dashboard.
3. Simulátorové chaty už lze smazat; autoritativním zdrojem je
   `PetrSindelarHCZ/Dashboard.DeviceSimulator`.
4. Nechat Wi-Fi/NTP chat už jen do rozhodnutí chování WeatherWorkeru při vypnutém počasí.
5. Nechat aktuální weather-crash chat do dokončení opravy.
6. Po těchto zbývajících kontrolách provést další vlnu a znovu aktualizovat tento
   dokument.

Po první vlně bude historie výrazně čistší a přitom zůstanou zachované chaty,
které ještě skutečně nesou riziko ztráty informace.
