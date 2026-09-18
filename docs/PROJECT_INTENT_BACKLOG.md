# Zachycené projektové záměry a neimplementované funkce

> Stav k 18. 9. 2026. Tento dokument vznikl při auditu projektových diskusí před
> jejich případným odstraněním. Jeho účelem je zachovat rozhodnutí a nápady,
> které nejsou ještě plně implementované nebo nejsou dostatečně popsány v
> ostatní dokumentaci.

## Jak dokument používat

Tento soubor není náhradou za `ROADMAP.md`, `ARCHITECTURE.md` ani technickou
dokumentaci konkrétních integrací. Slouží jako záchytný seznam původních
projektových záměrů.

Stavy:

- **HOTOVO** — funkce je ověřeně přítomná v aktuálním masteru,
- **ČÁSTEČNĚ** — základ existuje, ale původní záměr není dokončen,
- **PLÁN** — záměr byl odsouhlasen nebo opakovaně diskutován, ale implementace
  zatím není v masteru,
- **VOLITELNÉ** — zajímavá budoucí možnost, nikoli závazná součást nejbližšího
  vývoje.

Při konfliktu s aktuálním kódem má přednost kód a následně specializovaná
dokumentace. Tento dokument se má při realizaci jednotlivých bodů aktualizovat.

---

## 1. Funkce, které už není potřeba držet pouze v historii chatů

### WebUI, konfigurace a OTA — HOTOVO

Aktuální firmware obsahuje:

- mobilní WebUI,
- konfiguraci Wi-Fi, systému, GoodWe, AZRouteru a počasí,
- recovery AP,
- tovární reset,
- export konfigurace do verzovaného YAML,
- import YAML s validací,
- ruční OTA se SHA-256,
- OTA z GitHub release,
- kontrolu velikosti firmware a reprodukovatelné release sestavení.

Tyto oblasti jsou popsány v `README.md`, `PV_DASHBOARD_SPEC.md` a související
implementaci.

### Náhled fyzického e-inku ve WebUI — HOTOVO

Původní požadavek byl umožnit vzdáleně zjistit, co je právě vykreslené na
e-paperu, i když uživatel není u zařízení.

Aktuální master obsahuje serverový náhled displeje a endpoint
`/api/display.bmp` spolu s metadaty a WebUI obsluhou. Náhled má představovat
poslední vyrenderovaný obsah, nikoli samostatně znovu sestavenou webovou kopii
obrazovky.

Tím je zachována důležitá zásada: **náhled má vycházet ze stejného renderovaného
obsahu jako fyzický panel**, aby se WebUI a e-ink nerozcházely.

### Více lokalit počasí — HOTOVO

Konfigurace počasí podporuje více uložených lokalit (aktuálně nejvýše osm),
jejich vyhledání ve WebUI a výběr aktivní lokality. WeatherWorker používá cache
podle lokality a provideru.

### Open-Meteo a MET Norway — HOTOVO

Oba provideři používají společný model a WeatherWorker mimo hlavní smyčku.
ČHMÚ zůstává samostatným budoucím bodem.

---

## 2. Konfigurovatelné rozložení e-ink obrazovek — PLÁN

Jedním z původních hlavních záměrů bylo, aby finální rozložení obrazovek nebylo
navždy pevně zakódované v `HomeScreen.cpp`, `SolarScreen.cpp` atd.

### Požadovaný výsledek

WebUI má časem umožnit upravovat alespoň:

- které widgety jsou na konkrétní obrazovce viditelné,
- jejich pozici,
- jejich velikost,
- případně variantu widgetu,
- pořadí a existenci uživatelských obrazovek,
- návrat k výchozí šabloně.

Uživatel má vidět náhled výsledného e-ink layoutu přímo na stránce
**Obrazovky**.

### Návrhové omezení

ESP32-WROOM-32 nemá PSRAM a OTA partition má omezenou velikost. Editor proto
nemá znamenat přesun velkého JavaScript frameworku do firmware.

Preferovaný směr:

1. firmware zná omezenou množinu podporovaných widgetů,
2. layout je datový popis (např. ID widgetu, x, y, šířka, výška, parametry),
3. renderer na ESP32 používá stejný popis pro fyzický panel,
4. WebUI upravuje pouze tento popis,
5. konfigurace layoutu je součástí exportu/importu,
6. musí existovat bezpečný návrat k výchozímu layoutu.

