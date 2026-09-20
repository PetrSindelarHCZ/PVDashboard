# 433 MHz / CC1101 – stav průzkumu a další postup

> Stav k **20. 9. 2026**. Jde o vývojovou diagnostiku na větvi
> `feature/cc1101-diagnostics`, nikoli o potvrzenou funkcionalitu aktuálního
> `masteru`.
>
> Cílem není jen dekódovat jeden senzor, ale postupně **zmapovat všechny
> opakující se 433MHz zdroje v dosahu**, rozlišit skutečné protokoly od šumu a
> teprve potom rozhodnout, které protokoly mají být součástí výsledného řešení.

## Hardware a zapojení

Použitý přijímač je CC1101 pro 433 MHz. Ověřená SPI identifikace:

- `PARTNUM=0x00`,
- `VERSION=0x04`,
- pracovní frekvence pro současný průzkum: **433,92 MHz**,
- režim příjmu: ASK/OOK raw capture přes GDO piny.

Aktuální zapojení CC1101 na Waveshare ESP32 e-Paper Driver Board:

| CC1101 | ESP32 / konektor |
| --- | --- |
| GND | J3-14 GND |
| VCC | J3-1 3,3 V |
| GDO0 | J3-3 / GPIO36 |
| CSN | J4-2 / GPIO23 |
| SCK | J3-15 / GPIO13 |
| MOSI | J3-12 / GPIO14 |
| MISO / GDO1 | J4-8 / GPIO19 |
| GDO2 | J3-4 / GPIO39 |

SCK a MOSI jsou sdílené s e-paperem; CC1101 má vlastní CS a MISO.

## Omezení současného prototypu

Raw RF capture se během fyzického refreshu e-paperu pozastavuje, aby se
neovlivňovala sdílená SPI/periferie. Naměřené výpadky příjmu jsou přibližně:

- partial refresh: **~2,6 s**,
- full refresh: **~9,9 s**.

To je důležitý zdroj ztracených RF paketů. U senzorů vysílajících jednou za
desítky sekund může jediný refresh znamenat vynechaný celý přenos.

Současný CC1101 na hlavním dashboardu proto slouží primárně jako **vývojový
sniffer/prototyp dekodérů**. Dlouhodobá architektura stále počítá s možností
samostatné 433MHz gateway, která nebude mít blackout během e-paper refreshu.

## Diagnostické vrstvy

### `[CC1101][RX]`

Raw pulzy zachycené z rádia. Jsou důležité pro hledání nových protokolů.
Neznámý burst se nesmí automaticky považovat za šum jen proto, že pro něj
zatím neexistuje dekodér.

### `[CC1101][MC]`

Obecný Manchester decoder. Tento řádek není samostatné čidlo; znamená pouze,
že raw burst bylo možné převést na konzistentní Manchester bitstream.
Konkrétní protokol se interpretuje až následně, například jako FT017TH.

### `[CC1101][FT017TH]`

Interpretace známého 65bitového Manchester rámce jako FT017TH.

### `[CC1101][PWM67]` / `[CC1101][PWM67?]`

Pracovní název pro další periodický zdroj. `PWM67` znamená dostatečně čistý
paket pro současný decoder; `PWM67?` znamená charakteristický sync kandidát,
ale poškozenou/neúplnou preambuli nebo data.

## Potvrzený Lidl/SilverCrest senzor – Auriol HG02832 / HG05124A-DCF

Fyzické čidlo označované uživatelem jako Lidl/SilverCrest bylo cíleným capture
prakticky potvrzeno jako rodina **Auriol HG02832 / HG05124A-DCF**.

Zachycený validní rámec:

```text
FD 4C 80 B3 35
```

Dekódované hodnoty:

- ID: **0xFD**,
- teplota: **17,9 °C**,
- vlhkost: **76 %**,
- kanál: **1**,
- baterie: **LOW**,
- TX flag: vypnutý,
- checksum: **platný**.

Uživatel potvrdil, že dekódované hodnoty i stav odpovídají skutečnému čidlu.

Pozorované časování:

