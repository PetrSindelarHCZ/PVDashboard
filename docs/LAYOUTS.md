# Konfigurovatelné obrazovky

Tento dokument popisuje datový layout zaváděný ve scénáři D roadmapy.

## Princip

Firmware nebude implementovat obecný HTML/CSS layout. Každá obrazovka má omezenou,
předem známou množinu widgetů. Layout pouze určuje jejich geometrii a parametry.
Stejný layout musí používat fyzický e-paper, Display Preview i navigační hitboxy.

Základní geometrie widgetu:

- `id` – stabilní identifikátor instance widgetu,
- `type` – podporovaný typ widgetu,
- `x`, `y` – levý horní roh v pixelech,
- `width`, `height` – rozměr v pixelech.

Souřadnice jsou v logickém prostoru displeje 800 × 480.

## První podporovaná sada

První implementační krok pokrývá obrazovku Home:

- `weather-card` / `HomeWeatherCard`,
- `energy-card` / `HomeEnergyCard`,
- `indoor-card` / `HomeIndoorCard`.

Home již nevytváří geometrii zvlášť pro renderer a zvlášť pro navigaci.
Obě vrstvy dostávají jeden společný `ScreenLayout`.

Výchozí layout zatím zachovává dnešní automatické rozložení podle toho, zda je
aktivní počasí a FVE. Tím se první krok scénáře D obejde bez změny vzhledu.

## Omezení první fáze

Datový model je zatím runtime popis. Persistovaná konfigurace, validace změn
z WebUI, editor, export/import a reset šablony budou doplněny v následujících
krocích scénáře D.

Při zavedení editoru musí firmware validovat alespoň:

- widget leží v ploše obsahu a nepřekračuje 800 × 480,
- šířka a výška splňují minimum daného typu,
- na jedné obrazovce není duplicitní `id`,
- počet widgetů nepřekročí pevný limit,
- neznámý typ widgetu se odmítne místo tichého ignorování.


## D2 — persistence a REST API

Home layout je uložen v `AppConfig.display.homeLayout` a v NVS pod klíčem
`layout_home`. Pokud `customized=false`, renderer používá původní automatickou
šablonu podle dostupnosti Počasí a FVE.

REST rozhraní (D2):

- `GET /api/layout/home` — uložený i právě efektivní layout a limity widgetů,
- `POST /api/layout/home` — validace a uložení vlastního layoutu,
- `POST /api/layout/home/reset` — návrat na automatickou výchozí šablonu.

POST používá JSON, například:

```json
{
  "customized": true,
  "widgets": [
    {
      "id": "weather-card",
      "type": "weather",
      "visible": true,
      "x": 75,
      "y": 63,
      "width": 225,
      "height": 402
    },
    {
      "id": "energy-card",
      "type": "energy",
      "visible": true,
      "x": 315,
      "y": 63,
      "width": 225,
      "height": 402
    },
    {
      "id": "indoor-card",
      "type": "indoor",
      "visible": true,
      "x": 555,
      "y": 63,
      "width": 230,
      "height": 402
    }
  ]
}
```

Firmware odmítá neznámé nebo duplicitní widgety, geometrii mimo obsahovou
plochu, příliš malé widgety a překryv viditelných widgetů.

Formát YAML zálohy je od této fáze verze 6 a obsahuje `layout.home_json`.
Starší zálohy se importují s výchozím automatickým Home layoutem.