### Co je už hotové

Serverový náhled e-inku je vhodný základ pro budoucí editor. Samotný editor
pozic a velikostí widgetů ale v aktuálním masteru není.

---

## 3. Fyzické ovládání dashboardu — PLÁN

Dashboard je nyní plně ovladatelný z WebUI. Původní návrh ale počítal také s
lokálním ovládáním bez telefonu.

### Minimální varianta

Dvě fyzická tlačítka:

- předchozí obrazovka,
- následující obrazovka.

Tlačítka mají cyklicky procházet hlavními obrazovkami. Přepnutí obrazovky musí
použít stejný ScreenManager jako WebUI a respektovat refresh politiku.

### Rozšířená varianta

V inventáři je k dispozici také pětisměrné navigační tlačítko/joystick.
Je možné později zvážit:

- vlevo/vpravo = změna obrazovky,
- nahoru/dolů = změna detailu/podobrazovky,
- stisk = potvrzení nebo návrat.

Tato varianta zatím není závazná. Před implementací je nutné ověřit dostupné
GPIO na konkrétní Waveshare desce.

---

## 4. Vnitřní čidla — PLÁN

Domovská obrazovka dnes obsahuje demonstrační hodnoty. Roadmapa už počítá s
připojením reálného senzoru.

### První plánovaný hardware

BME280:

- teplota,
- relativní vlhkost,
- atmosférický tlak.

Preferované je lokální připojení k hlavní jednotce, pokud bude čidlo fyzicky
vhodně umístěné. Data mají vstoupit do společného DataModelu a nemají být
načítána přímo rendererem.

### CO2

CO2 bylo v původních návrzích součástí domácího přehledu, konkrétní senzor ale
zatím nebyl definitivně vybrán. Proto zůstává samostatným budoucím bodem.

---

## 5. Bazén — ČÁSTEČNĚ

Obrazovka `pool` a datový model existují, ale hodnoty jsou stále demonstrační.

### Původní cílový rozsah

Minimálně:

- teplota vody,
- případně cílová teplota,
- stav technologie podle dostupných budoucích integrací.

Pro první reálný prototyp je k dispozici vodotěsný DS18B20. U bazénu není
napájení ani Wi-Fi zásadní problém, takže preferovaná architektura je samostatný
Wi-Fi uzel u bazénu, nikoli extrémně nízkopříkonový rádiový senzor.

### Budoucí rozšíření

Později lze doplnit další parametry bazénu. Datový model a API mají zůstat
modulární, aby PoolScreen nebyl svázaný s jedním konkrétním hardwarem.

---

## 6. Vzdálená venkovní čidla a 433 MHz — PLÁN

Pro venkovní senzory může být Wi-Fi i BLE nevhodné kvůli vzdálenosti a spotřebě.
V projektu proto vznikl směr použít samostatnou bránu.

### Dostupný hardware

V inventáři je modul CC1101 433 MHz s anténou.

### Zamýšlená architektura

```text
venkovní / levná 433MHz čidla
            |
          433 MHz
            |
      gateway / receiver
            |
      Ethernet nebo Wi-Fi
            |
        PVDashboard
```

Preferovaným směrem pro trvale napájenou gateway je samostatný mikrokontrolér.
Dříve byla zvažována i Ethernet/PoE brána, aby bylo možné přijímač umístit do
rádiově vhodného místa nezávisle na hlavním e-paper dashboardu.

### Proč oddělit gateway

- hlavní dashboard nemusí neustále dekódovat rádiové protokoly,
- lze lépe umístit anténu,
- nové protokoly čidel lze doplňovat bez zásahu do zobrazovací jednotky,
- gateway může později obsloužit i další domácí projekty.

### Deep sleep a probuzení rádiem — VOLITELNÉ

Byla otevřena otázka, zda lze ESP probudit z deep sleep příchodem 433MHz
signálu. To je vhodnější pro samostatné bateriové uzly než pro hlavní dashboard,
který je běžně trvale napájený. Případná implementace závisí na tom, zda výstup
konkrétního přijímače lze bezpečně použít jako wake GPIO a jak se zabrání
opakovanému probouzení šumem.

---

## 7. Dlouhodobá historie KPI a grafy — PLÁN

Původní návrh dashboardu počítá nejen s okamžitými hodnotami, ale i s historií.

