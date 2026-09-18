#pragma once
#include <Arduino.h>

enum class LayoutWidgetType : uint8_t {
    HomeWeatherCard,
    HomeEnergyCard,
    HomeIndoorCard
};

struct LayoutWidget {
    const char* id = "";
    LayoutWidgetType type = LayoutWidgetType::HomeIndoorCard;
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;
};

class ScreenLayout {
public:
    static constexpr uint8_t MaxWidgets = 8;

    void clear() { _count = 0; }

    bool add(const char* id, LayoutWidgetType type,
             int16_t x, int16_t y, int16_t width, int16_t height) {
        if (!id || !*id || width <= 0 || height <= 0 || _count >= MaxWidgets) return false;
        LayoutWidget& widget = _widgets[_count++];
        widget.id = id;
        widget.type = type;
        widget.x = x;
        widget.y = y;
        widget.width = width;
        widget.height = height;
        return true;
    }

    uint8_t count() const { return _count; }
    const LayoutWidget& operator[](uint8_t index) const { return _widgets[index]; }

private:
    LayoutWidget _widgets[MaxWidgets];
    uint8_t _count = 0;
};
