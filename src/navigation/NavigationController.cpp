#include "NavigationController.h"
#include <limits.h>

NavigationController::NavigationController(ScreenManager& screenManager, DataModel& dataModel)
    : _screenManager(screenManager), _dataModel(dataModel) {
    publishState();
}

uint8_t NavigationController::collectSidebarEntries(IScreen** entries, uint8_t maxEntries) const {
    uint8_t count = 0;
    for (auto* screen : _screenManager.getAllScreens()) {
        if (!screen || !screen->isSidebarEntry()) continue;
        if (count < maxEntries) entries[count++] = screen;
    }
    return count;
}

String NavigationController::sidebarIdForActiveScreen() const {
    IScreen* active = _screenManager.getActiveScreen();
    if (active == nullptr) return String();

    const String parent = active->getSidebarParentId();
    IScreen* entries[MaxSidebarEntries] = {};
    const uint8_t count = collectSidebarEntries(entries, MaxSidebarEntries);
    for (uint8_t i = 0; i < count; ++i) {
        if (entries[i]->getId().equalsIgnoreCase(parent)) return entries[i]->getId();
    }
    return count > 0 ? entries[0]->getId() : String();
}

bool NavigationController::ensureSidebarSelection() {
    IScreen* entries[MaxSidebarEntries] = {};
    const uint8_t count = collectSidebarEntries(entries, MaxSidebarEntries);
    if (count == 0) {
        const bool changed = !_state.sidebarScreenId.isEmpty();
        _state.sidebarScreenId = "";
        return changed;
    }

    for (uint8_t i = 0; i < count; ++i) {
        if (entries[i]->getId().equalsIgnoreCase(_state.sidebarScreenId)) {
            _state.sidebarScreenId = entries[i]->getId();
            return false;
        }
    }

    const String activeSidebarId = sidebarIdForActiveScreen();
    _state.sidebarScreenId = activeSidebarId.isEmpty() ? entries[0]->getId() : activeSidebarId;
    return true;
}

void NavigationController::publishState() {
    _dataModel.system.navigationArea = navigationAreaName(_state.area);
    _dataModel.system.navigationSidebarScreenId = _state.sidebarScreenId;
    _dataModel.system.navigationFocusId = _state.focusId;
}

void NavigationController::notifyChange(bool fullRefresh) {
    if (_changeCallback) _changeCallback(fullRefresh);
}

void NavigationController::syncToActiveScreen(bool notify) {
    const NavigationState previous = _state;
    _state.area = NavigationArea::Sidebar;
    _state.focusId = "";
    _state.sidebarScreenId = sidebarIdForActiveScreen();
    ensureSidebarSelection();
    publishState();

    const bool changed =
        previous.area != _state.area ||
        previous.sidebarScreenId != _state.sidebarScreenId ||
        previous.focusId != _state.focusId;
    if (notify && changed) notifyChange(false);
}

bool NavigationController::moveSidebar(int8_t delta) {
    IScreen* entries[MaxSidebarEntries] = {};
    const uint8_t count = collectSidebarEntries(entries, MaxSidebarEntries);
    if (count == 0) return false;

    int current = -1;
    for (uint8_t i = 0; i < count; ++i) {
        if (entries[i]->getId().equalsIgnoreCase(_state.sidebarScreenId)) {
            current = i;
            break;
        }
    }
    if (current < 0) current = 0;

    const int next = current + delta;
    if (next < 0 || next >= count) return false;
    if (next == current && _state.sidebarScreenId == entries[next]->getId()) return false;

    _state.sidebarScreenId = entries[next]->getId();
    return true;
}

void NavigationController::buildCurrentLayout(NavigationLayout& layout) const {
    layout.clear();
    IScreen* active = _screenManager.getActiveScreen();
    if (active != nullptr) active->buildNavigationLayout(_dataModel, layout);
}

bool NavigationController::enterPage(bool& screenChanged) {
    screenChanged = false;
    ensureSidebarSelection();
    if (_state.sidebarScreenId.isEmpty()) return false;

    // RIGHT only enters the page that is already displayed. Changing the
    // displayed page is an explicit OK action in sidebar mode.
    if (!_screenManager.getActiveScreenId().equalsIgnoreCase(_state.sidebarScreenId)) {
        return false;
    }

    NavigationLayout layout;
    buildCurrentLayout(layout);
    _state.area = NavigationArea::Page;
    _state.focusId = layout.resolveInitialFocus();
    return true;
}

bool NavigationController::leavePage() {
    if (_state.area != NavigationArea::Page) return false;
    _state.area = NavigationArea::Sidebar;
    _state.focusId = "";
    _state.sidebarScreenId = sidebarIdForActiveScreen();
    ensureSidebarSelection();
    return true;
}

