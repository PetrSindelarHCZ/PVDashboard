#pragma once
#include <Arduino.h>

// ==========================================
// Waveshare ESP32 e-Paper Driver Board SPI Piny
// ==========================================
#define EPD_CS    15
#define EPD_DC    27
#define EPD_RST   26
#define EPD_BUSY  25
#define EPD_SCK   13
#define EPD_MISO  12
#define EPD_MOSI  14

// ==========================================
// Pětisměrný navigační ovladač
// Tlačítka spínají GPIO proti GND (active LOW)
// ==========================================
#define JOY_UP_PIN     16
#define JOY_DOWN_PIN   17
#define JOY_LEFT_PIN   18
#define JOY_RIGHT_PIN  32
#define JOY_OK_PIN     33

// ==========================================
// Rozměry displeje
// ==========================================
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480

// ==========================================
// Časové a systémové konstanty
// ==========================================
#define DEFAULT_HOSTNAME      "dashboard"
#define DEFAULT_TIMEZONE      "CET-1CEST,M3.5.0,M10.5.0/3" // Europe/Prague s automatickým DST
#define DEFAULT_NTP_SERVER    "pool.ntp.org"

// Intervaly v milisekundách
#define FULL_REFRESH_INTERVAL_MS    (24 * 60 * 60 * 1000UL) // 1x denně full refresh proti duchům
#define STATUS_POLL_INTERVAL_MS     (10 * 1000UL)           // 10 sekund perioda dat
