#pragma once
#include <Arduino.h>
#include <vector>
#include "IScreen.h"

class ScreenManager {
public:
    ScreenManager();

    void registerScreen(IScreen* screen);
    bool activateScreen(const String& id);
    IScreen* getActiveScreen() const;
    String getActiveScreenId() const;
    const std::vector<IScreen*>& getAllScreens() const;

private:
    std::vector<IScreen*> _screens;
    IScreen* _activeScreen = nullptr;
};
