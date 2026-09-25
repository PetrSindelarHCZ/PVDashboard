#pragma once

#include <Arduino.h>
#include "../navigation/NavigationTypes.h"
#include "../../include/AppConfig.h"

class FiveWayJoystick {
public:
    static constexpr uint8_t UpPin = JOY_UP_PIN;
    static constexpr uint8_t DownPin = JOY_DOWN_PIN;
    static constexpr uint8_t LeftPin = JOY_LEFT_PIN;
    static constexpr uint8_t RightPin = JOY_RIGHT_PIN;
    static constexpr uint8_t OkPin = JOY_OK_PIN;
    static constexpr uint8_t SetPin = JOY_SET_PIN;
    static constexpr uint8_t ResetPin = JOY_RESET_PIN;
    static constexpr uint32_t DebounceMs = 20;
    static constexpr uint32_t LongPressMs = 2500;
    static constexpr uint32_t RepeatDelayMs = 450;
    static constexpr uint32_t RepeatIntervalMs = 140;

    void begin();

    // Returns true once for each debounced button press.
    bool poll(NavigationAction& action);
    bool pollControl(ControlAction& action);

private:
    struct ButtonState {
        ButtonState(uint8_t pinValue, NavigationAction actionValue)
            : pin(pinValue), action(actionValue) {}

        uint8_t pin;
        NavigationAction action;
        bool rawPressed = false;
        bool stablePressed = false;
        unsigned long lastRawChangeMs = 0;
        unsigned long nextRepeatMs = 0;
    };

    ButtonState _buttons[5] = {
        {UpPin, NavigationAction::Up},
        {DownPin, NavigationAction::Down},
        {LeftPin, NavigationAction::Left},
        {RightPin, NavigationAction::Right},
        {OkPin, NavigationAction::Ok}
    };

    struct ControlButtonState {
        ControlButtonState(uint8_t pinValue, const char* nameValue)
            : pin(pinValue), name(nameValue) {}

        uint8_t pin;
        const char* name;
        bool rawPressed = false;
        bool stablePressed = false;
        bool longReported = false;
        unsigned long lastRawChangeMs = 0;
        unsigned long pressedAtMs = 0;
    };

    ControlButtonState _setButton{SetPin, "SET"};
    ControlButtonState _resetButton{ResetPin, "RESET"};
    bool _started = false;
};