- preambule/sync kolem **0,8–0,9 ms**,
- krátký datový HIGH přibližně **0,25–0,30 ms**,
- dlouhý datový HIGH přibližně **0,60–0,66 ms**,
- rámec má **40 bitů**,
- datové pole obsahuje ID, vlhkost, battery/TX/channel flags, teplotu a checksum.

Na diagnostické větvi je implementován decoder s výstupem:

```text
[CC1101][AURIOL] id=0xFD temp=17.9 C humidity=76 % channel=1 battery=LOW tx=OFF | checksum=OK ...
```

Decoder vyžaduje odpovídající preambuli, 40bitový timing a platný checksum; nemá
tedy klasifikovat náhodný RF šum jen podle podobných délek pulzů.

### Co ještě ověřit

- periodu vysílání v běžném provozu,
- reakci `tx=ON` po stisku TX/RESET, pokud jej senzor má,
- změnu channel bitů při přepnutí kanálu,
- stav battery po vložení čerstvých baterií,
- zda ID 0xFD zůstává po výměně baterií stejné,
- případně více kusů stejné rodiny a jejich odlišení podle ID.

## Rodina 1 – FT017TH

### Co je potvrzené

Pozorovaný rámec:

- Manchester kódování,
- délka **65 bitů**,
- typicky 3 opakování stejného rámce; třetí kopie může být zachycena jen jako
  64/65 bitů,
- typická Manchester půlperioda ~**488–491 µs**,
- dvojnásobná délka ~**975–1001 µs**,
- prvních 9 bitů je `1`,
- bity 33..44 nesou teplotu,
- bity 45..56 nesou relativní vlhkost.

Ověřený senzor:

- kandidátní ID/adresa: **0x151**,
- stabilní neznámá část `unkA=0x354A`,
- příklady:
  - 19,8 °C / 61,8 %, `rawT=2154`, `rawH=1986`,
  - 19,7 °C / 62,4 %, `rawT=2150`, `rawH=2005`,
- `unkB` se mění spolu s rámcem a může obsahovat checksum nebo řídicí bity.

Jednou byl zachycen další validně vypadající 65bitový Manchester rámec:

- kandidátní ID **0x1A8**,
- dekódováno přibližně -10,1 °C / 30,9 %,
- měl 3 shodná opakování a validní časování.

**0x1A8 zatím není potvrzený jako skutečný další senzor**, protože nebyla
pozorována pravidelná recidiva.

### Co ještě zjistit

- dlouhodobě potvrdit, zda se `0x1A8` vrací,
- zjistit význam bitů 18..32,
- zjistit význam bitů 57..64 / `unkB`,
- ověřit checksum/CRC,
- najít případné battery-low, channel a pairing/reset bity,
- zachytit změnu po výměně baterie / změně kanálu / resetu senzoru,
- evidovat všechny další stabilní FT017TH ID zvlášť.

## Rodina 2 – pracovní název PWM67

### Co je potvrzené

Jde o skutečný periodický zdroj, nikoli náhodný šum. Pozorované časy přenosů
dávají periodu přibližně **66,3–66,9 s**. Pokud přesný decoder jeden paket
vynechá, další čistý `PWM67` se proto může jevit jako interval ~133,8 s.

Charakteristická preambule:

- přibližně 8×:
  - LOW ~1,84–1,97 ms,
  - HIGH ~0,65–0,78 ms.

Charakteristický sync:

- poslední LOW preambule ~1,8–1,9 ms,
- HIGH ~**7,2 ms**,
- následující LOW je typicky ~**10,2 ms**, ale u slabšího/poškozeného příjmu
  byly vidět i výrazně kratší hodnoty.

Aktuálně dekódovaná datová část používá páry:

- `0` = HIGH krátký ~0,7–0,8 ms + LOW dlouhý ~1,85–1,9 ms,
- `1` = HIGH dlouhý ~1,8–1,9 ms + LOW krátký ~0,7–0,85 ms.

Opakovaně vychází 10bitový segment:

```text
0111100111
```

Poslední `1` může být zachycena dvěma způsoby:

- plně: dlouhý HIGH + dlouhý LOW footer,
- pouze dlouhý HIGH na konci burstu; decoder to označí jako
  `truncated-footer`.

