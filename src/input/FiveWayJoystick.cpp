#include "FiveWayJoystick.h"

namespace {
const char* buttonName(NavigationAction action) {
    switch (action) {
        case NavigationAction::Up: return "UP";
        case NavigationAction::Down: return "DOWN";
        case NavigationAction::Left: return "LEFT";
        case NavigationAction::Right: return "RIGHT";
        case NavigationAction::Ok: return "OK";
        default: return "?";
    }
}

const char* levelName(bool pressed) {
    return pressed ? "LOW/PRESSED" : "HIGH/RELEASED";
}
}

void FiveWayJoystick::begin() {
    for (auto& button : _buttons) {
        pinMode(button.pin, INPUT_PULLUP);
        const bool pressed = digitalRead(button.pin) == LOW;
        button.rawPressed = pressed;
        button.stablePressed = pressed;
        button.lastRawChangeMs = millis();
        button.nextRepeatMs = 0;
    }

    _started = true;

    Serial.printf(
        "[JOY] Active LOW, INPUT_PULLUP | UP=%u DOWN=%u LEFT=%u RIGHT=%u OK=%u\n",
        static_cast<unsigned>(UpPin),
        static_cast<unsigned>(DownPin),
        static_cast<unsigned>(LeftPin),
        static_cast<unsigned>(RightPin),
        static_cast<unsigned>(OkPin));

    Serial.printf(
        "[JOY RAW] startup | UP=%u DOWN=%u LEFT=%u RIGHT=%u OK=%u (1=released, 0=pressed)\n",
        digitalRead(UpPin), digitalRead(DownPin), digitalRead(LeftPin),
        digitalRead(RightPin), digitalRead(OkPin));

    Serial.println("[DIAG] SET/RESET disabled; GPIO34/35 are not configured or read.");
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

            // Cancel key repeat as soon as the electrical release is seen.
            // Do not wait for the debounced release: an e-paper refresh can
            // start before DebounceMs elapses and otherwise leave
            // stablePressed=true with an overdue repeat timer, causing one
            // physical click to advance repeatedly after each refresh.
            if (!pressed) {
                button.nextRepeatMs = 0;
            }

            Serial.printf(
                "[JOY RAW] %s GPIO%u -> %s at %lu ms\n",
                buttonName(button.action),
                static_cast<unsigned>(button.pin),
                levelName(pressed),
                now);
        }

        if (button.stablePressed == button.rawPressed) continue;
        if (now - button.lastRawChangeMs < DebounceMs) continue;

        button.stablePressed = button.rawPressed;

        Serial.printf(
            "[JOY DEBOUNCED] %s GPIO%u -> %s\n",
            buttonName(button.action),
            static_cast<unsigned>(button.pin),
            button.stablePressed ? "PRESSED" : "RELEASED");

        if (button.stablePressed) {
            button.nextRepeatMs = now + RepeatDelayMs;
            if (!eventReady) {
                eventReady = true;
                eventAction = button.action;
            }
        } else {
            button.nextRepeatMs = 0;
        }
    }

    // Only vertical navigation repeats while held. Horizontal actions are
    // intentionally one-shot: LEFT/RIGHT switch pager pages on screens such
    // as FVE, and an e-paper refresh is long enough that a held/released key
    // can otherwise advance across multiple pages before the UI settles.
    if (!eventReady) {
        for (auto& button : _buttons) {
            const bool repeatable =
                button.action == NavigationAction::Up ||
                button.action == NavigationAction::Down;

            if (!repeatable ||
                !button.rawPressed ||
                !button.stablePressed ||
                button.nextRepeatMs == 0 ||
                static_cast<long>(now - button.nextRepeatMs) < 0) {
                continue;
            }

            button.nextRepeatMs = now + RepeatIntervalMs;
            eventReady = true;
            eventAction = button.action;
            Serial.printf(
                "[JOY REPEAT] %s GPIO%u\n",
                buttonName(button.action),
                static_cast<unsigned>(button.pin));
            break;
        }
    }

    if (!eventReady) return false;

    action = eventAction;
    Serial.printf("[JOY ACTION] %s\n", navigationActionName(action));
    return true;
}


bool FiveWayJoystick::pollControl(ControlAction& action) {
    // Diagnostic build: SET and RESET are intentionally disabled and their
    // GPIOs are left untouched.
    action = ControlAction::None;
    return false;
}
