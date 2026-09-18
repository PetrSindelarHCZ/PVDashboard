#pragma once

#include <Arduino.h>
#include <stdint.h>

enum class NavigationAction : uint8_t {
    Up,
    Down,
    Left,
    Right,
    Ok
};

enum class NavigationArea : uint8_t {
    Sidebar,
    Page
};

inline const char* navigationActionName(NavigationAction action) {
    switch (action) {
        case NavigationAction::Up: return "up";
        case NavigationAction::Down: return "down";
        case NavigationAction::Left: return "left";
        case NavigationAction::Right: return "right";
        case NavigationAction::Ok: return "ok";
    }
    return "unknown";
}

inline bool parseNavigationAction(String value, NavigationAction& action) {
    value.trim();
    value.toLowerCase();
    if (value == "up") { action = NavigationAction::Up; return true; }
    if (value == "down") { action = NavigationAction::Down; return true; }
    if (value == "left") { action = NavigationAction::Left; return true; }
    if (value == "right") { action = NavigationAction::Right; return true; }
    if (value == "ok" || value == "enter") { action = NavigationAction::Ok; return true; }
    return false;
}

inline const char* navigationAreaName(NavigationArea area) {
    return area == NavigationArea::Page ? "page" : "sidebar";
}

struct NavigationRect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;

    int16_t centerX() const { return x + width / 2; }
    int16_t centerY() const { return y + height / 2; }
};

struct NavigationElement {
    String id;
    NavigationRect bounds;
    bool enabled = true;
};

struct NavigationLayout {
    // Fixed-size storage keeps navigation deterministic on ESP32 and avoids
    // allocating a graph every time the user presses a direction.
    static constexpr uint8_t MaxElements = 16;

    NavigationElement elements[MaxElements];
    uint8_t count = 0;

    // Optional. When empty or invalid, the controller derives the entry point
    // from geometry (left-most, then top-most element).
    String entryPointId;

    void clear() {
        count = 0;
        entryPointId = "";
    }

    bool add(const String& id, int16_t x, int16_t y, int16_t width, int16_t height, bool enabled = true) {
        if (count >= MaxElements || id.isEmpty() || width <= 0 || height <= 0) return false;
        NavigationElement& element = elements[count++];
        element.id = id;
        element.bounds = {x, y, width, height};
        element.enabled = enabled;
        return true;
    }

    int find(const String& id) const {
        for (uint8_t i = 0; i < count; ++i) {
            if (elements[i].enabled && elements[i].id == id) return i;
        }
        return -1;
    }

    String resolveEntryPoint() const {
        const int explicitIndex = find(entryPointId);
        if (explicitIndex >= 0) return elements[explicitIndex].id;

        int best = -1;
        for (uint8_t i = 0; i < count; ++i) {
            if (!elements[i].enabled) continue;
            if (best < 0 ||
                elements[i].bounds.x < elements[best].bounds.x ||
                (elements[i].bounds.x == elements[best].bounds.x &&
                 elements[i].bounds.y < elements[best].bounds.y)) {
                best = i;
            }
        }
        return best >= 0 ? elements[best].id : String();
    }
};

struct NavigationState {
    NavigationArea area = NavigationArea::Sidebar;
    String sidebarScreenId = "home";
    String focusId;
};
