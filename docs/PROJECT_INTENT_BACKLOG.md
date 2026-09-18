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

## 3. Ovládání dashboardu joystickem — ČÁSTEČNĚ

Cílovým lokálním ovladačem je pětisměrné navigační tlačítko. Fyzické GPIO zatím
nejsou zapojené; první etapa implementuje společnou softwarovou navigaci a její
ekvivalent ve WebUI.

### Schválený model navigace

Navigace rozlišuje dvě oblasti:

- **Sidebar** — výchozí stav. `UP/DOWN` prochází položky, `OK` aktivuje a načte
  vybranou stránku, `LEFT` se ignoruje a `RIGHT` vstoupí do právě zobrazené stránky.
- **Page bez podstránek** — `UP/DOWN/LEFT/RIGHT` se pohybuje mezi
  focusovatelnými prvky. `LEFT` bez dalšího prvku vlevo vrátí focus do sidebaru.
- **Page s více podstránkami** — po `RIGHT` ze sidebaru se nejdřív vstoupí do
  obecné pager vrstvy. `LEFT/RIGHT` přepíná podstránky a `OK` teprve vstoupí
  do navigace prvků aktuální podstránky. `LEFT` z prvku bez souseda vlevo vrací
  focus z prvků zpět do pageru. Z první podstránky vrací další `LEFT` do sidebaru.
- `OK` nad konkrétním prvkem je zatím rezervované pro budoucí práci s prvkem,
  editaci nebo potvrzení.

WebUI na kartě **Obrazovky** používá stejné navigační akce jako budoucí fyzický
joystick. Neobsahuje vlastní logiku přepínání.

### Dynamický layout

Navigační sousedé nejsou pevně zakódovaní podle ID widgetů. Každá stránka
poskytne aktuální seznam focusovatelných obdélníků a controller sousedy odvodí
z jejich geometrie. Po vstupu ze sidebaru se počáteční focus zvolí jako
nejlevější focusovatelný prvek, při shodě nejvyšší. Nejde ale o zvláštní
vstupní ani výstupní bod; výstup vlevo vzniká čistě z geometrie aktuálního
layoutu.

Toto pravidlo je důležité hlavně pro budoucí editovatelnou Home stránku:
po změně pozice, velikosti nebo přítomnosti widgetů se navigace sestaví z právě
platné konfigurace stránky bez změny firmware nebo ručně psaného grafu vazeb.

Pager je obecná vlastnost `IScreen`, nikoli speciální logika Počasí. Weather
ji používá pro jednu podstránku na každou nakonfigurovanou lokalitu. Při více
lokalitách se dole na e-inku zobrazí řada teček; vyplněná tečka označuje právě
zobrazenou lokalitu. Pořadí lokalit lze měnit přímo v rozbalovacím seznamu
**Nastavení → Počasí → Lokalita** a stejné pořadí určuje pořadí podstránek a
teček na displeji. Přepnutí pageru nemění persistentní `activeLocationId`,
takže Home dál používá uživatelem zvolenou aktivní lokalitu.

### Co ještě chybí

- fyzické připojení joysticku a volba GPIO,
- debounce a obsluha tlačítek,
- long-press / auto-repeat,
- akce `OK` nad konkrétními prvky,
- případné ruční override sousednosti pro atypické layouty.

Před fyzickým zapojením je stále nutné ověřit dostupné GPIO konkrétní revize
Waveshare desky a boot-strapping piny.

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

## 10. GoodWe + AZRouter a simulátor — HOTOVO / SAMOSTATNÉ REPO

Produkční zařízení jsou dvě nezávislá zařízení a konfigurace proto musí mít pro
GoodWe a AZRouter samostatný host/IP, port a příslušný protokol.

Vývojový simulátor je uložen v samostatném repozitáři
`PetrSindelarHCZ/Dashboard.DeviceSimulator`. Repo obsahuje zdrojový projekt,
README, dokumentaci protokolů a API, fixtures, testy, VS Code workspace a
startovací skript.

Simulátor může provozovat oba emulované zdroje na jedné IP adrese a rozlišuje je
portem/protokolem:

- GoodWe: UDP/8899,
- AZRouter: HTTP/8081,
- ovládací WebUI/API simulátoru: HTTP/8080.

Z toho plyne dlouhodobé pravidlo:

**Nikdy neslučovat konfiguraci GoodWe a AZRouteru do jednoho společného
„FVE hostu“.**

Toto je důležité i tehdy, když vývojový simulátor obě služby obsluhuje na jednom
počítači. Historické chaty o vzniku simulátoru už nejsou jediným zdrojem jeho
implementace ani dokumentace.

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

## 16. Wi-Fi správa známých sítí — HOTOVO

Projekt se posunul od jediné uložené Wi-Fi k seznamu známých sítí. Platné cílové
chování je:

- uchovávat více známých sítí,
- v AP+STA fallbacku je průběžně hledat,
- viditelné kandidáty prioritizovat podle signálu,
- při selhání jedné sítě zkoušet další,
- ruční `Připojit` explicitně zvolenou síť znovu povolí,
- ruční `Odpojit` dané SSID trvale vyřadí z auto-connectu,
- ručně odpojené SSID se po restartu samo znovu nepovolí,
- znovu se povolí až ruční akcí `Připojit`,
- ostatní známé sítě mohou zůstat kandidáty pro automatické připojení.

Aktuální master toto chování implementuje pomocí persistentního
`autoConnect=false` uloženého v NVS. Toto je správné a záměrné chování, nikoli
dočasný workaround ani otevřený bod.

---

## 17. Vypínatelný modul počasí — HOTOVO / OVĚŘENO

Počasí má být možné vypnout bez ztráty uložené konfigurace. Cílové chování je:

- odstranit obrazovku Počasí a hodinové podobrazovky,
- skrýt navigační položku,
- odstranit weather blok z hlavního dashboardu,
- zastavit aktivní získávání dat,
- **nevytvářet ani nedržet WeatherWorker task, mutex a cache**, pokud je modul
  vypnutý,
- zachovat provider, lokality a ostatní nastavení v NVS.

Při vypnutí za běhu se worker nesmí ukončit násilným `vTaskDelete()` z jiného
tasku. Dostane stop request, případnou právě běžící HTTP/TLS operaci bezpečně
dokončí a poté sám uvolní cache, mutex a svůj stack.

Při opětovném zapnutí se WeatherWorker znovu vytvoří z uložené konfigurace.

Implementace byla ověřena na fyzickém zařízení. Vypnutí worker korektně ukončí
a uvolní jeho runtime prostředky; opětovné zapnutí znovu vytvoří worker a načte
všechny nakonfigurované lokality. Současně je ověřen společný memory-heavy gate
mezi DisplayWorkerem a Weather TLS, takže render a TLS již neběží souběžně.

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
