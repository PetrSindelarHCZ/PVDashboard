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

inline bool validIdentifier(const String& id) {
    if (id.isEmpty() || id.length() > 32) return false;
    for (size_t i = 0; i < id.length(); ++i) {
        const char c = id[i];
        if (!(isAlphaNumeric(c) || c == '-' || c == '_')) return false;
    }
    return true;
}

inline bool knownDataSource(const String& source) {
    static const char* sources[] = {
        "solar.productionPowerW",
        "solar.houseConsumptionW",
        "solar.gridPowerW",
        "solar.energyTodayKWh",
        "solar.batterySocPercent",
        "solar.batteryPowerW",
        "azrouter.gridPowerW",
        "azrouter.routedPowerW",
        "azrouter.routedEnergyTodayKWh",
        "azrouter.boilerTempC",
        "weather.outdoorTempC",
        "weather.outdoorHumidityPercent",
        "weather.surfacePressureHpa",
        "weather.windSpeedKmh",
        "inside.livingRoomTempC",
        "inside.bedroomTempC",
        "inside.poolTempC",
        "pool.waterTempC",
        "pool.targetTempC",
        "pool.ph",
        "pool.freeChlorineMgL",
        "pool.airTempC",
        "pool.airHumidityPercent",
        "system.wifiRssi",
        "system.uptimeSeconds"
    };
    for (const char* candidate : sources) {
        if (source == candidate) return true;
    }
    return false;
}

inline bool knownSparklineSource(const String& source) {
    return source == "solar.productionPowerW" ||
           source == "solar.houseConsumptionW";
}

inline bool knownElementType(const String& type) {
    return type == "text" || type == "kpi" ||
           type == "progress" || type == "sparkline";
}

inline int16_t elementMinWidth(const String& type) {
    if (type == "text") return 40;
    if (type == "kpi") return 70;
    if (type == "progress") return 90;
    if (type == "sparkline") return 120;
    return 0;
}

inline int16_t elementMinHeight(const String& type) {
    if (type == "text") return 20;
    if (type == "kpi") return 45;
    if (type == "progress") return 35;
    if (type == "sparkline") return 60;
    return 0;
}

inline bool knownWidget(const String& id, const String& type) {
    if (type == "custom") return validIdentifier(id) && id.startsWith("custom-");
    return (id == "weather-card" && type == "weather") ||
           (id == "energy-card" && type == "energy") ||
           (id == "indoor-card" && type == "indoor");
}

inline int16_t minWidth(const String& type) {
    if (type == "weather") return 190;
    if (type == "energy") return 190;
    if (type == "indoor") return 180;
    if (type == "custom") return 160;
    return 0;
}

inline int16_t minHeight(const String& type) {
    if (type == "weather") return 360;
    if (type == "energy") return 330;
    if (type == "indoor") return 260;
    if (type == "custom") return 120;
    return 0;
}

inline LayoutWidgetType runtimeType(const String& type) {
    if (type == "weather") return LayoutWidgetType::HomeWeatherCard;
    if (type == "energy") return LayoutWidgetType::HomeEnergyCard;
    if (type == "custom") return LayoutWidgetType::HomeCustomCard;
    return LayoutWidgetType::HomeIndoorCard;
}

inline const char* typeName(LayoutWidgetType type) {
    switch (type) {
        case LayoutWidgetType::HomeWeatherCard: return "weather";
        case LayoutWidgetType::HomeEnergyCard: return "energy";
        case LayoutWidgetType::HomeIndoorCard: return "indoor";
        case LayoutWidgetType::HomeCustomCard: return "custom";
    }
    return "unknown";
}

