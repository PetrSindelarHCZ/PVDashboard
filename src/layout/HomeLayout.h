#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../display/assets/WidgetIcons.h"
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

inline uint8_t rfSlotNumber(const String& slotId) {
    if (!slotId.startsWith("sensor")) return 0;
    const int value = slotId.substring(6).toInt();
    if (value < 1 || value > MaxRfSensors) return 0;
    if (slotId != "sensor" + String(value)) return 0;
    return static_cast<uint8_t>(value);
}

inline String rfSlotId(uint8_t slot) {
    if (slot < 1 || slot > MaxRfSensors) return "";
    return "sensor" + String(slot);
}

inline bool knownDataSource(const String& source) {
    if (source.startsWith("rf.sensor")) {
        const int metricSeparator = source.indexOf('.', 3);
        if (metricSeparator > 3) {
            const String slotId = source.substring(3, metricSeparator);
            const String metric = source.substring(metricSeparator + 1);
            if (rfSlotNumber(slotId) > 0 &&
                (metric == "temperatureC" || metric == "humidityPercent")) {
                return true;
            }
        }
    }

    static const char* sources[] = {
        "solar.productionPowerW",
        "solar.houseConsumptionW",
        "solar.gridPowerW",
        "solar.energyTodayKWh",
        "solar.batterySocPercent",
        "solar.batteryPowerW",
        "azrouter.gridPowerW",
        "azrouter.gridL1PowerW",
        "azrouter.gridL2PowerW",
        "azrouter.gridL3PowerW",
        "azrouter.gridL1VoltageV",
        "azrouter.gridL2VoltageV",
        "azrouter.gridL3VoltageV",
        "azrouter.gridL1CurrentA",
        "azrouter.gridL2CurrentA",
        "azrouter.gridL3CurrentA",
        "azrouter.routedPowerW",
        "azrouter.routedL1PowerW",
        "azrouter.routedL2PowerW",
        "azrouter.routedL3PowerW",
        "azrouter.routedEnergyTodayKWh",
        "azrouter.routedEnergyWeekKWh",
        "azrouter.routedEnergyMonthKWh",
        "azrouter.routedEnergyYearKWh",
        "azrouter.routedEnergyTotalKWh",
        "azrouter.systemTempC",
        "weather.outdoorTempC",
        "weather.outdoorHumidityPercent",
        "weather.surfacePressureHpa",
        "weather.windSpeedKmh",
        "inside.temperatureC",
        "inside.humidityPercent",
        "inside.pressureHpa",
        "inside.livingRoomTempC",
        "inside.bedroomTempC",
        "inside.poolTempC",
        "battery.voltageV",
        "battery.socPercent",
        "battery.changeRatePercentPerHour",
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
    if (source == "solar.productionPowerW" ||
        source == "solar.houseConsumptionW" ||
        source == "inside.temperatureC" ||
        source == "inside.humidityPercent" ||
        source == "inside.pressureHpa") return true;

    if (source.startsWith("rf.sensor")) {
        const int metricSeparator = source.indexOf('.', 3);
        if (metricSeparator > 3) {
            const String slotId = source.substring(3, metricSeparator);
            const String metric = source.substring(metricSeparator + 1);
            return rfSlotNumber(slotId) > 0 &&
                   (metric == "temperatureC" || metric == "humidityPercent");
        }
    }
    return false;
}

inline bool knownElementType(const String& type) {
    return type == "text" || type == "kpi" ||
           type == "progress" || type == "sparkline" ||
           type == "trend" || type == "minmax";
}

inline bool usesHistoryPeriod(const CustomWidgetElementConfig& element) {
    return element.type == "sparkline" ||
           element.type == "trend" ||
           element.type == "minmax";
}

inline uint8_t historyPeriodHours(const HomeLayoutWidgetConfig& widget) {
    for (const CustomWidgetElementConfig& element : widget.elements) {
        if (usesHistoryPeriod(element) &&
            sensorGraphPeriodIndex(element.graphPeriodHours) >= 0) {
            return element.graphPeriodHours;
        }
    }
    return 12;
}

inline int16_t elementMinWidth(const String& type) {
    if (type == "text") return 40;
    if (type == "kpi") return 70;
    if (type == "progress") return 90;
    if (type == "sparkline") return 120;
    if (type == "trend") return 24;
    if (type == "minmax") return 90;
    return 0;
}

inline int16_t elementMinHeight(const String& type) {
    if (type == "text") return 20;
    if (type == "kpi") return 25;
    if (type == "progress") return 35;
    if (type == "sparkline") return 60;
    if (type == "trend") return 24;
    if (type == "minmax") return 40;
    return 0;
}

inline bool knownWidget(const String& id, const String& type) {
    if (type == "custom") return validIdentifier(id) && id.startsWith("custom-");
    if (type == "rf-sensor") return validIdentifier(id) && id.startsWith("rf-card-");
    return (id == "weather-card" && type == "weather") ||
           (id == "energy-card" && type == "energy") ||
           (id == "indoor-card" && type == "indoor") ||
           (id == "fve-summary" && type == "fve-summary") ||
           (id == "azrouter-summary" && type == "azrouter-summary") ||
           (id == "pool-summary" && type == "pool-summary") ||
           (id == "consumption-summary" && type == "consumption-summary");
}

inline int16_t minWidth(const String& type) {
    if (type == "weather") return 190;
    if (type == "energy") return 190;
    if (type == "indoor") return 150;
    if (type == "fve-summary") return 190;
    if (type == "azrouter-summary") return 190;
    if (type == "pool-summary") return 150;
    if (type == "consumption-summary") return 180;
    if (type == "rf-sensor") return 150;
    if (type == "custom") return 160;
    return 0;
}

inline int16_t minHeight(const String& type) {
    if (type == "weather") return 180;
    if (type == "energy") return 330;
    if (type == "indoor") return 140;
    if (type == "fve-summary") return 180;
    if (type == "azrouter-summary") return 180;
    if (type == "pool-summary") return 140;
    if (type == "consumption-summary") return 140;
    if (type == "rf-sensor") return 140;
    if (type == "custom") return 120;
    return 0;
}

inline LayoutWidgetType runtimeType(const String& type) {
    if (type == "weather") return LayoutWidgetType::HomeWeatherCard;
    if (type == "energy") return LayoutWidgetType::HomeEnergyCard;
    if (type == "fve-summary") return LayoutWidgetType::HomeFveCard;
    if (type == "azrouter-summary") return LayoutWidgetType::HomeAZRouterCard;
    if (type == "pool-summary") return LayoutWidgetType::HomePoolCard;
    if (type == "consumption-summary") return LayoutWidgetType::HomeConsumptionCard;
    if (type == "rf-sensor") return LayoutWidgetType::HomeRfSensorCard;
    if (type == "custom") return LayoutWidgetType::HomeCustomCard;
    return LayoutWidgetType::HomeIndoorCard;
}

inline const char* typeName(LayoutWidgetType type) {
    switch (type) {
        case LayoutWidgetType::HomeWeatherCard: return "weather";
        case LayoutWidgetType::HomeEnergyCard: return "energy";
        case LayoutWidgetType::HomeIndoorCard: return "indoor";
        case LayoutWidgetType::HomeFveCard: return "fve-summary";
        case LayoutWidgetType::HomeAZRouterCard: return "azrouter-summary";
        case LayoutWidgetType::HomePoolCard: return "pool-summary";
        case LayoutWidgetType::HomeConsumptionCard: return "consumption-summary";
        case LayoutWidgetType::HomeRfSensorCard: return "rf-sensor";
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

inline bool parseCustomFontSize(const String& value, uint8_t& px) {
    if (value == "auto") {
        px = 0;
        return true;
    }

    // Backward compatibility with layouts created before numeric font sizes.
    if (value == "small") {
        px = 16;
        return true;
    }
    if (value == "normal") {
        px = 18;
        return true;
    }
    if (value == "large") {
        px = 22;
        return true;
    }

    const int numeric = value.toInt();
    if (numeric < 7 || numeric > 64 || String(numeric) != value) return false;
    px = static_cast<uint8_t>(numeric);
    return true;
}

inline int16_t requiredElementHeight(const CustomWidgetElementConfig& element) {
    const int16_t baseHeight = elementMinHeight(element.type);
    uint8_t fontPx = 0;
    if (!parseCustomFontSize(element.fontSize, fontPx)) return baseHeight;

    if (element.type == "text") {
        if (fontPx == 0) return baseHeight;
        return fontPx > baseHeight ? fontPx : baseHeight;
    }

    if (element.type == "kpi") {
        const bool hasLabel = element.showLabel && !element.label.isEmpty();
        const int16_t valueHeight =
            fontPx == 0 ? baseHeight : (fontPx > baseHeight ? fontPx : baseHeight);
        return static_cast<int16_t>(valueHeight + (hasLabel ? 20 : 0));
    }

    if (element.type == "minmax") {
        const int16_t rowHeight =
            fontPx == 0 ? 18 : (fontPx > 18 ? fontPx : 18);
        return static_cast<int16_t>(rowHeight * 2 + 4);
    }

    return baseHeight;
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
        uint8_t fontPx = 0;
        if (!parseCustomFontSize(element.fontSize, fontPx)) {
            return fail("Custom font size must be auto or 7 to 64 px");
        }
        if (!(element.align == "left" || element.align == "center" || element.align == "right")) {
            return fail("Unknown custom alignment");
        }
        if (element.verticalAlign > 2) {
            return fail("Unknown custom vertical alignment");
        }
        if (!(element.graphStyle == "line" || element.graphStyle == "bars")) {
            return fail("Unknown custom graph style");
        }
        if (sensorGraphPeriodIndex(element.graphPeriodHours) < 0) {
            return fail("Graph period must be 1,2,4,6,12,24,48 or 72 hours");
        }
        if (element.type != "sparkline" && element.graphStyle != "line") {
            return fail("Graph style is valid only for sparkline elements");
        }

        const int16_t requiredHeight = requiredElementHeight(element);
        if (element.width < elementMinWidth(element.type) ||
            element.height < requiredHeight) {
            return fail("Custom element is smaller than its supported minimum");
        }

        // Elements may use the complete widget surface, including the
        // header/chrome area and the physical edges of the card.
        if (element.x < 0 || element.y < 0 ||
            element.x + element.width > widget.width ||
            element.y + element.height > widget.height) {
            return fail("Custom element is outside its widget");
        }

        if (usesHistoryPeriod(element)) {
            const uint8_t sharedPeriod = historyPeriodHours(widget);
            if (element.graphPeriodHours != sharedPeriod) {
                return fail("Historical elements in one widget must use the same period");
            }
        }

        if (element.type == "text") {
            if (element.text.isEmpty()) return fail("Text element requires text");
        } else {
            if (!knownDataSource(element.source)) return fail("Unknown custom data source");
            if ((element.type == "sparkline" || element.type == "trend" ||
                 element.type == "minmax") &&
                !knownSparklineSource(element.source)) {
                return fail("Trend element source has no history");
            }
            if (element.type == "progress" && !(element.maxValue > element.minValue)) {
                return fail("Progress range is invalid");
            }
        }

        for (uint8_t j = 0; j < i; ++j) {
            const CustomWidgetElementConfig& previous = widget.elements[j];
            if (element.id == previous.id) return fail("Duplicate custom element id");
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
        return fail("Custom Home layout must contain 1 to 7 widgets");
    }

    bool anyVisible = false;
    for (uint8_t i = 0; i < config.widgetCount; ++i) {
        const HomeLayoutWidgetConfig& widget = config.widgets[i];
        if (!knownWidget(widget.id, widget.type)) return fail("Unknown Home widget");
        if (!(widget.background == "white" || widget.background == "black")) {
            return fail("Unknown Home widget background");
        }
        if (!WidgetIcons::valid(widget.icon)) {
            return fail("Unknown Home widget icon");
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

        if (widget.type == "custom") {
            String customError;
            if (!validateCustomWidget(widget, &customError)) return fail(customError);
        } else if (widget.type == "weather" || widget.type == "energy") {
            if (!widget.elements.empty()) {
                String elementError;
                if (!validateCustomWidget(widget, &elementError)) return fail(elementError);
            }
        } else if (widget.type == "indoor") {
            if (widget.title.length() > 40)
                return fail("Indoor widget title is too long");
            if (!widget.elements.empty()) {
                String elementError;
                if (!validateCustomWidget(widget, &elementError)) return fail(elementError);
                for (const CustomWidgetElementConfig& element : widget.elements) {
                    if (element.type != "text" && !element.source.startsWith("inside.")) {
                        return fail("Indoor widget element must use BME280 data");
                    }
                }
            }
        } else if (widget.type == "pool-summary") {
            if (widget.title.length() > 40)
                return fail("Pool widget title is too long");
            if (!widget.elements.empty()) {
                if (widget.rfSensorSlot == 0 || widget.rfSensorSlot > MaxRfSensors)
                    return fail("Pool widget requires a valid RF sensor slot");
                String elementError;
                if (!validateCustomWidget(widget, &elementError)) return fail(elementError);
                const String rfPrefix = "rf." + rfSlotId(widget.rfSensorSlot) + ".";
                for (const CustomWidgetElementConfig& element : widget.elements) {
                    if (element.type != "text" && !element.source.startsWith(rfPrefix)) {
                        return fail("Pool widget element must use its selected RF sensor");
                    }
                }
            }
        } else if (widget.type == "rf-sensor") {
            if (widget.rfSensorSlot == 0 || widget.rfSensorSlot > MaxRfSensors)
                return fail("RF sensor widget requires a valid sensor slot");
            if (widget.title.length() > 40)
                return fail("RF sensor widget title is too long");
            if (!widget.elements.empty()) {
                String elementError;
                if (!validateCustomWidget(widget, &elementError)) return fail(elementError);
                const String rfPrefix = "rf." + rfSlotId(widget.rfSensorSlot) + ".";
                for (const CustomWidgetElementConfig& element : widget.elements) {
                    if (element.type != "text" && !element.source.startsWith(rfPrefix)) {
                        return fail("RF widget element must use its selected sensor");
                    }
                }
            }
        } else if (!widget.elements.empty() || !widget.title.isEmpty()) {
            return fail("Predefined widget cannot contain custom elements");
        }

        anyVisible = anyVisible || widget.visible;

        for (uint8_t j = 0; j < i; ++j) {
            const HomeLayoutWidgetConfig& previous = config.widgets[j];
            if (widget.id == previous.id) return fail("Duplicate Home widget id");
            if (widget.type != "custom" && widget.type != "rf-sensor" &&
                widget.type == previous.type) {
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
    item["verticalAlign"] = element.verticalAlign;
    item["showLabel"] = element.showLabel;
    item["graphStyle"] = element.graphStyle;
}

inline void serializeStorageElement(JsonObject item, const CustomWidgetElementConfig& element) {
    item["id"] = element.id;
    item["type"] = element.type;
    if (!element.source.isEmpty()) item["source"] = element.source;
    if (!element.label.isEmpty()) item["label"] = element.label;
    if (!element.unit.isEmpty()) item["unit"] = element.unit;
    if (!element.text.isEmpty()) item["text"] = element.text;
    item["x"] = element.x;
    item["y"] = element.y;
    item["width"] = element.width;
    item["height"] = element.height;

    if (element.decimals != 1) item["decimals"] = element.decimals;
    if (element.minValue != 0.0f) item["min"] = element.minValue;
    if (element.maxValue != 100.0f) item["max"] = element.maxValue;
    if (element.fontSize != "auto") item["fontSize"] = element.fontSize;
    if (element.align != "left") item["align"] = element.align;
    if (element.verticalAlign != 0) item["verticalAlign"] = element.verticalAlign;
    if (!element.showLabel) item["showLabel"] = false;
    if (element.graphStyle != "line") item["graphStyle"] = element.graphStyle;
}

inline String serializeStorageJson(const HomeLayoutConfig& config) {
    JsonDocument doc;
    doc["customized"] = config.customized;

    if (config.customized) {
        JsonArray widgets = doc["widgets"].to<JsonArray>();
        for (uint8_t i = 0; i < config.widgetCount && i < MaxHomeLayoutWidgets; ++i) {
            const HomeLayoutWidgetConfig& widget = config.widgets[i];
            JsonObject item = widgets.add<JsonObject>();
            item["id"] = widget.id;
            item["type"] = widget.type;
            item["x"] = widget.x;
            item["y"] = widget.y;
            item["width"] = widget.width;
            item["height"] = widget.height;

            if (!widget.visible) item["visible"] = false;
            if (!widget.showFrame) item["showFrame"] = false;
            if (widget.background != "white") item["background"] = widget.background;
            if (widget.inverseText) item["inverseText"] = true;
            if (widget.icon != static_cast<uint8_t>(WidgetIcons::Icon::Auto))
                item["icon"] = WidgetIcons::key(static_cast<WidgetIcons::Icon>(widget.icon));
            if (!widget.title.isEmpty()) item["title"] = widget.title;
            const uint8_t sharedHistoryPeriod = historyPeriodHours(widget);
            if (sharedHistoryPeriod != 12)
                item["historyPeriodHours"] = sharedHistoryPeriod;

            if (widget.type == "pool-summary" || widget.type == "rf-sensor") {
                const String slotId = rfSlotId(widget.rfSensorSlot);
                if (!slotId.isEmpty()) item["rfSensorSlotId"] = slotId;
            }
            if (widget.type == "rf-sensor") {
                if (!widget.rfShowHumidity) item["rfShowHumidity"] = false;
                if (!widget.rfShowLastSeen) item["rfShowLastSeen"] = false;
            }

            if (!widget.elements.empty()) {
                JsonArray elements = item["elements"].to<JsonArray>();
                for (uint8_t e = 0;
                     e < widget.elements.size() && e < MaxCustomWidgetElements;
                     ++e) {
                    serializeStorageElement(
                        elements.add<JsonObject>(), widget.elements[e]);
                }
            }
        }
    }

    String json;
    ::serializeJson(doc, json);
    return json;
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
        item["showFrame"] = widget.showFrame;
        item["background"] = widget.background;
        item["inverseText"] = widget.inverseText;
        item["icon"] =
            WidgetIcons::key(static_cast<WidgetIcons::Icon>(widget.icon));
        item["historyPeriodHours"] = historyPeriodHours(widget);

        if (widget.type == "weather" || widget.type == "energy" ||
            widget.type == "indoor" || widget.type == "pool-summary") {
            item["title"] = widget.title;
            if (widget.type == "pool-summary")
                item["rfSensorSlotId"] = rfSlotId(widget.rfSensorSlot);
            if (!widget.elements.empty()) {
                JsonArray elements = item["elements"].to<JsonArray>();
                for (uint8_t e = 0; e < widget.elements.size() && e < MaxCustomWidgetElements; ++e) {
                    serializeElement(elements.add<JsonObject>(), widget.elements[e]);
                }
            }
        } else if (widget.type == "rf-sensor") {
            item["title"] = widget.title;
            item["rfSensorSlotId"] = rfSlotId(widget.rfSensorSlot);
            item["rfShowHumidity"] = widget.rfShowHumidity;
            item["rfShowLastSeen"] = widget.rfShowLastSeen;
            if (!widget.elements.empty()) {
                JsonArray elements = item["elements"].to<JsonArray>();
                for (uint8_t e = 0; e < widget.elements.size() && e < MaxCustomWidgetElements; ++e) {
                    serializeElement(elements.add<JsonObject>(), widget.elements[e]);
                }
            }
        } else if (widget.type == "custom") {
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
    if (element.fontSize == "small") element.fontSize = "16";
    else if (element.fontSize == "normal") element.fontSize = "18";
    else if (element.fontSize == "large") element.fontSize = "22";
    element.align = String(item["align"] | "left");
    element.verticalAlign = item["verticalAlign"] | 0;
    element.showLabel = item["showLabel"] | true;
    element.graphStyle = String(item["graphStyle"] | "line");
    element.graphPeriodHours = item["graphPeriodHours"] | 12;
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
        widget.showFrame = item["showFrame"] | true;
        widget.background = String(item["background"] | "white");
        widget.inverseText = item["inverseText"] | false;
        widget.icon = static_cast<uint8_t>(
            WidgetIcons::fromKey(String(item["icon"] | "auto")));
        uint8_t sharedHistoryPeriod = item["historyPeriodHours"] | 0;

        if (widget.type == "weather" || widget.type == "energy" ||
            widget.type == "indoor" || widget.type == "pool-summary") {
            if (widget.type == "weather")
                widget.title = String(item["title"] | "VENKU");
            else if (widget.type == "energy")
                widget.title = String(item["title"] | "ENERGIE");
            else
                widget.title = String(item["title"] | (widget.type == "indoor" ? "UVNITŘ" : "BAZÉN"));
            if (widget.type == "pool-summary")
                widget.rfSensorSlot = rfSlotNumber(String(item["rfSensorSlotId"] | ""));
            JsonArray elements = item["elements"].as<JsonArray>();
            if (!elements.isNull()) {
                if (elements.size() > MaxCustomWidgetElements) {
                    if (error != nullptr) *error = "Too many sensor widget elements";
                    return false;
                }
                for (JsonObject elementItem : elements) {
                    CustomWidgetElementConfig element;
                    parseElement(elementItem, element);
                    widget.elements.push_back(element);
                }
            }
        } else if (widget.type == "rf-sensor") {
            widget.title = String(item["title"] | "VENKU");
            widget.rfSensorSlot = rfSlotNumber(String(item["rfSensorSlotId"] | ""));
            widget.rfShowHumidity = item["rfShowHumidity"] | true;
            widget.rfShowLastSeen = item["rfShowLastSeen"] | true;
            JsonArray elements = item["elements"].as<JsonArray>();
            if (!elements.isNull()) {
                if (elements.size() > MaxCustomWidgetElements) {
                    if (error != nullptr) *error = "Too many RF widget elements";
                    return false;
                }
                for (JsonObject elementItem : elements) {
                    CustomWidgetElementConfig element;
                    parseElement(elementItem, element);
                    widget.elements.push_back(element);
                }
            }
        } else if (widget.type == "custom") {
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

        // Shared widget history period is kept in the already heap-backed
        // element configuration so HomeLayoutWidgetConfig does not grow in static DRAM.
        // New layouts carry historyPeriodHours at widget level; old layouts are
        // migrated from the first historical element.
        if (sensorGraphPeriodIndex(sharedHistoryPeriod) < 0) {
            sharedHistoryPeriod = 12;
            for (const CustomWidgetElementConfig& element : widget.elements) {
                if (usesHistoryPeriod(element) &&
                    sensorGraphPeriodIndex(element.graphPeriodHours) >= 0) {
                    sharedHistoryPeriod = element.graphPeriodHours;
                    break;
                }
            }
        }
        for (CustomWidgetElementConfig& element : widget.elements) {
            if (usesHistoryPeriod(element))
                element.graphPeriodHours = sharedHistoryPeriod;
        }
    }

    if (!validate(parsed, error)) return false;
    config = parsed;
    return true;
}

inline void buildDefault(const DataModel& dm, ScreenLayout& layout) {
    layout.clear();

    // Seven-card overview matching the visual Home concept:
    // 3 large source cards above, 4 compact operational cards below.
    if (dm.weather.enabled)
        layout.add("weather-card", LayoutWidgetType::HomeWeatherCard, 75, 63, 225, 215);

    if (dm.solar.enabled)
        layout.add("fve-summary", LayoutWidgetType::HomeFveCard, 315, 63, 225, 215);

    if (dm.azrouter.enabled)
        layout.add("azrouter-summary", LayoutWidgetType::HomeAZRouterCard, 555, 63, 230, 215);

    bool hasRfTemperature = false;
    for (uint8_t i = 0; i < dm.rfSensors.sensorCount && i < MaxRfSensors; ++i) {
        if (dm.rfSensors.sensors[i].configured && dm.rfSensors.sensors[i].hasTemperature) {
            hasRfTemperature = true;
            break;
        }
    }

    if (hasRfTemperature) {
        layout.add("indoor-card", LayoutWidgetType::HomeIndoorCard, 75, 293, 155, 172);
        layout.add("rf-card-1", LayoutWidgetType::HomeRfSensorCard, 240, 293, 155, 172);
        if (dm.pool.enabled)
            layout.add("pool-summary", LayoutWidgetType::HomePoolCard, 405, 293, 155, 172);
        if (dm.solar.enabled)
            layout.add("consumption-summary", LayoutWidgetType::HomeConsumptionCard, 570, 293, 215, 172);
    } else {
        layout.add("indoor-card", LayoutWidgetType::HomeIndoorCard, 75, 293, 210, 172);
        if (dm.pool.enabled)
            layout.add("pool-summary", LayoutWidgetType::HomePoolCard, 295, 293, 210, 172);
        if (dm.solar.enabled)
            layout.add("consumption-summary", LayoutWidgetType::HomeConsumptionCard, 515, 293, 270, 172);
    }
}

inline bool buildDefaultWidget(const DataModel& dm, const String& id,
                              HomeLayoutWidgetConfig& result) {
    result = HomeLayoutWidgetConfig();
    result.visible = true;
    result.showFrame = true;
    result.background = "white";
    result.inverseText = false;

    auto set = [&](const char* widgetId, const char* type,
                   int16_t x, int16_t y, int16_t w, int16_t h) {
        result.id = widgetId;
        result.type = type;
        result.x = x;
        result.y = y;
        result.width = w;
        result.height = h;
        return true;
    };

    if (id == "weather-card") return set("weather-card", "weather", 75, 63, 225, 215);
    if (id == "fve-summary") return set("fve-summary", "fve-summary", 315, 63, 225, 215);
    if (id == "azrouter-summary") return set("azrouter-summary", "azrouter-summary", 555, 63, 230, 215);
    if (id == "indoor-card") {
        if (!set("indoor-card", "indoor", 75, 293, 155, 172)) return false;
        result.title = "UVNITŘ";

        CustomWidgetElementConfig temperature;
        temperature.id = "temperature";
        temperature.type = "kpi";
        temperature.source = "inside.temperatureC";
        temperature.unit = "°C";
        temperature.x = 10;
        temperature.y = 48;
        temperature.width = 135;
        temperature.height = 38;
        temperature.decimals = 1;
        temperature.fontSize = "28";
        temperature.align = "left";
        temperature.showLabel = false;
        result.elements.push_back(temperature);

        CustomWidgetElementConfig humidity;
        humidity.id = "humidity";
        humidity.type = "kpi";
        humidity.source = "inside.humidityPercent";
        humidity.unit = "%";
        humidity.x = 10;
        humidity.y = 96;
        humidity.width = 62;
        humidity.height = 28;
        humidity.decimals = 0;
        humidity.fontSize = "18";
        humidity.align = "left";
        humidity.showLabel = false;
        result.elements.push_back(humidity);

        CustomWidgetElementConfig pressure;
        pressure.id = "pressure";
        pressure.type = "kpi";
        pressure.source = "inside.pressureHpa";
        pressure.unit = "hPa";
        pressure.x = 10;
        pressure.y = 128;
        pressure.width = 135;
        pressure.height = 28;
        pressure.decimals = 0;
        pressure.fontSize = "16";
        pressure.align = "left";
        pressure.showLabel = false;
        result.elements.push_back(pressure);
        return true;
    }
    if (id == "pool-summary") {
        if (!set("pool-summary", "pool-summary", 405, 293, 155, 172)) return false;
        result.title = "BAZÉN";
        return true;
    }
    if (id == "consumption-summary") return set("consumption-summary", "consumption-summary", 570, 293, 215, 172);

    if (id == "rf-card-1") {
        if (!set("rf-card-1", "rf-sensor", 240, 293, 155, 172)) return false;
        result.title = "VENKU";
        for (uint8_t i = 0; i < dm.rfSensors.sensorCount && i < MaxRfSensors; ++i) {
            const RfSensorData& sensor = dm.rfSensors.sensors[i];
            if (sensor.configured && sensor.hasTemperature && !sensor.slotId.isEmpty()) {
                result.rfSensorSlot = rfSlotNumber(sensor.slotId);
                return true;
            }
        }
        return false;
    }

    // Legacy editor template retained for older saved layouts.
    if (id == "energy-card") return set("energy-card", "energy", 315, 63, 225, 402);
    return false;
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
        if ((widget.type == "energy" || widget.type == "fve-summary" ||
             widget.type == "consumption-summary") && !dm.solar.enabled) continue;
        if (widget.type == "azrouter-summary" && !dm.azrouter.enabled) continue;
        if (widget.type == "pool-summary" && !dm.pool.enabled) continue;

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
