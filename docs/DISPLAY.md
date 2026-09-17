# Displej a refresh strategie

## Ověřený hardware

- panel: černobílý 7,5", 800 × 480 px,
- zadní označení: **DEPG0750BNU790F30HP**,
- druhé označení: **N2104P-0117-01-10323-1**,
- FPC: **FPC-8612**,
- nákup přibližně 01/2021,
- deska: Waveshare e-Paper ESP32 Driver Board,
- přepínač analogové části: **B**,
- druhý přepínač: **ON**,
- knihovna: GxEPD2 1.6.9,
- zvolený driver: GxEPD2_750_T7.

Poloha A dávala na testovaném panelu o něco slabší obraz, proto se používá B.

## Pozorované chování

Přímé vykreslení výsledného obrazu plným waveformem mělo slabou, šedou černou.
Rychlý diferenciální refresh naopak vytvořil tmavší černou, ale při opakovaném
překreslování celého rozložení zanechával duchy a svislé artefakty.

Částečný refresh v současném kódu není malá lokální oblast. EpaperDisplay pro něj
nastavuje partial window na celých 800 × 480 px a znovu vykreslí celou obrazovku.

## Současná strategie

### Start, změna obrazovky a ruční plný refresh

1. celý panel se plným waveformem vyčistí do bíla,
2. výsledná obrazovka se vykreslí diferenciálním waveformem.

Tento dvoufázový postup kombinuje odstranění duchů s lepším výsledným kontrastem.
Na testovaném panelu trvá přibližně 7,3 sekundy.

### Automatická aktualizace

Běžná minutová aktualizace používá diferenciální refresh celé obrazovky a trvá
přibližně 2,0 sekundy. Po pěti po sobě jdoucích částečných obnovách
DisplayManager automaticky provede čisticí plnou obnovu.

Více požadavků vzniklých do pěti sekund po předchozím vykreslení se slučuje.
Požadavek na plný refresh se při sloučení zachová.

## Vzhled

Všechny obrazovky používají společný ScreenStyle:

- černé záhlaví,
- bílý text v tmavých plochách,
- vlevo ikony Wi-Fi (oblouky), GoodWe (solární panel) a AZRouteru (topná spirála),
- připojené zařízení má čistou ikonu, nedostupné šikmé přeškrtnutí přímo přes ikonu;
  při odpojené Wi-Fi jsou přeškrtnuté také obě síťové integrace,
- Wi-Fi ukazuje 1–3 oblouky podle RSSI; při připojení jsou hranice −75 a −67 dBm,
  následně se používá hystereze ±3 dB a potvrzení změny po 5 sekundách;
  odpojení se označí bez tohoto zpoždění, číselné RSSI zůstává v diagnostice,
- změna ustálené úrovně požádá o běžný refresh (s existujícím slučováním požadavků);
  vykreslení používá uloženou úroveň, takže se nezmění mezi stránkami jednoho snímku,
- datum a svátek vpravo používají český Unicode font a lokální UTF-8 kalendář,
- datum se zarovnává před hodiny podle šířky textu; případné zkrácení zachovává celé UTF-8 znaky,
- velké hodiny zůstávají vpravo,
- stejné fonty a vzhled karet.

## Známá omezení

### České písmo na černém pozadí

U8g2_for_Adafruit_GFX 1.8.0 při změně fontu resetuje průhlednost na 0.
Proto `EpaperDisplay::setUnicodeFont()` vždy volá `setFontMode(1)` až po
`setFont()`. Nastavení průhlednosti pouze při inicializaci nestačí: bílé písmo
na implicitně bílém pozadí znaků vytváří bílé obdélníky v černém záhlaví.

Ověřeno hostitelským rastrovým testem skutečné knihovny 1.8.0: všechny znaky
361 položek kalendáře jsou ve fontu `t0_18b_te`; český text při bílé barvě na
černém pozadí je přesným pixelovým opakem černého textu na bílém pozadí.
Test bez opravy reprodukuje chybné obdélníky. Fyzický panel je nutné ověřit
po nahrání firmwaru.

### Refresh

- Refresh běží v samostatné FreeRTOS úloze; jeho stav a délka jsou dostupné přes /api/status.
- DisplayConfig.fullRefreshIntervalMinutes není zapojené do rozhodování.
- Počítadlo částečných obnov se po restartu neuchovává.
- Lokální refresh pouze hodin nebo jednotlivých hodnot není implementovaný.
- Přesná kompatibilita driveru je odvozena z reálného chování panelu; výrobní
  štítek neobsahuje běžné označení Good Display.

## Další ověření

Před změnou waveformu nebo driveru provést:

1. alespoň 24hodinový test minutových aktualizací,
2. opakované přepínání všech obrazovek,
3. kontrolu ghostingu po každé páté částečné obnově,
4. kontrolu kontrastu po studeném startu,
5. zaznamenání okolní teploty při problému.

Vlastní LUT nebo zásah do GxEPD2 má smysl až tehdy, pokud současný dvoufázový
postup nebude dlouhodobě stabilní.
