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

    for (ControlButtonState* button : {&_setButton, &_resetButton}) {
        pinMode(button->pin, INPUT); // GPIO34/35: externi 10k pull-up
        const bool pressed = digitalRead(button->pin) == LOW;
        button->rawPressed = pressed;
        button->stablePressed = pressed;
        button->longReported = false;
        button->lastRawChangeMs = millis();
        button->pressedAtMs = pressed ? millis() : 0;
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

    Serial.printf(
        "[KEY RAW] startup | SET=%u RESET=%u (1=released, 0=pressed; external 10k pull-up)\n",
        digitalRead(SetPin), digitalRead(ResetPin));
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

    // Direction buttons repeat while held. OK stays one-shot to avoid
    // accidental repeated activation.
    if (!eventReady) {
        for (auto& button : _buttons) {
            if (!button.stablePressed ||
                button.action == NavigationAction::Ok ||
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
    action = ControlAction::None;
    if (!_started) return false;

    const unsigned long now = millis();
    ControlButtonState* buttons[2] = {&_setButton, &_resetButton};

    for (ControlButtonState* button : buttons) {
        const bool pressed = digitalRead(button->pin) == LOW;

        if (pressed != button->rawPressed) {
            button->rawPressed = pressed;
            button->lastRawChangeMs = now;
            Serial.printf("[KEY RAW] %s GPIO%u -> %s at %lu ms\n",
                          button->name,
                          static_cast<unsigned>(button->pin),
                          levelName(pressed),
                          now);
        }

        if (button->stablePressed != button->rawPressed &&
            now - button->lastRawChangeMs >= DebounceMs) {
            button->stablePressed = button->rawPressed;
            Serial.printf("[KEY DEBOUNCED] %s -> %s\n",
                          button->name,
                          button->stablePressed ? "PRESSED" : "RELEASED");

            if (button->stablePressed) {
                button->pressedAtMs = now;
                button->longReported = false;
            } else {
                const bool wasLong = button->longReported;
                button->pressedAtMs = 0;
                button->longReported = false;

                if (!wasLong) {
                    if (button == &_setButton) {
                        action = ControlAction::SetShort;
                        Serial.println("[KEY] SET short");
                    } else {
                        action = ControlAction::ResetShort;
                        Serial.println("[KEY] RESET short");
                    }
                    return true;
                }
            }
        }

        if (button == &_setButton &&
            button->stablePressed &&
            !button->longReported &&
            button->pressedAtMs != 0 &&
            now - button->pressedAtMs >= LongPressMs) {
            button->longReported = true;
            action = ControlAction::SetLong;
            Serial.println("[KEY] SET long");
            return true;
        }
    }

    return false;
}