int NavigationController::findNeighbour(
    const NavigationLayout& layout,
    int currentIndex,
    NavigationAction action) const {

    if (currentIndex < 0 || currentIndex >= layout.count) return -1;
    if (action != NavigationAction::Up &&
        action != NavigationAction::Down &&
        action != NavigationAction::Left &&
        action != NavigationAction::Right) return -1;

    const NavigationRect& current = layout.elements[currentIndex].bounds;
    const int32_t currentCx = current.centerX();
    const int32_t currentCy = current.centerY();

    int bestIndex = -1;
    int32_t bestScore = INT32_MAX;

    for (uint8_t i = 0; i < layout.count; ++i) {
        if (i == currentIndex || !layout.elements[i].enabled) continue;

        const NavigationRect& candidate = layout.elements[i].bounds;
        const int32_t candidateCx = candidate.centerX();
        const int32_t candidateCy = candidate.centerY();

        bool inDirection = false;
        bool orthogonalOverlap = false;
        int32_t primaryGap = 0;
        int32_t primaryCenterDistance = 0;
        int32_t crossDistance = 0;

        switch (action) {
            case NavigationAction::Right:
                inDirection = candidateCx > currentCx;
                {
                    const int32_t gap = candidate.x - (current.x + current.width);
                    primaryGap = gap > 0 ? gap : 0;
                }
                primaryCenterDistance = abs(candidateCx - currentCx);
                crossDistance = abs(candidateCy - currentCy);
                orthogonalOverlap =
                    candidate.y < current.y + current.height &&
                    candidate.y + candidate.height > current.y;
                break;
            case NavigationAction::Left:
                inDirection = candidateCx < currentCx;
                {
                    const int32_t gap = current.x - (candidate.x + candidate.width);
                    primaryGap = gap > 0 ? gap : 0;
                }
                primaryCenterDistance = abs(candidateCx - currentCx);
                crossDistance = abs(candidateCy - currentCy);
                orthogonalOverlap =
                    candidate.y < current.y + current.height &&
                    candidate.y + candidate.height > current.y;
                break;
            case NavigationAction::Down:
                inDirection = candidateCy > currentCy;
                {
                    const int32_t gap = candidate.y - (current.y + current.height);
                    primaryGap = gap > 0 ? gap : 0;
                }
                primaryCenterDistance = abs(candidateCy - currentCy);
                crossDistance = abs(candidateCx - currentCx);
                orthogonalOverlap =
                    candidate.x < current.x + current.width &&
                    candidate.x + candidate.width > current.x;
                break;
            case NavigationAction::Up:
                inDirection = candidateCy < currentCy;
                {
                    const int32_t gap = current.y - (candidate.y + candidate.height);
                    primaryGap = gap > 0 ? gap : 0;
                }
                primaryCenterDistance = abs(candidateCy - currentCy);
                crossDistance = abs(candidateCx - currentCx);
                orthogonalOverlap =
                    candidate.x < current.x + current.width &&
                    candidate.x + candidate.width > current.x;
                break;
            default:
                break;
        }

        if (!inDirection) continue;

        // Do not jump almost perpendicular to the pressed direction. Diagonal
        // navigation is allowed when the candidate still lies inside a 90°
        // directional cone, while aligned rows/columns always remain valid.
        if (!orthogonalOverlap && crossDistance > primaryCenterDistance) continue;

        // Prefer an element that visually overlaps the current row/column.
        // Only when there is no such candidate do diagonal elements compete.
        const int32_t overlapPenalty = orthogonalOverlap ? 0 : 1000000L;
        const int32_t score = overlapPenalty + primaryGap * 1024L + crossDistance;
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool NavigationController::movePage(NavigationAction action) {
    NavigationLayout layout;
    buildCurrentLayout(layout);

    if (layout.count == 0) {
        return action == NavigationAction::Left ? leavePage() : false;
    }

    int current = layout.find(_state.focusId);
    if (current < 0) {
        _state.focusId = layout.resolveInitialFocus();
        current = layout.find(_state.focusId);
        if (current < 0) return false;
    }

    const int next = findNeighbour(layout, current, action);
    if (next >= 0) {
        _state.focusId = layout.elements[next].id;
        return true;
    }

    // The sidebar is directly to the left of the page. Therefore every
    // focusable element on the page's left navigation edge can leave the page:
    // LEFT first tries to find another element to the left; if none exists,
    // focus returns to the sidebar. No dedicated exit node is required.
    if (action == NavigationAction::Left) {
        return leavePage();
    }

    return false;
}

bool NavigationController::handleAction(NavigationAction action) {
    ensureSidebarSelection();

    bool changed = false;
    bool screenChanged = false;

    if (_state.area == NavigationArea::Sidebar) {
        switch (action) {
            case NavigationAction::Up:
                changed = moveSidebar(-1);
                break;
            case NavigationAction::Down:
                changed = moveSidebar(1);
                break;
            case NavigationAction::Right:
                changed = enterPage(screenChanged);
                break;
            case NavigationAction::Ok:
                if (!_state.sidebarScreenId.isEmpty() &&
                    !_screenManager.getActiveScreenId().equalsIgnoreCase(_state.sidebarScreenId) &&
                    _screenManager.activateScreen(_state.sidebarScreenId)) {
                    _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
                    screenChanged = true;
                    changed = true;
                }
                break;
            case NavigationAction::Left:
                break;
        }
    } else {
        switch (action) {
            case NavigationAction::Up:
            case NavigationAction::Down:
            case NavigationAction::Left:
            case NavigationAction::Right:
                changed = movePage(action);
                break;
            case NavigationAction::Ok:
                // Reserved for element actions/edit mode in a later feature.
                break;
        }
    }

    if (!changed && !screenChanged) return false;

    publishState();
    Serial.printf(
        "[NAV] action=%s area=%s sidebar=%s focus=%s screen=%s%s\n",
        navigationActionName(action),
        navigationAreaName(_state.area),
        _state.sidebarScreenId.c_str(),
        _state.focusId.c_str(),
        _screenManager.getActiveScreenId().c_str(),
        screenChanged ? " [screen-change]" : "");

    notifyChange(screenChanged);
    return true;
}