inline bool rectanglesIntersect(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                                int16_t bx, int16_t by, int16_t bw, int16_t bh) {
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

inline bool intersects(const HomeLayoutWidgetConfig& a, const HomeLayoutWidgetConfig& b) {
    return rectanglesIntersect(a.x, a.y, a.width, a.height,
                               b.x, b.y, b.width, b.height);
}

inline bool validateCustomWidget(const HomeLayoutWidgetConfig& widget, String* error = nullptr) {
    auto fail = [error](const String& message) {
        if (error != nullptr) *error = message;
        return false;
    };

    if (widget.title.length() > 40) return fail("Custom widget title is too long");
    if (widget.elements.size() == 0 || widget.elements.size() > MaxCustomWidgetElements) {
        return fail("Custom widget must contain 1 to 8 elements");
    }

    for (uint8_t i = 0; i < widget.elements.size(); ++i) {
        const CustomWidgetElementConfig& element = widget.elements[i];

        if (!validIdentifier(element.id)) return fail("Invalid custom element id");
        if (!knownElementType(element.type)) return fail("Unknown custom element type");
        if (element.label.length() > 40 || element.unit.length() > 16 ||
            element.text.length() > 80) {
            return fail("Custom element text is too long");
        }
        if (element.decimals > 3) return fail("Custom KPI decimals must be 0 to 3");
        if (!(element.fontSize == "auto" || element.fontSize == "small" ||
              element.fontSize == "normal" || element.fontSize == "large")) {
            return fail("Unknown custom font size");
        }
        if (!(element.align == "left" || element.align == "center" || element.align == "right")) {
            return fail("Unknown custom alignment");
        }
        if (!(element.graphStyle == "line" || element.graphStyle == "bars")) {
            return fail("Unknown custom graph style");
        }
        if (element.type != "sparkline" && element.graphStyle != "line") {
            return fail("Graph style is valid only for sparkline elements");
        }

        int16_t requiredHeight = elementMinHeight(element.type);
        if (element.type == "text" && element.fontSize == "large" && requiredHeight < 28) {
            requiredHeight = 28;
        }
        if (element.width < elementMinWidth(element.type) ||
            element.height < requiredHeight) {
            return fail("Custom element is smaller than its supported minimum");
        }

        // The first 38 px are reserved for the card title/chrome.
        if (element.x < 8 || element.y < 40 ||
            element.x + element.width > widget.width - 8 ||
            element.y + element.height > widget.height - 8) {
            return fail("Custom element is outside its widget");
        }

        if (element.type == "text") {
            if (element.text.isEmpty()) return fail("Text element requires text");
        } else {
            if (!knownDataSource(element.source)) return fail("Unknown custom data source");
            if (element.type == "sparkline" && !knownSparklineSource(element.source)) {
                return fail("Sparkline source has no history");
            }
            if (element.type == "progress" && !(element.maxValue > element.minValue)) {
                return fail("Progress range is invalid");
            }
        }

        for (uint8_t j = 0; j < i; ++j) {
            const CustomWidgetElementConfig& previous = widget.elements[j];
            if (element.id == previous.id) return fail("Duplicate custom element id");
            if (rectanglesIntersect(element.x, element.y, element.width, element.height,
                                    previous.x, previous.y, previous.width, previous.height)) {
                return fail("Custom elements overlap");
            }
        }
    }

    return true;
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
        return fail("Custom Home layout must contain 1 to 6 widgets");
    }

    bool anyVisible = false;
    for (uint8_t i = 0; i < config.widgetCount; ++i) {
        const HomeLayoutWidgetConfig& widget = config.widgets[i];
        if (!knownWidget(widget.id, widget.type)) return fail("Unknown Home widget");

        if (widget.width < minWidth(widget.type) ||
            widget.height < minHeight(widget.type)) {
            return fail("Home widget is smaller than its supported minimum");
        }

        if (widget.x < ContentLeft || widget.y < ContentTop ||
            widget.x + widget.width > ContentRight ||
            widget.y + widget.height > ContentBottom) {
            return fail("Home widget is outside the content area");
        }

        if (widget.type == "custom") {
            String customError;
            if (!validateCustomWidget(widget, &customError)) return fail(customError);
        } else if (!widget.elements.empty() || !widget.title.isEmpty()) {
            return fail("Predefined widget cannot contain custom elements");
        }

        anyVisible = anyVisible || widget.visible;

        for (uint8_t j = 0; j < i; ++j) {
            const HomeLayoutWidgetConfig& previous = config.widgets[j];
            if (widget.id == previous.id) return fail("Duplicate Home widget id");
            if (widget.type != "custom" && widget.type == previous.type) {
                return fail("Duplicate predefined Home widget");
            }
            if (widget.visible && previous.visible && intersects(widget, previous)) {
                return fail("Visible Home widgets overlap");
            }
        }
    }

    if (!anyVisible) return fail("At least one Home widget must be visible");
    return true;
}

inline void serializeElement(JsonObject item, const CustomWidgetElementConfig& element) {
    item["id"] = element.id;
    item["type"] = element.type;
    item["source"] = element.source;
    item["label"] = element.label;
    item["unit"] = element.unit;
    item["text"] = element.text;
    item["x"] = element.x;
    item["y"] = element.y;
    item["width"] = element.width;
    item["height"] = element.height;
    item["decimals"] = element.decimals;
    item["min"] = element.minValue;
    item["max"] = element.maxValue;
    item["fontSize"] = element.fontSize;
    item["align"] = element.align;
    item["showLabel"] = element.showLabel;
    item["graphStyle"] = element.graphStyle;
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

        if (widget.type == "custom") {
            item["title"] = widget.title;
            JsonArray elements = item["elements"].to<JsonArray>();
            for (uint8_t e = 0; e < widget.elements.size() && e < MaxCustomWidgetElements; ++e) {
                serializeElement(elements.add<JsonObject>(), widget.elements[e]);
            }
        }
    }
    String json;
    ::serializeJson(doc, json);
    return json;
}

inline bool parseElement(JsonObject item, CustomWidgetElementConfig& element) {
    element.id = String(item["id"] | "");
    element.type = String(item["type"] | "");
    element.source = String(item["source"] | "");
    element.label = String(item["label"] | "");
    element.unit = String(item["unit"] | "");
    element.text = String(item["text"] | "");
    element.x = item["x"] | 0;
    element.y = item["y"] | 0;
    element.width = item["width"] | 0;
    element.height = item["height"] | 0;
    element.decimals = item["decimals"] | 1;
    element.minValue = item["min"] | 0.0f;
    element.maxValue = item["max"] | 100.0f;
    element.fontSize = String(item["fontSize"] | "auto");
    element.align = String(item["align"] | "left");
    element.showLabel = item["showLabel"] | true;
    element.graphStyle = String(item["graphStyle"] | "line");
    return true;
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

        if (widget.type == "custom") {
            widget.title = String(item["title"] | "");
            JsonArray elements = item["elements"].as<JsonArray>();
            if (!elements.isNull()) {
                if (elements.size() > MaxCustomWidgetElements) {
                    if (error != nullptr) *error = "Too many custom widget elements";
                    return false;
                }
                for (JsonObject elementItem : elements) {
                    CustomWidgetElementConfig element;
                    parseElement(elementItem, element);
                    widget.elements.push_back(element);
                }
            }
        }
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

    if (layout.count() == 0) buildDefault(dm, layout);
}

inline const HomeLayoutWidgetConfig* findWidget(const HomeLayoutConfig& config, const char* id) {
    if (!id) return nullptr;
    for (uint8_t i = 0; i < config.widgetCount && i < MaxHomeLayoutWidgets; ++i) {
        if (config.widgets[i].id == id) return &config.widgets[i];
    }
    return nullptr;
}

} // namespace HomeLayout
