# Integrace

Tento dokument popisuje externí datové zdroje PVDashboardu: GoodWe, AZRouter
a poskytovatele počasí. Stav hotových funkcí je v [PROJECT_STATUS.md](PROJECT_STATUS.md);
budoucí práce v [ROADMAP.md](ROADMAP.md).

## GoodWe GW10K-ET

- transport: UDP, standardně port 8899,
- protokol: Modbus RTU rámec zapouzdřený v UDP,
- unit ID: 0xF7,
- funkce: 0x03,
- první registr: 35100,
- počet registrů: 125,
- odpověď může mít prefix AA 55,
- klient validuje délku, zdrojový host/port, funkci a CRC16.

Hlavní mapování do DataModelu:

| DataModel | Zdroj |
| --- | --- |
| productionPowerW | součet ppv1 a ppv2 |
| energyTodayKWh | e_day, registr 35193, měřítko 0,1 kWh |
| batteryPowerW | pbattery1, registr 35182 |
| batterySocPercent | zatím odhad z napětí baterie |
| gridPowerW | Active Power Meter, registr 35140 |
| houseConsumptionW | výpočet z toků, případně load_ptotal |

Klient provede nejvýše dva pokusy po 700 ms. Při nedostupnosti proto jeden
polling blokuje hlavní smyčku nejvýše přibližně 1,4 sekundy.

Používaná znaménka:

- gridPowerW: kladné = přetok, záporné = odběr,
- batteryPowerW: kladné = vybíjení, záporné = nabíjení.

## AZRouter

- transport: lokální HTTP API,
- volitelná autentizace používá stejné jméno a heslo jako AZRouter WebUI,
- klient nejprve volá `POST /api/v1/login` s payloadem
  `{"data":{"username":"...","password":"..."}}`,
- pokud login vrátí token, další požadavky posílají `Authorization: Bearer ...`,
- pokud login vrátí session cookie, klient ji posílá přes `Cookie`,
- při HTTP 401/403 se session zahodí, provede jeden re-login a požadavek se jednou zopakuje,
- bez uložených credentials zůstává zachováno anonymní čtení pro firmware, který jej dovoluje,
- standardní port: 8081,
- /api/v1/power:
  - `output.power` id 0–2 = vytěžený výkon L1/L2/L3,
  - `output.power` id 3 = celkový vytěžený výkon,
  - `output.energy` id 0–4 = celkem / rok / měsíc / týden / dnes,
  - `input.power` id 0–2 = výkon sítě L1/L2/L3; dashboard z nich skládá součet,
  - `input.current` id 0–2 = proud L1/L2/L3 v mA,
  - `input.voltage` id 0–2 = napětí L1/L2/L3 v mV,
  - `input.status` id 0–2 = stav připojení jednotlivých fází,
- /api/v1/status:
  - `system.temperature` = teplota elektroniky/master jednotky; **není to bojler**,
- /api/v1/devices:
  - endpoint se nadále načítá kvůli diagnostice a budoucímu rozšíření,
  - údaje připojeného zařízení se ale **nepoužívají jako masterová data AZRouteru**,
  - zejména `devices.power.totalPower` už nepřepisuje `output.power[3]`.

Každá AZRouter veličina má samostatný příznak platnosti. FVE UI používá
masterová data z `/power` a `/status`; device-level informace se na dashboardu
nezobrazují.

Za úspěch celé aktualizace se považuje platná odpověď /api/v1/power s alespoň
jednou rozpoznanou výkonovou/energetickou veličinou. Connect timeout je 400 ms
a timeout odpovědi 1 000 ms. Když hlavní endpoint selže, doplňkové dotazy se v
daném cyklu neprovedou. Jejich samostatné selhání nezneplatní již přijatá
výkonová data.

`DataModel` už neinicializuje GoodWe ani AZRouter demonstračními hodnotami.
Před prvním úspěšným pollingem proto FVE UI zobrazuje nedostupné hodnoty.

AZRouter username/password se ukládají pouze do lokální NVS. WebUI nikdy
nevrací uložené heslo zpět do prohlížeče; prázdné heslo při uložení znamená
zachovat stávající. YAML export záměrně AZRouter credentials neobsahuje.

## Společné chování GoodWe a AZRouteru

Oba zdroje mají nezávislé nastavení aktivace, hostu, portu a polling intervalu.
Nikdy se neslučují do jednoho společného FVE hostu, i když vývojový simulátor
může obě služby provozovat na jedné IP adrese.

