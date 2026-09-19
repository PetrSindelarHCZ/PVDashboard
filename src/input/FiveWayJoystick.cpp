#include "FiveWayJoystick.h"

void FiveWayJoystick::begin() {
    for (auto& button : _buttons) {
        pinMode(button.pin, INPUT_PULLUP);
        const bool pressed = digitalRead(button.pin) == LOW;
        button.rawPressed = pressed;
        button.stablePressed = pressed;
        button.lastRawChangeMs = millis();
    }

    _started = true;

    Serial.printf(
        "[JOY] Aktivni LOW, INPUT_PULLUP | UP=%u DOWN=%u LEFT=%u RIGHT=%u OK=%u\n",
        static_cast<unsigned>(UpPin),
        static_cast<unsigned>(DownPin),
        static_cast<unsigned>(LeftPin),
        static_cast<unsigned>(RightPin),
        static_cast<unsigned>(OkPin));
}

bool FiveWayJoystick::poll(NavigationAction& action) {
    if (!_started) return false;

    const unsigned long now = millis();
    bool eventReady = false;
    NavigationAction eventAction = NavigationAction::Up;

    for (auto& button : _buttons) {
        const bool pressed = digitalRead(button.pin) == LOW;

        if (pressed != button.rawPressed) {
            button.rawPressed = pressed;
            button.lastRawChangeMs = now;
        }

        if (button.stablePressed == button.rawPressed) continue;
        if (now - button.lastRawChangeMs < DebounceMs) continue;

        button.stablePressed = button.rawPressed;

        // Only the press edge generates navigation. Release only rearms it.
        if (button.stablePressed && !eventReady) {
            eventReady = true;
            eventAction = button.action;
        }
    }

    if (!eventReady) return false;

    action = eventAction;
    Serial.printf("[JOY] %s\n", navigationActionName(action));
    return true;
}