Pozorovaný footer není pevné délky; zatím byl přibližně od **3,1 ms do 12,1 ms**.

### Slabší příjem

Logy silně naznačují, že tento vysílač je vzdálenější nebo má horší RF podmínky:

- někdy chybí část preambule,
- někdy se rozpadne sync LOW,
- datová část přesto často obsahuje správný prefix `011110...`,
- `[PWM67?]` proto často zachytí stejný zdroj, i když přesný decoder selže.

Příklad poškozeného burstu obsahoval čistou preambuli a HIGH sync ~7,18 ms,
ale LOW sync byl jen ~6,14 ms. O kus dále byly přesto znovu čitelné datové
páry odpovídající `011110...`.

### Co ještě zjistit

Nejdůležitější otevřená otázka je, zda současných 10 bitů představuje celý
paket, krátkou hlavičku, nebo jen první stabilní část delšího protokolu.

Příště:

- nasbírat více čistých i poškozených přenosů bez změny decoderu,
- ověřit dlouhodobě periodu,
- sledovat, zda se `0111100111` někdy změní,
- pokud je možné určit fyzický senzor, změnit měřenou teplotu/vlhkost a hledat
  korelované změny v bitstreamu,
- porovnat paket před/po resetu a výměně baterie,
- doplnit tolerantní klasifikaci poškozené preambule, ale zachovat rozdíl mezi
  „plně dekódováno“ a „rozpoznán pouze podpis“,
- nepřidělovat definitivní název/protokol, dokud nebude zařízení nebo formát
  rámce potvrzený.

## Další neznámé signály

V logu jsou i bursty, které neodpovídají FT017TH ani známému podpisu PWM67.

### Krátké chaotické bursty

Často obsahují mnoho pulzů přibližně 70–400 µs bez stabilní preambule nebo
opakujícího se symbolového páru. Část z nich může být slicer chatter / lokální
EMI a může se objevit krátce po skončení skutečného vysílání.

Tyto bursty zatím **nepovažovat automaticky za nový senzor**.

### Dlouhé / strukturované neznámé bursty

V dřívějších capture byly zároveň vidět delší strukturované úseky, například:

- pulzy řádově ~500–950 µs,
- opakující se delší mezery kolem ~16 ms,
- někdy velmi vysoký počet pulzů v jednom burstu.

Tyto skupiny zatím nemají identifikovaný protokol. Mohou obsahovat další
reálný 433MHz vysílač a mají být při dalším průzkumu oddělené od čistého
vysokofrekvenčního chatteru.

## Jak příště hledat další protokoly

Další práce nemá být postavená jen na přidávání ručních dekodérů. Nejdřív je
potřeba z neznámých burstů vytvořit přehled opakujících se rodin.

Doporučený postup:

1. **Dlouhý capture** – nechat zařízení běžet několik desítek minut až hodin,
   ideálně s omezeným e-paper refreshem.
2. **Fingerprint každého neznámého burstu** – počet pulzů, min/max/medián,
   dominantní krátká/dlouhá délka, první sekvence pulzů a čas od předchozího
   podobného burstu.
3. **Seskupování podle časování** – stejné nebo podobné bursty počítat pod
   jedním kandidátním typem místo jejich vypisování jako izolovaný RAW.
4. **Perioda** – sledovat recurrence např. ~30 s, ~45 s, ~60 s, ~67 s,
   několik minut atd.; pravidelná perioda je silný indikátor senzoru.
5. **Opakování uvnitř přenosu** – hledat více kopií stejného rámce stejně jako
   u FT017TH.
6. **Teprve potom decoder** – specifický decoder vytvořit až pro rodinu, která
   se prokazatelně vrací.
7. **Korelace s fyzickým světem** – pokud se podaří najít odpovídající čidlo,
   zahřát/ochladit jej, změnit vlhkost, kanál nebo baterii a sledovat změny
   bitů.
8. **Uchovat důkazní RAW vzorek** – pro každý nový typ mít několik úplných
   raw paketů, nejen odvozený výsledek decoderu.