### Příklady dat

- výroba FVE,
- spotřeba domu,
- import/export sítě,
- baterie,
- výkon/energie AZRouteru,
- teploty,
- později bazén a další senzory.

### Požadované časové úrovně

Směr návrhu byl víceúrovňový:

- krátkodobá/raw data,
- hodinové agregace,
- denní agregace,
- měsíční agregace.

E-paper pak nemusí stahovat nebo držet detailní dlouhou historii a pro graf
dostane pouze rozsah a rozlišení, které skutečně potřebuje.

### Lokální buffer

Při nedostupnosti internetu se nemají měření bezprostředně ztratit. Budoucí
řešení má počítat s malou lokální frontou nebo bufferem a následným odesláním.

Je nutné hlídat opotřebení flash; ukládání každého vzorku samostatně do NVS není
vhodný návrh.

---

## 8. Cloudové ukládání historie — PLÁN

Byla diskutována možnost použít cloudovou službu s cílem udržet provozní náklady
na nule nebo velmi blízko nule.

Konkrétní platforma nebyla definitivně zvolena. Před implementací se má znovu
porovnat aktuální nabídka Azure/AWS a případně jednodušších služeb.

### Požadavky na rozhraní

Firmware nemá být těsně svázaný s jedním cloudem. Preferovaný návrh:

- samostatný modul pro odesílání telemetrie,
- jasný interní datový kontrakt,
- lokální buffer,
- konfigurovatelný endpoint/credentials,
- cloud nesmí být podmínkou pro základní lokální funkci dashboardu.

Lokální FVE a WebUI musí fungovat i při úplném výpadku internetu nebo cloudové
služby.

---

## 9. ČHMÚ provider — PLÁN

Open-Meteo a MET Norway jsou funkční. ČHMÚ bylo prověřeno, ale jeho veřejná
data nejsou jednoduchý bodový endpoint.

Budoucí adaptér musí:

- bezpečně určit správný regionální soubor a záznam pro zvolené místo,
- omezit objem stahovaných dat,
- vejít se do paměťových limitů ESP32 bez PSRAM,
- převést výsledek do stejného WeatherData jako ostatní provideři.

Dokud toto není splněno, nemá být ČHMÚ ve WebUI nabízené jako funkční provider.

---

## 10. GoodWe + AZRouter a simulátor — zachovat architektonické rozhodnutí

Produkční zařízení jsou dvě nezávislá zařízení a konfigurace proto musí mít pro
GoodWe a AZRouter samostatný host/IP, port a příslušný protokol.

Vývojový simulátor může naopak provozovat oba emulované zdroje na jedné IP
adrese a rozlišovat je portem/protokolem.

Z toho plyne dlouhodobé pravidlo:

**Nikdy neslučovat konfiguraci GoodWe a AZRouteru do jednoho společného
„FVE hostu“.**

Toto je důležité i tehdy, když současný vývojový simulátor obě služby obsluhuje
na jednom počítači.

---

## 11. Home Assistant — VOLITELNÉ

Home Assistant byl zvažován jako možné budoucí rozšíření, nikoli jako základní
závislost projektu.

Současná zásada zůstává:

- dashboard musí umět pracovat přímo s lokálními zdroji,
- Home Assistant může být později další vstup/výstup nebo integrační vrstva,
- výpadek HA nesmí znemožnit základní funkce.

---

## 12. Refresh strategie e-inku — ČÁSTEČNĚ

Aktuální roadmapa stále správně drží otevřený dlouhodobý test ghostingu a
případné lokální refreshe menších oblastí.

Je nutné zachovat tyto otevřené body:

- alespoň 24hodinový test pravidelných aktualizací,
- kontrola ghostingu,
- potvrzení, zda má smysl lokální refresh pouze času nebo vybraných hodnot,
- odstranění nepoužívaného `fullRefreshIntervalMinutes` ze schématu, pokud
  definitivně nebude součástí politiky.

### Pozor na starší dokumentaci

Starší texty mohou obsahovat historické tvrzení, že se po určitém počtu partial
refreshů automaticky vynutí full refresh. Aktuální roadmapa naopak uvádí, že
počet partial refreshů již full refresh automaticky nevynucuje.

Při dalším auditu je potřeba tento rozpor sjednotit podle aktuální implementace
a ověření na fyzickém panelu.

---

