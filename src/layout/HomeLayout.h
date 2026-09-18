#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "ScreenLayout.h"

namespace HomeLayout {

constexpr int16_t DisplayWidth = 800;
constexpr int16_t DisplayHeight = 480;
constexpr int16_t ContentLeft = 75;
constexpr int16_t ContentTop = 63;
constexpr int16_t ContentRight = 785;
constexpr int16_t ContentBottom = 465;

inline bool knownWidget(const String& id, const String& type) {
    return (id == "weather-card" && type == "weather") ||
           (id == "energy-card" && type == "energy") ||
           (id == "indoor-card" && type == "indoor");
}

inline int16_t minWidth(const String& type) {
    if (type == "weather") return 190;
    if (type == "energy") return 190;
    if (type == "indoor") return 180;
    return 0;
}

inline int16_t minHeight(const String& type) {
    if (type == "weather") return 360;
    if (type == "energy") return 330;
    if (type == "indoor") return 260;
    return 0;
}

inline LayoutWidgetType runtimeType(const String& type) {
    if (type == "weather") return LayoutWidgetType::HomeWeatherCard;
    if (type == "energy") return LayoutWidgetType::HomeEnergyCard;
    return LayoutWidgetType::HomeIndoorCard;
}

inline const char* typeName(LayoutWidgetType type) {
    switch (type) {
        case LayoutWidgetType::HomeWeatherCard: return "weather";
        case LayoutWidgetType::HomeEnergyCard: return "energy";
        case LayoutWidgetType::HomeIndoorCard: return "indoor";
    }
    return "unknown";
}

inline bool intersects(const HomeLayoutWidgetConfig& a, const HomeLayoutWidgetConfig& b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}

inline bool validate(const HomeLayoutConfig& config, String* error = nullptr) {
    auto fail = [error](const String& message) {
        if (error != nullptr) *error = message;
        return false;
    };

    if (!config.customized) {
        if (config.widgetCount != 0) return fail("Default layout must not contain stored widgets");
        return true;
    }

    if (config.widgetCount == 0 || config.widgetCount > MaxHomeLayoutWidgets) {
        return fail("Custom Home layout must contain 1 to 3 widgets");
    }

    bool anyVisible = false;
    for (uint8_t i = 0; i < config.widgetCount; ++i) {
        const HomeLayoutWidgetConfig& widget = config.widgets[i];
        if (!knownWidget(widget.id, widget.type)) {
            return fail("Unknown Home widget");
        }

        if (widget.width < minWidth(widget.type) ||
            widget.height < minHeight(widget.type)) {
            return fail("Home widget is smaller than its supported minimum");
        }

        if (widget.x < ContentLeft || widget.y < ContentTop ||
            widget.x + widget.width > ContentRight ||
            widget.y + widget.height > ContentBottom) {
            return fail("Home widget is outside the content area");
        }

        anyVisible = anyVisible || widget.visible;

        for (uint8_t j = 0; j < i; ++j) {
            const HomeLayoutWidgetConfig& previous = config.widgets[j];
            if (widget.id == previous.id || widget.type == previous.type) {
                return fail("Duplicate Home widget");
            }
            if (widget.visible && previous.visible && intersects(widget, previous)) {
                return fail("Visible Home widgets overlap");
            }
        }
    }

    if (!anyVisible) return fail("At least one Home widget must be visible");
    return true;
}

inline String serializeJson(const HomeLayoutConfig& config) {
    JsonDocument doc;
    doc["customized"] = config.customized;
    JsonArray widgets = doc["widgets"].to<JsonArray>();
    for (uint8_t i = 0; i < config.widgetCount && i < MaxHomeLayoutWidgets; ++i) {
        const HomeLayoutWidgetConfig& widget = config.widgets[i];
        JsonObject item = widgets.add<JsonObject>();
        item["id"] = widget.id;
        item["type"] = widget.type;
        item["visible"] = widget.visible;
        item["x"] = widget.x;
        item["y"] = widget.y;
        item["width"] = widget.width;
        item["height"] = widget.height;
    }
    String json;
    ::serializeJson(doc, json);
    return json;
}

inline bool parseJson(const String& json, HomeLayoutConfig& config, String* error = nullptr) {
    JsonDocument doc;
    const DeserializationError jsonError = deserializeJson(doc, json);
    if (jsonError) {
        if (error != nullptr) *error = "Invalid Home layout JSON";
        return false;
    }

    HomeLayoutConfig parsed;
    parsed.customized = doc["customized"] | false;
    JsonArray widgets = doc["widgets"].as<JsonArray>();

    if (!parsed.customized) {
        parsed.widgetCount = 0;
        if (!validate(parsed, error)) return false;
        config = parsed;
        return true;
    }

    if (widgets.isNull() || widgets.size() == 0 || widgets.size() > MaxHomeLayoutWidgets) {
        if (error != nullptr) *error = "Invalid Home widget count";
        return false;
    }

    for (JsonObject item : widgets) {
        HomeLayoutWidgetConfig& widget = parsed.widgets[parsed.widgetCount++];
        widget.id = String(item["id"] | "");
        widget.type = String(item["type"] | "");
        widget.visible = item["visible"] | true;
        widget.x = item["x"] | 0;
        widget.y = item["y"] | 0;
        widget.width = item["width"] | 0;
        widget.height = item["height"] | 0;
    }

    if (!validate(parsed, error)) return false;
    config = parsed;
    return true;
}

inline void buildDefault(const DataModel& dm, ScreenLayout& layout) {
    layout.clear();

    const bool showWeather = dm.weather.enabled;
    const bool showEnergy = dm.solar.enabled;

    if (showWeather && showEnergy) {
        layout.add("weather-card", LayoutWidgetType::HomeWeatherCard, 75, 63, 225, 402);
        layout.add("energy-card", LayoutWidgetType::HomeEnergyCard, 315, 63, 225, 402);
        layout.add("indoor-card", LayoutWidgetType::HomeIndoorCard, 555, 63, 230, 402);
    } else if (showWeather) {
        layout.add("weather-card", LayoutWidgetType::HomeWeatherCard, 75, 63, 345, 402);
        layout.add("indoor-card", LayoutWidgetType::HomeIndoorCard, 435, 63, 350, 402);
    } else if (showEnergy) {
        layout.add("energy-card", LayoutWidgetType::HomeEnergyCard, 75, 63, 465, 402);
        layout.add("indoor-card", LayoutWidgetType::HomeIndoorCard, 555, 63, 230, 402);
    } else {
        layout.add("indoor-card", LayoutWidgetType::HomeIndoorCard, 75, 63, 710, 402);
    }
}

inline void buildResolved(const HomeLayoutConfig& config, const DataModel& dm, ScreenLayout& layout) {
    if (!config.customized) {
        buildDefault(dm, layout);
        return;
    }

    layout.clear();
    for (uint8_t i = 0; i < config.widgetCount && i < MaxHomeLayoutWidgets; ++i) {
        const HomeLayoutWidgetConfig& widget = config.widgets[i];
        if (!widget.visible) continue;
        if (widget.type == "weather" && !dm.weather.enabled) continue;
        if (widget.type == "energy" && !dm.solar.enabled) continue;

        layout.add(
            widget.id.c_str(),
            runtimeType(widget.type),
            widget.x,
            widget.y,
            widget.width,
            widget.height);
    }

    // A custom layout can temporarily resolve to zero cards when all of its
    // source-dependent widgets are disabled. Keep Home usable in that state.
    if (layout.count() == 0) buildDefault(dm, layout);
}

} // namespace HomeLayout