## Diagnostika, kterou má smysl doplnit

- počítadlo paketů pro každou známou i kandidátní rodinu,
- čas posledního paketu a interval od předchozího,
- počty `OK / weak / corrupted`,
- fingerprint neznámých burstů a počet výskytů stejné skupiny,
- jednoduchý ukazatel kvality založený na úplnosti preambule/syncu a počtu
  validních symbolů; nejde o skutečné RSSI, pokud ho explicitně nečteme z
  CC1101,
- možnost omezit spam raw výpisů a přitom zachovat reprezentativní vzorek,
- dlouhodobé síťové logování/capture, aby průzkum nevyžadoval připojený Serial
  Monitor.

## RF podmínky

Pro 433,92 MHz je čtvrtvlnný přímý vodič přibližně **17,3 cm**. Při slabém
příjmu ověřit zejména:

- správnou anténu a její orientaci,
- odstup přijímače/antény od ESP32, e-paperu a napájecích vodičů,
- vliv lokálních zdrojů EMI,
- zda se počet `weak/corrupted` paketů mění po přesunutí zařízení.

## Nejbližší pokračování

Při příštím sezení nezačínat znovu identifikací PWM67. Nejprve:

1. ověřit několik dalších cyklů PWM67 a zachovat současné timing údaje,
2. doplnit seskupování/fingerprint **všech neznámých raw burstů**,
3. z delšího logu vybrat další opakující se rodiny signálů,
4. zvlášť prozkoumat strukturované bursty s jiným časováním než FT017TH/PWM67,
5. teprve pak hledat jejich známé protokoly nebo psát nové dekodéry.

Tím zůstane průzkum otevřený i pro další domácí nebo sousední 433MHz senzory a
nezaměří se předčasně jen na dva dnes rozpoznané zdroje.


## Připravený domácí test: Hyundai + SilverCrest/Lidl

K dispozici jsou dvě fyzická čidla pro cílený laboratorní test:

1. **Hyundai** – přesný model zatím není potvrzen.
2. **SilverCrest / Lidl** – fyzické čidlo už bylo RF capturem potvrzeno jako
   rodina **Auriol HG02832 / HG05124A-DCF**; viz potvrzený decoder výše.

Při příštím testu je ideální nejprve vyfotit zadní štítek obou čidel
(model, IAN, FCC/CE údaje, přepínač kanálu, tlačítko TX/RESET). Samotný RF test
ale může začít i bez přesného modelu.

### Hyundai – fyzické čidlo je teplota + vlhkost

Fyzické Hyundai čidlo u uživatele zobrazuje **teplotu i vlhkost**. To znamená,
že nejde považovat starý rtl_433 protokol **Hyundai WS SENZOR Remote
Temperature Sensor** za potvrzenou shodu; ten přenáší pouze teplotu,
battery/startup, channel a ID.

Velmi pravděpodobným modelem je rodina **Hyundai WS Senzor 77 TH** nebo
kompatibilní varianta: 433,9 MHz, CH1-CH3, teplota + vlhkost. Přesný RF formát
zatím není známý a musí být odvozen z reálného capture.

Pro srovnání starý rtl_433 Hyundai WS protokol očekává:

- frekvence: **433,92 MHz**,
- perioda vysílání: přibližně **33 s**,
- modulace: OOK PPM / distance coding,
- vlastní RF pulse přibližně **224 µs**,
- mezera pro bit 0 přibližně **1032 µs**,
- mezera pro bit 1 přibližně **1992 µs**,
- mezera mezi opakovanými pakety přibližně **4016 µs**,
- zpráva má **24 bitů**,
- stejná zpráva se v jednom přenosu opakuje až **23×**.

Datové pole:

```text
TTTTTTTT TTTTBSCC IIIIIIII
```

kde:

- `T` = signed teplota ×10 v °C,
- `B` = stav baterie,
- `S` = startup / vložení baterie / TX tlačítko,
- `CC` = kanál 1–3,
- `I` = 8bit ID senzoru.


Na diagnostické větvi je nyní přidán experimentální decoder
`[CC1101][HYUNDAI]`. Nemění globální RF framing ani ISR; místo toho skenuje
i dlouhý/overflow raw snapshot a hledá:

