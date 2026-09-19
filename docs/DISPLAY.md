# Displej a refresh strategie

## Hardware

- panel: černobílý 7,5", 800 × 480 px,
- zadní označení: **DEPG0750BNU790F30HP**,
- další označení: **N2104P-0117-01-10323-1**,
- FPC: **FPC-8612**,
- deska: Waveshare e-Paper ESP32 Driver Board,
- analogový přepínač: **B**,
- druhý přepínač: **ON**,
- knihovna: GxEPD2 1.6.9,
- driver: GxEPD2_750_T7.

Poloha A dávala na testovaném panelu slabší obraz, proto se používá B.

## Refresh politika

Přímý full waveform vytvářel slabší černou, zatímco diferenciální refresh měl
lepší kontrast, ale při dlouhodobém opakování může vytvářet ghosting. Aktuální
full refresh proto probíhá dvoufázově:

1. celý panel se plným waveformem vyčistí do bíla,
2. výsledná obrazovka se vykreslí diferenciálním waveformem.

Full refresh se používá při startu, změně obrazovky a ručním požadavku.
Běžné aktualizace stejné obrazovky používají diferenciální partial refresh celé
plochy 800 × 480. **Počet partial refreshů sám o sobě full refresh nevyvolává.**

Více požadavků vzniklých krátce po sobě se slučuje a full požadavek má přednost.

Fyzická joysticková navigace má optimalizovanou cestu: při změně focusu se
předává dirty region starého/nového kurzoru a Sidebar používá pouze levý pruh.
Přepnutí obrazovky z joysticku používá celoplošný differential partial refresh;
čistící full refresh zůstává pro start a explicitní požadavky. Mezikroky fyzické
navigace navíc nepřegenerovávají celý serverový preview snapshot.

Dlouhodobý 24hodinový test ghostingu této politiky zůstává otevřený.

## DisplayWorker a preview

DisplayWorker je jediným vlastníkem DisplayManageru a fyzického e-paperu.
Render běží mimo hlavní smyčku, takže WebUI zůstává dostupné i během pomalé
obnovy. Stav je publikovaný přes /api/status.

Serverový náhled posledního vyrenderovaného snímku je dostupný přes
/api/display.bmp. Preview se připravuje po pruzích 800 × 60 px místo velkého
souvislého canvasu; pracovní buffer tak zůstává malý a poslední platný snapshot
lze zachovat i při neúspěchu nového capture.

## Paměť a Weather TLS

ESP32-WROOM-32 nemá PSRAM. Největším rizikem proto není jen celkový free heap,
ale velikost souvislého bloku interní DRAM.

DisplayWorker a WeatherWorker sdílejí memory-heavy gate:

- Display jej drží během inicializace, renderu a tvorby preview,
- Weather jej drží během HTTPS/TLS fetchu,
- weather task při čekání průběžně kontroluje stop request,
- worker se při vypnutí počasí nemaže násilně uprostřed TLS operace.

Díky tomu se render/preview a TLS nespouštějí současně.

## Vzhled a navigace

Všechny obrazovky používají společný ScreenStyle:

- černé záhlaví,
- vlevo Wi-Fi a pouze aktivní energetické zdroje,
- datum, svátek a čas vpravo,
- časová část se zobrazí až po první platné NTP synchronizaci od bootu,
- levý sidebar obsahuje jen aktuálně registrované moduly,
- aktivní obrazovka a navigační focus jsou dva samostatné stavy,
- focus se kreslí podle NavigationControlleru,
- Weather při více lokalitách zobrazuje pager teček a při focusu název lokality.

Wi-Fi ikona používá tři úrovně signálu s hysterezí; recovery AP je označené
malým AP. Číselné RSSI zůstává v diagnostice.

## České písmo

U8g2_for_Adafruit_GFX při změně fontu resetuje režim průhlednosti. Proto
EpaperDisplay po změně Unicode fontu znovu nastavuje průhledný font mode.
To je nutné pro správné bílé české znaky na černém záhlaví.

## Otevřené body

- alespoň 24hodinový test minutových aktualizací a ghostingu,
- rozhodnutí o nepoužívaném fullRefreshIntervalMinutes,
- lokální refresh pouze hodin nebo jednotlivých hodnot zatím není implementovaný.

Před změnou waveformu nebo driveru nejprve zopakovat dlouhodobý test, přepínání
všech obrazovek, studený start a zaznamenat okolní teplotu při problému.
Vlastní LUT má smysl až pokud současný dvoufázový postup nebude stabilní.