Při vypnutí zdroje:

- zdroj se nepolluje,
- jeho stavová ikona se nekreslí,
- diagnostika rozlišuje Vypnuto od nedostupného aktivního zdroje,
- FVE obrazovka zůstane jen pokud je aktivní alespoň jeden energetický zdroj.

Při vypnutí obou zdrojů se FVE odstraní ze ScreenManageru i sidebaru. Pokud byla
právě aktivní, dashboard přejde na Home.

FVE obrazovka používá obecný pager:
- **Přehled** — společné KPI GoodWe + AZRouter,
- **GoodWe** — výroba, baterie, distribuce a denní graf,
- **AZRouter** — master stav, L1/L2/L3, vytěžování a uložená energie.

Rozložení SolarScreen se přizpůsobuje režimu GoodWe + AZRouter, pouze GoodWe nebo pouze AZRouter.

Každý aktivní zdroj publikuje kromě hodnot také dostupnost, čas posledního
úspěchu/pokusu, počet chyb a poslední chybu. Při výpadku zůstávají v DataModelu
poslední platné hodnoty, proto UI musí současně respektovat dostupnost nebo stáří.
Po chybě se interval dalšího pokusu zvětšuje exponenciálním backoffem a první
úspěch obnoví běžný polling interval.

Vývojový simulátor je samostatný projekt **Dashboard.DeviceSimulator**:
GoodWe UDP/8899, AZRouter HTTP/8081 a ovládací WebUI/API simulátoru HTTP/8080.

## Počasí

WeatherWorker běží mimo hlavní smyčku a používá společné rozhraní
IWeatherProvider. Provider převádí externí odpověď do společného WeatherData,
takže renderery nejsou závislé na konkrétním API.

### Open-Meteo

- endpoint: https://api.open-meteo.com/v1/forecast,
- aktuální hodnoty, denní agregace i hodinová data v jednom požadavku,
- časová zóna auto,
- geocoding pro vyhledávání lokalit ve WebUI.

### MET Norway

- endpoint: https://api.met.no/weatherapi/locationforecast/2.0/compact,
- identifikační User-Agent,
- podmíněné požadavky přes If-Modified-Since,
- respektování serverových hlaviček Date a Expires,
- převod symbolů do společného modelu,
- pravděpodobnost srážek je volitelná.

### Lokality a pager

Konfigurace podporuje nejvýše **8 lokalit**. Každá má stabilní ID, název,
stát a souřadnice. Persistentní activeLocationId určuje lokalitu používanou
Home obrazovkou.

WebUI umožňuje lokalitu vyhledat, přidat, vybrat, odstranit a změnit její
pořadí. Pořadí je uložené v NVS a zároveň určuje pořadí podstránek Weather
pageru. Přepnutí pageru nemění persistentní activeLocationId.

WeatherWorker udržuje cache podle ID lokality. Hlavní Weather obrazovka při více
lokalitách zobrazuje tečky pageru a při focusu také název právě volené lokality.
Hodinové obrazovky weather-hourly-0 až weather-hourly-3 jsou samostatná vrstva
pro výběr dne předpovědi.

### Lifecycle a paměť

Při weather.enabled=false se WeatherWorker ani jeho mutex/cache zbytečně
neudržují. Při vypnutí za běhu dostane worker požadavek na korektní ukončení;
probíhající TLS operace se bezpečně dokončí a teprve potom se uvolní task,
cache a mutex. Weather obrazovky se současně odregistrují.

HTTPS používá ověřené kořenové certifikáty, ne režim setInsecure(). Weather TLS
a render/preview displeje sdílejí memory-heavy gate, aby se na ESP32 bez PSRAM
nepřekrývaly největší nároky na souvislou interní DRAM.

### ČHMÚ

ČHMÚ zůstává plánovaným providerem. Veřejná data nejsou jednoduchý bodový
endpoint; případný adaptér musí určit správný regionální soubor, omezit objem
dat a převést výsledek do stejného WeatherData bez překročení paměťových limitů
ESP32.

## Známý drobný UI dluh

Stručný stavový panel WebUI zatím u explicitně vypnutého GoodWe/AZRouteru může
zobrazit Offline místo Vypnuto. Polling, e-ink diagnostika, sidebar i dynamická
registrace obrazovek už stav aktivace respektují.
