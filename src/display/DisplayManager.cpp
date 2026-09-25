#include "DisplayManager.h"
#include "../diagnostics/Performance.h"

DisplayManager::DisplayManager(IDisplay& display, DisplayPreview* preview)
    : _display(display), _preview(preview), _forceFullRefresh(true) {
}

void DisplayManager::init() {
    Performance::Scope timing(Performance::DisplayInit);
    Serial.println("[DISPLAY] Inicializace displeje...");
    _display.init();
    if (_preview != nullptr) _preview->init();
    Serial.println("[DISPLAY] Displej inicializovan.");
}

void DisplayManager::requestRefresh(bool full) {
    if (full) {
        _forceFullRefresh = true;
    }
}

void DisplayManager::renderScreen(IScreen* screen, const DataModel& dataModel,
                                  bool forceFullRefresh,
                                  const DisplayRegion* partialRegion,
                                  bool capturePreview) {
    if (!screen) {
        Serial.println("[DISPLAY] ERROR: Zadna obrazovka k vykresleni!");
        return;
    }

    const bool full = forceFullRefresh || _forceFullRefresh;
    const String screenId = screen->getId();
    const bool screenChanged =
        !_lastScreenId.isEmpty() && !_lastScreenId.equalsIgnoreCase(screenId);
    const bool cleanPartialTransition =
        !full &&
        screenChanged &&
        (partialRegion == nullptr || !partialRegion->valid());

    Performance::Scope timing(full ? Performance::DisplayFull : Performance::DisplayPartial);
    const unsigned long renderStarted = millis();
    if (!full && partialRegion != nullptr && partialRegion->valid()) {
        Serial.printf(
            "[DISPLAY][%lu ms] Vykresluji obrazovku '%s' (PARTIAL REGION %d,%d %dx%d)...\n",
            millis(), screen->getId().c_str(),
            partialRegion->x, partialRegion->y,
            partialRegion->width, partialRegion->height);
    } else {
        Serial.printf("[DISPLAY][%lu ms] Vykresluji obrazovku '%s' (Rezim: %s)...\n", millis(),
                      screen->getId().c_str(),
                      full ? "FULL REFRESH" : "PARTIAL REFRESH");
    }

    if (cleanPartialTransition) {
        // A full-window differential update is fast, but drawing a complex
        // screen directly over a different previous screen can corrupt some
        // 7.5" V2 panels. First drive the whole panel to white using the same
        // partial waveform, then draw the new screen. This stays much faster
        // than a cleaning full refresh while giving the new screen a known
        // background.
        Serial.printf(
            "[DISPLAY][%lu ms] Screen transition %s -> %s: partial white pre-clear.\n",
            millis(),
            _lastScreenId.c_str(),
            screenId.c_str());

        _display.beginFrame(true);
        do {
            _display.clear(1);
        } while (_display.nextFrame());
    }

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
    } else if (partialRegion != nullptr && partialRegion->valid()) {
        _display.beginPartialFrame(
            partialRegion->x,
            partialRegion->y,
            partialRegion->width,
            partialRegion->height);
    } else {
        _display.beginFrame(true);
    }

    do {
        _display.clear(1); // bila barva
        screen->render(_display, dataModel);
    } while (_display.nextFrame());

    _forceFullRefresh = false;
    _lastScreenId = screenId;

    _display.powerOff();
    if (capturePreview && _preview != nullptr) {
        _preview->capture(*screen, dataModel, full);
    }
    Serial.printf("[DISPLAY][%lu ms] Vykresleni dokonceno za %lu ms.\n", millis(), millis() - renderStarted);
}