- 24 PPM bitů s krátkým HIGH pulzem a LOW mezerou ~1/2 ms,
- následný ~4ms packet separator,
- minimálně **4 identická opakování stejného 24bitového rámce**.

Požadavek na opakování je důležitý, protože Hyundai WS protokol nemá checksum.
Pokud se hypotéza potvrdí, výstup bude například:

```text
[CC1101][HYUNDAI] id=0x.. temp=.. C channel=.. battery=.. startup=.. repeats=.. | raw=.. .. ..
```

Teprve po reálném zachycení fyzického Hyundai senzoru má být protokol označen
jako potvrzený.

To je velmi užitečný fingerprint. Pokud po vložení baterie uvidíme zhruba
každých 33 s dlouhý burst tvořený množstvím krátkých ~0,2ms pulzů a mezer
~1/2 ms a zároveň opakování 24bitového rámce, je pravděpodobnost shody velmi
vysoká.

Poznámka: některé dříve zachycené „chaotické“ raw bursty obsahovaly hodně
krátkých pulzů a delší mezery. Zpětně proto není bezpečné všechny podobné
bursty označovat za šum; část může být protokol typu Hyundai WS. Ověřit až
cíleným testem s jediným zapnutým čidlem.

### SilverCrest / Lidl / Auriol – známé kandidátní rodiny

Bez modelového označení nelze vybrat jediný protokol. V rtl_433 existuje více
Lidl/Auriol rodin, mimo jiné:

#### Auriol AFW 2 A1, IAN 311588

- ~60 s mezi přenosy,
- 36 bitů,
- 12 identických opakování,
- OOK PPM,
- krátká/dlouhá vzdálenost přibližně 576 / 1536 µs,
- obsahuje ID, baterii, TX tlačítko, kanál, teplotu a vlhkost.

#### Auriol HG02832 / HG05124A-DCF

- OOK PWM,
- krátký pulz ~252 µs,
- dlouhý ~612 µs,
- sync ~860 µs,
- 40bitový rámec,
- obsahuje ID, vlhkost, battery/TX/channel flags, teplotu a checksum,
- mezi pakety je velmi dlouhá mezera přibližně 61 ms.

#### Auriol HG04641A, Lidl IAN 307350

- OOK PPM,
- fixní pulse přibližně 510 µs,
- mezera ~980 µs pro 0 a ~1976 µs pro 1,
- 36bitový rámec,
- stejná zpráva se opakuje 4×,
- obsahuje 16bit ID, battery flag, teplotu a 4bit checksum.

#### Auriol AFT 77 B2

- 68 bitů,
- alespoň 3 opakování,
- před paketem 9 sync pulzů kolem 1900 µs,
- datový pulse ~488 µs,
- mezera ~488 µs pro 0 a ~976 µs pro 1,
- obsahuje ID, flags, znaménko, BCD teplotu a dvě integrity hodnoty.

#### Auriol AHFL 433B2 IPX4

- 42 bitů,
- OOK PPM,
- typické vzdálenosti přibližně 2,1 / 4,15 ms,
- obsahuje ID, baterii, TX tlačítko, kanál, teplotu, vlhkost a checksum.

Existují i další Auriol/SilverCrest varianty. Proto je model/IAN ze štítku
nejrychlejší cesta k přesnému dekodéru.

### Doporučený testovací postup

Aby bylo možné každý RF podpis přiřadit bez pochybností:

1. **Obě čidla bez baterií.**
   Nechat CC1101 několik minut běžet jako baseline a zaznamenat okolní
   FT017TH, PWM67 a ostatní provoz.

2. **Pouze Hyundai.**
   Vložit baterie a zaznamenat přesný čas.
   Pokud má TX/RESET tlačítko, jednou jej stisknout.
   Nechat běžet alespoň 3–5 minut, aby byly vidět opakované periody.

3. **Hyundai – změna teploty.**
   Zahřát čidlo v ruce nebo jej na chvíli přesunout do chladnějšího prostoru.
   Nesnažit se o extrémní teploty; cílem je pouze několik stupňů změny.
   Pokud lze přepnout kanál 1/2/3, zachytit každý kanál zvlášť.

