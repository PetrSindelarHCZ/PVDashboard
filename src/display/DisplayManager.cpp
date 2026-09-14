#include "DisplayManager.h"
#include "../diagnostics/Performance.h"

DisplayManager::DisplayManager(IDisplay& display)
    : _display(display), _forceFullRefresh(true) {
}

void DisplayManager::init() {
    Performance::Scope timing(Performance::DisplayInit);
    Serial.println("[DISPLAY] Inicializace displeje...");
    _display.init();
    Serial.println("[DISPLAY] Displej inicializovan.");
}

void DisplayManager::requestRefresh(bool full) {
    if (full) {
        _forceFullRefresh = true;
    }
}

void DisplayManager::renderScreen(IScreen* screen, const DataModel& dataModel, bool forceFullRefresh) {
    if (!screen) {
        Serial.println("[DISPLAY] ERROR: Zadna obrazovka k vykresleni!");
        return;
    }

    const bool partialLimitReached =
        _consecutivePartialRefreshes >= MaxConsecutivePartialRefreshes;
    bool full = forceFullRefresh || _forceFullRefresh || partialLimitReached;
    if (partialLimitReached && !forceFullRefresh && !_forceFullRefresh) {
        Serial.printf("[DISPLAY] Po %u castecnych obnovach vynucuji plnou obnovu.\n",
                      MaxConsecutivePartialRefreshes);
    }
    Performance::Scope timing(full ? Performance::DisplayFull : Performance::DisplayPartial);
    const unsigned long renderStarted = millis();
    Serial.printf("[DISPLAY][%lu ms] Vykresluji obrazovku '%s' (Rezim: %s)...\n", millis(),
                  screen->getId().c_str(), 
                  full ? "FULL REFRESH" : "PARTIAL REFRESH");

    if (full) {
        // Older Waveshare 7.5" V2 panels can leave the final image noticeably
        // grey when it is drawn directly with the full-refresh waveform. A
        // full white erase removes accumulated charge and ghosting; drawing
        // the image afterwards with the differential waveform restores black.
        _display.beginFrame(false);
        do {
            _display.clear(1); // bila barva
        } while (_display.nextFrame());

        _display.beginFrame(true);
    } else {
        _display.beginFrame(true);
    }

    do {
        _display.clear(1); // bila barva
        screen->render(_display, dataModel);
    } while (_display.nextFrame());

    _forceFullRefresh = false;
    if (full) {
        _lastFullRefreshMs = millis();
        _consecutivePartialRefreshes = 0;
    } else {
        ++_consecutivePartialRefreshes;
    }
    _display.powerOff();
    Serial.printf("[DISPLAY][%lu ms] Vykresleni dokonceno za %lu ms.\n", millis(), millis() - renderStarted);
}