## 13. Testy a technický dluh — PLÁN

Vedle nových funkcí nemají zapadnout ani host-side testy:

- GoodWe CRC, délky rámců a mapování registrů,
- AZRouter varianty a chybné JSON odpovědi,
- validace konfigurace,
- porovnávání verzí,
- automatický build a kontrola velikosti firmware.

Dále je vhodné postupně odstranit historické nebo rozporné části dokumentace,
které popisují starší refresh chování nebo starší stav WebUI.

---

## 14. Navržené budoucí etapy

Toto není pevné pořadí, ale zachycení logických celků.

### Etapa A — dokončení současné platformy

- dlouhodobý refresh/ghosting test,
- úklid refresh konfigurace,
- testy a dokumentace,
- stabilizace aktuálního release procesu.

### Etapa B — skutečná data domácnosti

- BME280,
- CO2 senzor po výběru hardware,
- reálná bazénová teplota přes samostatný uzel.

### Etapa C — vzdálené senzory

- CC1101/433MHz gateway,
- definice jednoduchého lokálního API mezi gateway a dashboardem,
- integrace venkovních teplotních čidel,
- případně PoE/Ethernet gateway.

### Etapa D — konfigurovatelné obrazovky

- datový popis layoutu,
- ukládání a migrace layout konfigurace,
- editor ve WebUI,
- využití existujícího display preview,
- reset jednotlivé obrazovky na výchozí šablonu.

### Etapa E — historie a statistiky

- lokální buffer,
- cloudový nebo vlastní backend,
- agregace,
- historické grafy na e-inku a ve WebUI.

---

## 15. Co lze po tomto zápisu bezpečně považovat za zachycené

Následující původní témata již nemusejí přežívat jen jako staré chaty:

- výběr základního e-paper hardware,
- základní architektura GoodWe + AZRouter,
- oddělená konfigurace obou zdrojů,
- vývojový simulátor na jedné IP,
- OTA a recovery,
- export/import konfigurace,
- vzdálený náhled e-inku,
- WebUI jako primární konfigurace,
- možnost fyzických tlačítek,
- konfigurovatelný layout obrazovek,
- BME280 a budoucí CO2,
- bazénová čidla,
- vzdálená 433MHz čidla a gateway,
- budoucí cloudová historie KPI,
- možné budoucí zapojení Home Assistantu,
- ČHMÚ jako možný další weather provider.

Při dalším auditu projektových chatů je možné tento dokument použít jako kontrolní
seznam: chat je kandidát na odstranění, pokud neobsahuje další technické
rozhodnutí, které není zachycené zde nebo v jiné aktuální dokumentaci.


---

## 16. Wi-Fi správa známých sítí — HOTOVO S JEDNÍM ROZPOREM K OVĚŘENÍ

Projekt se posunul od jediné uložené Wi-Fi k seznamu známých sítí. Dlouhodobý
záměr je:

- uchovávat více známých sítí,
- v AP+STA fallbacku je průběžně hledat,
- viditelné kandidáty prioritizovat podle signálu,
- při selhání jedné sítě zkoušet další,
- ruční `Připojit` má explicitně zvolenou síť znovu povolit,
- ruční `Odpojit` nesmí způsobit okamžité automatické připojení zpět ke stejné
  síti, zatímco ostatní známé sítě mohou zůstat kandidáty.

Aktuální master implementuje seznam známých sítí, AP+STA fallback a automatické
hledání dalších kandidátů.

### Rozpor k rozhodnutí

Původně odsouhlasené chování ručního **Odpojit** bylo: blokovat právě odpojené
SSID **do restartu nebo do ručního Připojit**.

Aktuální implementace ukládá pro toto SSID `autoConnect=false` do NVS. Tím je
blokace persistentní i přes restart a síť se znovu automaticky povolí až ručním
připojením.

Tento rozdíl se nesmí ztratit při mazání starých chatů. Před uzavřením Wi-Fi
části je potřeba výslovně rozhodnout, zda:

1. restart blokaci zruší podle původního požadavku, nebo
2. současné persistentní chování bude přijato jako nové pravidlo.

Do té doby je bod považován za otevřený.

---

## 17. Vypínatelný modul počasí — ČÁSTEČNĚ

Počasí má být možné vypnout bez ztráty uložené konfigurace. Původní požadavek
byl, aby vypnutí odstranilo jeho aktivní runtime části z uživatelského pohledu:

