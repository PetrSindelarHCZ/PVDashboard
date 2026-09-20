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
