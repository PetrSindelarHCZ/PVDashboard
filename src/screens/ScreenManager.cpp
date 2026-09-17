#include "ScreenManager.h"

ScreenManager::ScreenManager() : _activeScreen(nullptr) {
}

void ScreenManager::registerScreen(IScreen* screen) {
    if (!screen || hasScreen(screen->getId())) return;
    _screens.push_back(screen);
    if (!_activeScreen) {
        _activeScreen = screen;
    }
}

bool ScreenManager::unregisterScreen(const String& id) {
    for (auto it = _screens.begin(); it != _screens.end(); ++it) {
        IScreen* screen = *it;
        if (!screen || !screen->getId().equalsIgnoreCase(id)) continue;
        const bool wasActive = _activeScreen == screen;
        _screens.erase(it);
        if (wasActive) _activeScreen = _screens.empty() ? nullptr : _screens.front();
        return true;
    }
    return false;
}

bool ScreenManager::hasScreen(const String& id) const {
    for (auto* screen : _screens) {
        if (screen && screen->getId().equalsIgnoreCase(id)) return true;
    }
    return false;
}

bool ScreenManager::activateScreen(const String& id) {
    for (auto* screen : _screens) {
        if (screen && screen->getId().equalsIgnoreCase(id)) {
            _activeScreen = screen;
            Serial.printf("[SCREENS] Aktivovana obrazovka: %s (%s)\n", 
                          screen->getId().c_str(), 
                          screen->getTitle().c_str());
            return true;
        }
    }
    Serial.printf("[SCREENS] ERROR: Obrazovka '%s' nenalezena!\n", id.c_str());
    return false;
}

IScreen* ScreenManager::getActiveScreen() const {
    return _activeScreen;
}

String ScreenManager::getActiveScreenId() const {
    return _activeScreen ? _activeScreen->getId() : "none";
}

const std::vector<IScreen*>& ScreenManager::getAllScreens() const {
    return _screens;
}
