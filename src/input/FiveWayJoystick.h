#pragma once

#include <Arduino.h>
#include "../navigation/NavigationTypes.h"

class FiveWayJoystick {
public:
    static constexpr uint8_t UpPin = 16;
    static constexpr uint8_t DownPin = 17;
    static constexpr uint8_t LeftPin = 18;
    static constexpr uint8_t RightPin = 32;
    static constexpr uint8_t OkPin = 33;
    static constexpr uint32_t DebounceMs = 30;

    void begin();

    // Returns true once for each debounced button press.
    bool poll(NavigationAction& action);

private:
    struct ButtonState {
        uint8_t pin = 0;
        NavigationAction action = NavigationAction::Up;
        bool rawPressed = false;
        bool stablePressed = false;
        unsigned long lastRawChangeMs = 0;
    };

    ButtonState _buttons[5] = {
        {UpPin, NavigationAction::Up},
        {DownPin, NavigationAction::Down},
        {LeftPin, NavigationAction::Left},
        {RightPin, NavigationAction::Right},
        {OkPin, NavigationAction::Ok}
    };

    bool _started = false;
};