4. **Hyundai vypnout.**
   Vyjmout baterii a chvíli ověřit, že nově nalezená periodická rodina zmizela.

5. **Pouze SilverCrest/Lidl.**
   Stejný postup: vložení baterie, případný TX/RESET, několik minut capture,
   změna teploty/vlhkosti a případně změna kanálu.

6. **Nakonec obě čidla současně.**
   Ověřit, že je decoder/fingerprint umí odlišit i při překrývajícím se
   běžném provozu.

### Co při testu zapisovat

Pro každý zásah si poznamenat čas alespoň na sekundy:

```text
19:10:00 Hyundai – vložena baterie
19:10:15 Hyundai – TX stisk
19:12:00 Hyundai – zahřívám v ruce
19:14:00 Hyundai – vyjmuta baterie

19:16:00 SilverCrest – vložena baterie
...
```

Pak lze RF log časově korelovat bez hádání.

### Co má firmware před testem umět

Současná diagnostická větev už poskytuje raw bursty a dekodéry FT017TH/PWM67.
Další vhodný krok před cíleným domácím testem:

- přidat fingerprint kandidátů typu **Hyundai WS**,
- u neznámých burstů evidovat dominantní pulzy a počet opakovaných bloků,
- zachovat alespoň jeden úplný RAW vzorek každého nového fingerprintu,
- neskrývat burst jen proto, že má mnoho krátkých pulzů,
- později podle modelového štítku přidat konkrétní Auriol/SilverCrest decoder.

Implementaci konkrétního Hyundai decoderu je vhodné dokončit až po prvním
cíleném capture, aby se potvrdilo, že fyzické čidlo je skutečně rodina
Hyundai WS SENZOR a ne jiný model prodávaný pod stejnou značkou.


### Diagnostika pro Hyundai TH

Po testu, kdy LED fyzického Hyundai čidla prokazatelně blikla a experimentální
24bitový Hyundai-WS decoder nic nenašel, byla hypotéza starého temperature-only
protokolu oslabena.

Raw buffer zůstává na **768 pulsech**, protože jeho zdvojnásobení přeteklo interní DRAM ESP32. Dlouhé bursty
se už neoznačují automaticky jako `[NOISE]`, ale jako `[CC1101][RX][LONG]`
a kromě statistik vypíšou prvních **96 H/L pulzů** i při overflowu. Cíl je získat timingový
fingerprint neznámého Hyundai TH přenosu bez změny globálního burst framingu
a bez zásahu do Auriol/PWM67 decoderů.

Při dalším cíleném testu:
1. sledovat bliknutí LED Hyundai,
2. poznamenat přibližný čas,
3. zachytit nejbližší `[LONG]` burst,
4. porovnat opakované timingové bloky,
5. teprve poté vytvořit skutečný WS 77 TH decoder.


## Neidentifikované Nexus-TH čidlo v dosahu

Nový dlouhý capture ukázal velmi čistý PPM přenos rodiny Nexus-TH:

- HIGH pulz přibližně 0,45–0,56 ms,
- LOW ~1,0 ms = 0,
- LOW ~2,0 ms = 1,
- LOW ~4,0 ms = oddělovač opakovaného rámce,
- rámec má 36 bitů a opakuje se vícekrát.

První rekonstruovaný rámec:

```text
010101111001000010100110111101001100
hex: 0x5790A6F4C
```

Struktura sedí přesně na rodinu **Nexus-TH**:

```text
[id0][id1][flags][temp0][temp1][temp2][0xF][humi0][humi1]
```

Dekódované hodnoty z rámce:

- ID: 0x57
- kanál: 2
- battery: OK
- test: OFF
- teplota: 16,6 °C
- vlhkost: 76 %

Na diagnostické větvi je přidán decoder `[CC1101][NEXUS-TH]`, který vyžaduje
minimálně 3 identická 36bitová opakování, konstantní nibble 0xF a validní rozsah
kanálu/vlhkosti. Tím se omezuje riziko false-positive, protože Nexus-TH nemá
skutečný checksum.

