# Fotovoltaický Dashboard & AZ Router Monitor (ESP32 + 7.5" e-Paper)

## 1. Cíl projektu
Vytvořit nástěnný/stolní přehledový displej na bázi **ESP32** a **7.5" e-Paper (800x480)** pro vizualizaci energetických toků v reálném čase:
- Výkon FVE panelů
- Stav nabití (SoC) a výkon baterie
- Výkon a stav AZ Routeru (vytěžování do bojleru / akumulační nádrže)
- Spotřeba rodinného domu
- Tok do/z distribuční sítě (přetoky vs. nákup)

---

## 2. Hardware
- **Řídicí deska:** Waveshare ESP32 e-Paper Driver Board (ESP32-WROOM-32, WiFi, Bluetooth, SPI)
- **Displej:** Waveshare 7.5" e-Paper (800x480 px, černobílý)
- **Střídač (Invertor):** *(Doplňte model – např. GoodWe, Victron, Solax, Fronius, Growatt)*
- **Regulátor přetoků:** AZ Router *(Doplňte verzi / typ rozhraní)*

---

## 3. Komunikační rozhraní a získávání dat
*(Doplňte dle reality, např. REST API, MQTT broker, Modbus TCP / RTU, Home Assistant integration):*
- **Metoda vyčítání střídače:** 
- **Metoda vyčítání AZ Routeru:** 
- **MQTT Broker / Home Assistant IP:** 
- **Frekvence čtení dat:** 

---

## 4. Architektura softwaru
- **Framework:** Arduino (PlatformIO)
- **Knihovny:**
  - `GxEPD2` – ovladač displeje s podporou celkového a částečného obnovení (partial refresh)
  - `Adafruit-GFX` – grafické vykreslování
  - `ArduinoJson` – parsování dat z API/MQTT
