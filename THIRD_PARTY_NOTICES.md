# Third-party notices

PVDashboard includes or derives bitmap assets from the following third-party icon projects.

## Weather Icons

- Project: Weather Icons
- Authors: Lukas Bischoff / Erik Flowers and contributors
- Source: https://github.com/erikflowers/weather-icons
- License: SIL Open Font License 1.1
- Use in PVDashboard: monochrome weather pictograms rasterized/embedded for the e-paper UI.

The weather bitmap set used here follows the Weather Icons artwork distributed by the `lmarzen/esp32-weather-epd` project; that project identifies the underlying Weather Icons assets as SIL OFL 1.1.

## Font Awesome Free

- Project: Font Awesome Free
- Copyright: Fonticons, Inc.
- Source: https://github.com/FortAwesome/Font-Awesome
- Icons license: CC BY 4.0
- Use in PVDashboard: monochrome sidebar icons (home, solar, pool, weather, settings), rasterized to 1-bit bitmaps for the e-paper UI.

This notice is informational and does not change the licensing terms of the respective upstream projects.

## KPI display font candidates

- Chakra Petch Bold, Quantico Bold and Aldrich
- Source: Google Fonts (https://github.com/google/fonts)
- License: SIL Open Font License 1.1
- Use in PVDashboard: temporary 1-bit diagnostic rasterization for selecting a KPI display typeface. The source TTF files live under tools/ and are not linked into firmware.


## Dashboard Sans V2

- Typeface source: Inter Display (Regular and Bold)
- Project: Inter by Rasmus Andersson and contributors
- License: SIL Open Font License 1.1
- Use in PVDashboard: native 1-bit bitmap faces generated at 8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32 and 36 px. The generated charset contains printable ASCII, the complete Czech diacritic set used by the UI, and the degree sign.
