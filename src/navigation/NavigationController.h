#pragma once

#include <Arduino.h>
#include <functional>
#include "NavigationTypes.h"
#include "../data/DataModel.h"
#include "../screens/ScreenManager.h"

class NavigationController {
public:
    using ChangeCallback = std::function<void(bool fullRefresh)>;

    NavigationController(ScreenManager& screenManager, DataModel& dataModel);

    void onChange(ChangeCallback callback) { _changeCallback = callback; }

    // Resets focus to the sidebar entry that owns the currently active screen.
    // Used after activation from legacy WebUI endpoints or dynamic screen changes.
    void syncToActiveScreen(bool notify = false);

    bool handleAction(NavigationAction action);

    const NavigationState& getState() const { return _state; }
    void buildCurrentLayout(NavigationLayout& layout) const;

private:
    static constexpr uint8_t MaxSidebarEntries = 12;

    ScreenManager& _screenManager;
    DataModel& _dataModel;
    NavigationState _state;
    ChangeCallback _changeCallback;

    uint8_t collectSidebarEntries(IScreen** entries, uint8_t maxEntries) const;
    String sidebarIdForActiveScreen() const;
    bool ensureSidebarSelection();
    void publishState();
    void notifyChange(bool fullRefresh);

    bool moveSidebar(int8_t delta);
    bool enterPage(bool& screenChanged);
    bool leavePage();
    bool movePage(NavigationAction action);
    int findNeighbour(const NavigationLayout& layout, int currentIndex, NavigationAction action) const;
};