- obrazovku Počasí a hodinové podobrazovky,
- navigační položku,
- blok počasí z hlavního dashboardu,
- aktivní získávání dat,
- přičemž uložené lokality/provider/nastavení zůstanou zachované.

Aktuální master již:

- ukládá `weather.enabled`,
- odregistruje weather obrazovky,
- skryje položku v e-ink menu,
- odstraní weather kartu z hlavního zobrazení,
- při disabled stavu neposílá HTTPS dotazy,
- zachovává konfiguraci.

Technická odchylka: `WeatherWorker` FreeRTOS task se i při vypnutém modulu
vytvoří a následně čeká bez časového limitu na notifikaci. Původní formulace
„vypnutí odstraní worker“ tedy není doslova splněná. Je potřeba rozhodnout, zda
je dormantní task přijatelný, nebo zda má být worker skutečně vytvořen/zrušen
podle `weather.enabled`.

---

## 18. Vlastní EInkGraph — HOTOVO

Pro grafy na e-inku byl zvolen vlastní lehký renderer bez velké chart knihovny.
Aktuální master obsahuje `src/display/EInkGraph.*` a používá jej minimálně pro
počasí a FVE.

Toto rozhodnutí platí i pro budoucí historické grafy:

- FVE,
- baterie,
- AZRouter,
- bazén,
- další časové řady.

Preferuje se jednoduché kreslení přes existující `IDisplay`, aby se zachovala
kontrola nad pamětí, vzhledem a kompatibilitou s e-paperem.

---

## 19. GPIO rezerva současné Waveshare desky — PLÁN / HARDWAROVÁ POZNÁMKA

Aktuální e-paper zapojení v masteru používá:

- MOSI GPIO14,
- SCK GPIO13,
- CS GPIO15,
- DC GPIO27,
- RST GPIO26,
- BUSY GPIO25,
- MISO GPIO12.

Pro budoucí I²C čidla byl navržen standardní pár:

- SDA GPIO21,
- SCL GPIO22.

V dřívější hardwarové úvaze byly pro fyzická tlačítka/joystick zvažovány
GPIO16, 17, 18, 19, 23, 32 a 33. **Nejde zatím o schválený finální pinout.**
Před zapojením se musí znovu ověřit konkrétní revize Waveshare boardu,
boot-strapping piny a případné interní vazby desky.

Tato poznámka má zabránit tomu, aby se při budoucím návrhu začínalo s GPIO
inventurou znovu od nuly.

---

## 20. Možná budoucí migrace kontroleru — VOLITELNÉ

Pokud by limity současného ESP32-WROOM-32 (flash/RAM/bez PSRAM) začaly brzdit
editor obrazovek, historii, grafiku nebo další integrace, byla diskutována
migrace na ESP32-S3 s výrazně větší flash a PSRAM, například třída
**N32R16V (32 MB flash / 16 MB PSRAM)**.

Pro zachování současného 7,5" 800×480 raw e-paper panelu byla diskutována
samostatná driver deska typu DESPI-C02 / odpovídající Waveshare HAT. U DESPI-C02
je nutné před případným použitím znovu ověřit nastavení hardware pro konkrétní
panel; v dřívější diskusi byla zmíněna konfigurace RESE 0,47 Ω.

Jako další možnost byl prověřován 7,5" 800×480 panel s dotykem přes GT911/I²C,
který by mohl časem nahradit fyzický joystick. Toto **není současný plán
migrace**, pouze zachycená varianta pro případ, že narazíme na limity současné
platformy.

---

## 21. NTP provozní politika — ZACHYCENÝ ZÁMĚR

Vedle pravidla „nezobrazovat čas/datum/svátek před první validní synchronizací
od bootu“ byl diskutován i provozní interval synchronizace:

- synchronizace při startu,
- okamžitý pokus po návratu Wi-Fi,
- běžně přibližně každých 6 hodin,
- při neúspěchu kratší retry přibližně 5–15 minut.

Přesné intervaly se mohou měnit podle implementace, ale důležité pravidlo je,
že krátký výpadek Wi-Fi po již úspěšné synchronizaci nemá skrýt běžící lokální
čas. Naopak po restartu bez validního NTP se časová část záhlaví nesmí tvářit
jako aktuální jen díky zachovanému RTC času.