Tento rámec byl původně pracovně přiřazen fyzickému Hyundai senzoru, ale uživatel
potvrdil, že Hyundai v danou chvíli ukazoval přibližně 24 °C. Dekódovaných 16,6 °C
proto Hyundai neodpovídá. Jde tedy o jiné Nexus-TH kompatibilní čidlo v dosahu.

Hyundai WS Senzor 77 TH zůstává neidentifikovaný a jeho skutečný protokol je třeba
odvodit z cíleného capture po stisku RESET / při vložení baterií, ideálně s poznámkou
okamžiku bliknutí LED.


## Hyundai WS Senzor 77 TH – potvrzený TFA Twin Plus / KW9010 protokol

Další průzkum ukázal silnou OEM stopu na výrobce **Carrin Electronics**.
Řada Carrin používá stejné konstrukční prvky jako Hyundai WS Senzor 77 TH
(displej čidla, CH1-CH3, RESET, °C/°F, 433 MHz, 2x AAA, podobné rozměry).

V rtl_433 je přímo podporován Carrin/Conrad model **KW9010** pod protokolem:

- TFA Twin Plus 30.3049,
- Conrad KW9010,
- Ea2 BL999.

Tento protokol používá:

- OOK/PPM,
- 36 bitů,
- LOW gap ~2 ms = 0,
- LOW gap ~4 ms = 1,
- oddělení opakovaných rámců ~6-10 ms,
- teplotu, vlhkost, ID, kanál, stav baterie,
- nibble checksum.

Na diagnostické větvi je decoder `[CC1101][TFA-TWIN]`.

Fyzické Hyundai WS Senzor 77 TH bylo nyní potvrzeno jako kompatibilní s tímto
protokolem. Zachycený rámec:

```text
[CC1101][TFA-TWIN] id=0x2F temp=24.6 C humidity=20 % channel=1 battery=OK repeats=4 checksum=OK | packets=2 interval=32.1 s | raw=F546F00DD
```

Uživatel potvrdil, že zjištěná teplota, vlhkost i kanál odpovídaly skutečnému
stavu senzoru. Tím je identifikace uzavřena:

- fyzický model: Hyundai WS Senzor 77 TH,
- RF rodina: TFA Twin Plus 30.3049 / Conrad KW9010 / Ea2 BL999,
- modulace: OOK/PPM,
- délka rámce: 36 bitů,
- přenáší: ID, channel, battery, temperature, humidity, checksum,
- pozorovaný interval: přibližně 32 s,
- ID se může po resetu / výměně baterií změnit.

Tag `[TFA-TWIN]` zůstává záměrně generický, protože stejný protokol používá
více přeznačených senzorů.


## Carrier-sense diagnostika dlouhých burstů

Pro rozlišení skutečného RF provozu od chatteru datového sliceru byla doplněna
diagnostika GDO2 carrier-sense přímo v ISR.

U každého dlouhého burstu se nyní vypisuje například:

```text
cs=650/700 (93%)
```

První hodnota je počet zachycených hran při aktivním GDO2 carrier-sense,
druhá je součet carrier-high + povolených hran během krátkého carrier-hold okna.
Vyšší podíl znamená silnější důkaz, že burst vznikl při skutečně detekovaném RF
nosném signálu; nízký podíl ukazuje spíš na přechody sliceru během hold okna.

Současně se dlouhé bursty orientačně klasifikují podle timingového tvaru:

- `shape=TFA-2/4ms?` pro HIGH přibližně 0,3-0,8 ms a LOW mezery ~2/4 ms,
- `shape=NEXUS-1/2ms?` pro HIGH přibližně 0,3-0,8 ms a LOW mezery ~1/2 ms,
- `shape=mixed` pro ostatní provoz.

Toto je pouze fingerprint pro výzkum, nikoli plnohodnotný decoder.
Číselné RSSI zatím není čteno: CC1101 sdílí SPI s e-paperem a RSSI odečtené až
po 18ms konci burstu by většinou reprezentovalo šumové pozadí, nikoli sílu
ukončeného paketu.
