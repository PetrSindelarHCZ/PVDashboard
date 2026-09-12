#include "DisplayManager.h"

DisplayManager::DisplayManager(IDisplay& display)
    : _display(display), _forceFullRefresh(true) {
}

void DisplayManager::init() {
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

    bool full = forceFullRefresh || _forceFullRefresh;
    Serial.printf("[DISPLAY] Vykresluji obrazovku '%s' (Rezim: %s)...\n", 
                  screen->getId().c_str(), 
                  full ? "FULL REFRESH" : "PARTIAL REFRESH");

    _display.beginFrame(!full);

    do {
        _display.clear(1); // bila barva
        screen->render(_display, dataModel);
    } while (_display.nextFrame());

    _forceFullRefresh = false;
    _lastFullRefreshMs = millis();
    _display.powerOff();
    Serial.println("[DISPLAY] Vykresleni dokonceno.");
}
