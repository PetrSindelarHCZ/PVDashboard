# Roadmapa

Tento dokument obsahuje pouze **budoucí práci**. Aktuální stav a hotové funkce
jsou v [PROJECT_STATUS.md](PROJECT_STATUS.md); technické principy v
[ARCHITECTURE.md](ARCHITECTURE.md).

## A — dokončení současné platformy

Nejbližší priority:

- [ ] provést alespoň 24hodinový test e-paper ghostingu a stability,
- [ ] podle výsledku odstranit nebo smysluplně zapojit fullRefreshIntervalMinutes,
- [ ] rozšířit host-side testy GoodWe, AZRouteru, konfigurace a verzování,
- [ ] sjednotit WebUI stav vypnutých GoodWe/AZRouter na Vypnuto místo Offline,
- [ ] rozhodnout, zda YAML backup rozšířit o celý seznam známých Wi-Fi sítí
      včetně autoConnect.

## B — fyzické ovládání

Softwarová navigace Sidebar / Pager / Page a WebUI joystick jsou hotové.
Zbývá fyzická vrstva:

- [x] zvolit GPIO pro pětisměrný joystick,
- [x] napojit pětisměrný joystick do NavigationControlleru,
- [x] implementovat debounce,
- [ ] ověřit pinout a chování na fyzickém zařízení,
- [x] implementovat auto-repeat směrových tlačítek,
- [ ] podle potřeby doplnit samostatné long-press akce,
- [ ] definovat akce OK nad konkrétními prvky,
- [ ] podle potřeby doplnit ruční override navigace pro atypický layout.

Aktuální joystick pinout je UP GPIO16, DOWN GPIO17, LEFT GPIO18, RIGHT GPIO32
a OK GPIO33. Všechny vstupy používají interní pull-up a tlačítka spínají proti
GND. SET a RESET zatím nejsou součástí firmware. Pro I²C čidla zůstává pár
SDA GPIO21 / SCL GPIO22.

## C — skutečná data domácnosti

### Vnitřní prostředí

- [x] implementovat BME280 přes I²C (SDA GPIO21 / SCL GPIO22, adresy 0x76/0x77),
- [x] napojit teplotu, vlhkost a tlak do DataModelu,
- [ ] fyzicky připojit 4pinový modul a ověřit měření na cílové desce.


### Bazén

- [ ] vytvořit samostatný Wi-Fi uzel u bazénu,
- [ ] použít vodotěsný DS18B20 jako první reálné čidlo,
- [ ] nahradit demonstrační hodnoty Pool obrazovky skutečnými daty,
- [ ] zachovat modulární API pro další budoucí parametry bazénu.

## D — konfigurovatelné obrazovky

Cílem je datový layout, nikoli obecný HTML/CSS framework v ESP32.

- [x] definovat podporovanou množinu widgetů,
- [x] navrhnout datový popis pozice, velikosti a parametrů,
- [x] renderovat stejný popis na fyzickém e-inku,
- [x] vytvořit editor layoutu ve WebUI s využitím existujícího preview,
- [x] zahrnout layout do exportu/importu konfigurace,
- [x] umožnit reset jednotlivé obrazovky na výchozí šablonu,
- [x] přidat vlastní Home widgety skládající se z Text/KPI/Progress/Graf prvků,
- [x] přidat vnořený editor vlastních prvků s mřížkou, drag/resize a datovými vazbami,
- [ ] rozšířit stejný model konfigurovatelného layoutu i na další obrazovky mimo Home.

## E — vzdálená čidla

Pro delší dosah a levná venkovní čidla je plánovaný samostatný gateway:

- [ ] prototyp s CC1101 433 MHz,
- [ ] definovat jednoduché lokální API gateway → dashboard,
- [ ] rozhodnout Ethernet/PoE vs Wi-Fi podle umístění,
- [ ] integrovat první venkovní teplotní čidlo.

Hlavní dashboard nemá být zatěžovaný průběžným dekódováním různých rádiových
protokolů; gateway má zůstat samostatná.

## F — historie a KPI

- [ ] definovat ukládané veličiny a intervaly,
- [ ] navrhnout krátkodobý lokální buffer odolný vůči výpadku internetu,
- [ ] přidat hodinové, denní a měsíční agregace,
- [ ] zobrazit historické grafy přes stávající lehký EInkGraph,
- [ ] případně přidat nezávislou cloudovou/backend vrstvu.

Cloud nesmí být podmínkou pro lokální funkci dashboardu a firmware nemá být
těsně svázaný s jedním poskytovatelem.


## H — logování, diagnostika a odolnost komunikace

Cílem je diagnostikovat dlouhodobý provoz bez nutnosti připojeného Serial Monitoru
a současně omezit potřebu restartovat celé ESP při výpadku jedné integrace.

- [ ] zavést jednotné logovací API s úrovněmi Error / Warning / Info / Debug / Trace,
- [ ] podporovat více výstupů stejného logu: Serial, WebUI ring-buffer a vzdálený Syslog,
- [ ] přidat do WebUI konfiguraci log levelu, povolených výstupů a adresy/portu Syslog serveru,
- [ ] udržovat v RAM omezený ring-buffer posledních zpráv pro stránku WebUI → Logs,
- [ ] neposílat běžné provozní logy průběžně do flash; vzdálenou historii řešit přes síť,
- [ ] automaticky přidávat k důležitým záznamům firmware verzi, uptime a diagnostiku paměti,
- [ ] při startu logovat reset reason a případnou informaci o předchozím panic/watchdog resetu,
- [ ] doplnit detailní GoodWe diagnostiku TX/RX: typ požadavku, délku odpovědi, dobu odezvy,
      timeouty, CRC chyby a počet po sobě jdoucích selhání,
- [ ] evidovat čas poslední úspěšné GoodWe komunikace, počet reconnectů a stav klienta/socketu,
- [ ] při problému s GoodWe současně logovat stav AZRouteru, Wi-Fi/RSSI a free heap,
      aby bylo možné odlišit lokální problém GoodWe od obecného síťového problému ESP,
- [ ] ověřit, zda se GoodWe požadavky nepřekrývají a zda je vždy dokončen jeden request
      před odesláním dalšího,
- [ ] implementovat stupňovaný GoodWe recovery mechanismus: opakování požadavku →
      znovuvytvoření socketu/klienta → nová inicializace komunikace,
- [ ] restart celého ESP použít až jako poslední nouzový krok po selhání izolovaného recovery,
- [ ] ověřit dlouhodobým testem reálného GoodWe stav, kdy GoodWe přestane odpovídat,
      zatímco AZRouter pokračuje v normální komunikaci.

Lokální komunikace GoodWe nevyžaduje uživatelské přihlášení; diagnostika se proto
má soustředit na stav lokálního protokolu, pořadí požadavků, socket/klient,
časování a případné chyby nebo degradaci Wi-Fi modulu měniče.

## G — volitelné / k dalšímu průzkumu

- [ ] ČHMÚ provider, pouze pokud půjde bezpečně omezit objem regionálních dat,
- [ ] Home Assistant jako volitelná integrační vrstva, nikoli závislost,
- [ ] lokální refresh menších částí panelu, pokud jej odůvodní dlouhodobý test,
- [ ] migrace na ESP32-S3 s PSRAM až pokud současný ESP32-WROOM-32 narazí na
      skutečný paměťový nebo funkční limit.

Případná změna kontroleru se má rozhodovat až podle reálného limitu. Současný
7,5" 800 × 480 panel a lokální provoz zůstávají výchozím cílem projektu.
