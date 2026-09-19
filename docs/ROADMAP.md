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

- [ ] vybrat a ověřit GPIO konkrétní revize Waveshare desky,
- [ ] připojit pětisměrný joystick,
- [ ] implementovat debounce,
- [ ] implementovat long-press / auto-repeat,
- [ ] definovat akce OK nad konkrétními prvky,
- [ ] podle potřeby doplnit ruční override navigace pro atypický layout.

Pro I²C čidla je preferovaný pár SDA GPIO21 / SCL GPIO22. Dříve zvažované
GPIO pro tlačítka jsou 16, 17, 18, 19, 23, 32 a 33; nejde o finální pinout a
před zapojením je nutné znovu ověřit boot-strapping a vazby konkrétní desky.

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

## G — volitelné / k dalšímu průzkumu

- [ ] ČHMÚ provider, pouze pokud půjde bezpečně omezit objem regionálních dat,
- [ ] Home Assistant jako volitelná integrační vrstva, nikoli závislost,
- [ ] lokální refresh menších částí panelu, pokud jej odůvodní dlouhodobý test,
- [ ] migrace na ESP32-S3 s PSRAM až pokud současný ESP32-WROOM-32 narazí na
      skutečný paměťový nebo funkční limit.

Případná změna kontroleru se má rozhodovat až podle reálného limitu. Současný
7,5" 800 × 480 panel a lokální provoz zůstávají výchozím cílem projektu.
