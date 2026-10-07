#include "DashboardApp.h"
#include <math.h>
#include <new>
#include <time.h>
#include "../diagnostics/Performance.h"
#include "../integrations/cc1101/Cc1101Diagnostics.h"
#include "../integrations/cc1101/Cc1101RawReceiver.h"
#include "../screens/ScreenStyle.h"
#include "../layout/HomeLayout.h"
#include "../../include/AppConfig.h"
#include "../../include/Version.h"

namespace {
constexpr uint32_t MinimumPollIntervalMs = 1000;
constexpr uint32_t MaximumBackoffMs = 300000;
constexpr uint8_t MaximumBackoffShift = 5;
constexpr uint32_t Bme280PollIntervalMs = 10000;
constexpr uint32_t Bme280DisplayRefreshIntervalMs = 60000;
constexpr uint32_t BatteryPollIntervalMs = 10000;
constexpr uint32_t BatteryDisplayRefreshIntervalMs = 60000;

uint32_t pollDelayMs(uint32_t intervalSeconds, uint8_t failureStreak) {
    uint64_t baseMs = static_cast<uint64_t>(intervalSeconds) * 1000ULL;
    if (baseMs < MinimumPollIntervalMs) baseMs = MinimumPollIntervalMs;
    const uint8_t shift = failureStreak < MaximumBackoffShift ? failureStreak : MaximumBackoffShift;
    uint64_t delayMs = baseMs << shift;
    const uint64_t maximumMs = baseMs > MaximumBackoffMs ? baseMs : MaximumBackoffMs;
    if (delayMs > maximumMs) delayMs = maximumMs;
    return static_cast<uint32_t>(delayMs);
}

uint8_t nextFailureStreak(uint8_t current) {
    return current < MaximumBackoffShift ? current + 1 : MaximumBackoffShift;
}

void applyGoodWeWorkerResult(
    SolarData& target,
    const SolarData& source,
    bool success) {

    target.enabled = source.enabled;
    target.status = source.status;

    if (!success) return;

    target.productionPowerW = source.productionPowerW;
    target.houseConsumptionW = source.houseConsumptionW;
    target.gridPowerW = source.gridPowerW;
    target.energyTodayKWh = source.energyTodayKWh;
    target.batteryPresent = source.batteryPresent;
    target.batterySocPercent = source.batterySocPercent;
    target.batteryPowerW = source.batteryPowerW;
    target.lastUpdateMs = source.lastUpdateMs;
}

void sampleInsideHistory(InsideData& data, InsideHistory*& history) {
    if (history == nullptr) {
        history = new (std::nothrow) InsideHistory();
        if (history == nullptr) {
            Serial.println(
                "[BME280] Historie nelze alokovat: nedostatek heap pameti.");
            return;
        }
    }
    data.history = history;

    InsideHistorySample sample;
    sample.temperatureCenti =
        static_cast<int16_t>(lroundf(data.temperatureC * 100.0f));
    int humidity = data.humidityPercent;
    if (humidity < 0) humidity = 0;
    if (humidity > 100) humidity = 100;
    sample.humidityPercent = static_cast<uint8_t>(humidity);

    float pressure = data.pressureHpa * 10.0f;
    if (pressure < 0.0f) pressure = 0.0f;
    if (pressure > 65535.0f) pressure = 65535.0f;
    sample.pressureDeciHpa = static_cast<uint16_t>(lroundf(pressure));
    sample.flags = 0x07;

    const time_t epoch = time(nullptr);
    const bool wallClock = epoch > 1700000000;
    const uint32_t timeSeconds =
        wallClock ? static_cast<uint32_t>(epoch) : millis() / 1000UL;

    for (uint8_t periodIndex = 0;
         periodIndex < SensorGraphPeriodCount;
         ++periodIndex) {

        const uint8_t hours = SensorGraphPeriodHours[periodIndex];
        const uint32_t bucketSeconds = sensorGraphBucketSeconds(hours);
        if (bucketSeconds == 0) continue;

        const uint32_t bucket = timeSeconds / bucketSeconds;
        InsideHistorySeries& series = history->series[periodIndex];

        auto appendSample =
            [&series](const InsideHistorySample& value) {
                series.samples[series.next] = value;
                series.next = static_cast<uint8_t>(
                    (series.next + 1) % SensorGraphSampleCount);
                if (series.count < SensorGraphSampleCount) ++series.count;
            };

        if (series.count == 0 || series.wallClock != wallClock) {
            series = InsideHistorySeries{};
            series.wallClock = wallClock;
            series.lastBucket = bucket;
            appendSample(sample);
            continue;
        }

        if (bucket == series.lastBucket) {
            const uint8_t latest =
                static_cast<uint8_t>(
                    (series.next + SensorGraphSampleCount - 1) %
                    SensorGraphSampleCount);
            series.samples[latest] = sample;
            continue;
        }

        if (bucket < series.lastBucket ||
            bucket - series.lastBucket >= SensorGraphSampleCount) {
            series = InsideHistorySeries{};
            series.wallClock = wallClock;
            series.lastBucket = bucket;
            appendSample(sample);
            continue;
        }

        const uint32_t gap = bucket - series.lastBucket;
        for (uint32_t step = 1; step < gap; ++step) {
            appendSample(InsideHistorySample{});
        }
        appendSample(sample);
        series.lastBucket = bucket;
    }
}

bool applyWifiAddressing(const WifiConfig& wifi) {
    if (wifi.dhcp) {
        // Při přechodu ze statické adresy nejdřív ukončíme aktivní STA spojení.
        // Jinak může lwIP ještě krátce obsluhovat původní statickou IP, než se
        // DHCP klient skutečně rozběhne a získá novou lease.
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[WIFI] Prechod na DHCP: odpojuji STA pred zmenou adresace...");
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(false);
            const unsigned long disconnectStarted = millis();
            while (WiFi.status() == WL_CONNECTED && millis() - disconnectStarted < 500UL) {
                delay(5);
            }
        }
        const bool ok = WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        Serial.printf("[WIFI] IP konfigurace: DHCP (%s)\n", ok ? "OK" : "CHYBA");
        return ok;
    }

    IPAddress ip, gateway, subnet, dns1, dns2;
    if (!ip.fromString(wifi.ipAddress) || !gateway.fromString(wifi.gateway) ||
        !subnet.fromString(wifi.subnetMask) || !dns1.fromString(wifi.dns1)) {
        Serial.println("[WIFI] Neplatna staticka IP konfigurace.");
        return false;
    }
    if (!wifi.dns2.isEmpty()) dns2.fromString(wifi.dns2);
    const bool ok = WiFi.config(ip, gateway, subnet, dns1, dns2);
    Serial.printf("[WIFI] IP konfigurace: STATIC %s / %s | GW %s | DNS %s%s%s (%s)\n",
                  wifi.ipAddress.c_str(), wifi.subnetMask.c_str(), wifi.gateway.c_str(), wifi.dns1.c_str(),
                  wifi.dns2.isEmpty() ? "" : ", ", wifi.dns2.c_str(), ok ? "OK" : "CHYBA");
    return ok;
}

bool isWeatherScreenId(const String& screenId) {
    return screenId == "weather" || screenId.startsWith("weather-hourly-");
}

int weatherLocationIndexById(const WeatherConfig& weather, const String& locationId) {
    for (uint8_t i = 0;
         i < weather.locationCount && i < MaxWeatherLocations;
         ++i) {
        if (weather.locations[i].id == locationId) return static_cast<int>(i);
    }
    return -1;
}

const char* weatherProviderLabel(const String& provider) {
    if (provider == "met-no") return "MET Norway";
    if (provider == "open-meteo") return "Open-Meteo";
    return provider.c_str();
}

String rfSensorBaseLabel(const RfSensorConfig& sensor) {
    if (!sensor.name.isEmpty()) return sensor.name;

    String label = sensor.protocol;
    label += " 0x";
    label += String(sensor.sensorId, HEX);
    if (sensor.channel > 0) {
        label += " CH";
        label += String(sensor.channel);
    }
    return label;
}

String rfSensorMetricLabel(const RfSensorConfig& sensor, bool humidity) {
    String label = rfSensorBaseLabel(sensor);
    label += humidity ? " – vlhkost" : " – teplota";
    return label;
}

const RfSensorConfig* rfSensorBySlot(
    const RfSensorsConfig& config,
    const String& slotId) {
    for (uint8_t i = 0; i < config.sensorCount && i < MaxRfSensors; ++i) {
        if (config.sensors[i].slotId == slotId) return &config.sensors[i];
    }
    return nullptr;
}

bool weatherDisplayDataChanged(const WeatherData& a, const WeatherData& b) {
    return a.status.available != b.status.available ||
           a.status.lastError != b.status.lastError ||
           a.lastUpdateMs != b.lastUpdateMs ||
           a.provider != b.provider ||
           a.locationId != b.locationId ||
           a.locationName != b.locationName ||
           a.dailyCount != b.dailyCount ||
           a.hourlyCount != b.hourlyCount;
}

DisplayRegion navigationFocusMarkerRegion(const NavigationRect& bounds) {
    DisplayRegion region;
    region.x = max<int16_t>(0, bounds.x + bounds.width - 31);
    region.y = max<int16_t>(0, bounds.y + 2);
    region.width = min<int16_t>(28, ScreenStyle::Width - region.x);
    region.height = min<int16_t>(26, ScreenStyle::Height - region.y);
    return region;
}

DisplayRegion expandedNavigationRegion(const NavigationRect& bounds) {
    constexpr int16_t Margin = 8;

    int16_t x1 = bounds.x - Margin;
    int16_t y1 = bounds.y - Margin;
    int16_t x2 = bounds.x + bounds.width + Margin;
    int16_t y2 = bounds.y + bounds.height + Margin;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > ScreenStyle::Width) x2 = ScreenStyle::Width;
    if (y2 > ScreenStyle::Height) y2 = ScreenStyle::Height;

    DisplayRegion region;
    region.x = x1;
    region.y = y1;
    region.width = x2 - x1;
    region.height = y2 - y1;
    return region;
}

DisplayRegion unionDisplayRegions(const DisplayRegion& a, const DisplayRegion& b) {
    if (!a.valid()) return b;
    if (!b.valid()) return a;

    const int16_t x1 = min(a.x, b.x);
    const int16_t y1 = min(a.y, b.y);
    const int16_t x2 = max(
        static_cast<int16_t>(a.x + a.width),
        static_cast<int16_t>(b.x + b.width));
    const int16_t y2 = max(
        static_cast<int16_t>(a.y + a.height),
        static_cast<int16_t>(b.y + b.height));

    DisplayRegion region;
    region.x = x1;
    region.y = y1;
    region.width = x2 - x1;
    region.height = y2 - y1;
    return region;
}

DisplayRegion headerRegion() {
    DisplayRegion region;
    region.x = 0;
    region.y = 0;
    region.width = ScreenStyle::Width;
    region.height = ScreenStyle::HeaderHeight;
    return region;
}

DisplayRegion sidebarRegion() {
    DisplayRegion region;
    region.x = 0;
    region.y = ScreenStyle::HeaderHeight;
    region.width = ScreenStyle::SidebarWidth + 1;
    region.height = ScreenStyle::Height - ScreenStyle::HeaderHeight;
    return region;
}

DisplayRegion pageRegion() {
    DisplayRegion region;
    region.x = ScreenStyle::SidebarWidth + 1;
    region.y = ScreenStyle::HeaderHeight;
    region.width = ScreenStyle::Width - region.x;
    region.height = ScreenStyle::Height - ScreenStyle::HeaderHeight;
    return region;
}

DisplayRegion solarLiveDataRegion() {
    DisplayRegion region;
    // Only the changing values of the three upper GoodWe cards.
    // Frames/titles stay untouched; the renderer clips its normal full-page
    // drawing to this interior strip and therefore also clears old digits.
    region.x = 83;
    region.y = 136;
    region.width = 694;
    region.height = 119;
    return region;
}

DisplayRegion solarHistoryDataRegion() {
    DisplayRegion region;
    // Plot + axes/labels inside the lower history card, excluding static
    // title/frame chrome.
    region.x = 83;
    region.y = 315;
    region.width = 694;
    region.height = 142;
    return region;
}

DisplayRegion azRouterMasterDataRegion() {
    DisplayRegion region;
    // Dynamic values inside the top AZRouter master card.
    region.x = 83;
    region.y = 132;
    region.width = 694;
    region.height = 63;
    return region;
}

DisplayRegion azRouterPhaseDataRegion() {
    DisplayRegion region;
    // Dynamic values inside the three phase cards, excluding frames/titles.
    region.x = 83;
    region.y = 246;
    region.width = 694;
    region.height = 111;
    return region;
}

DisplayRegion azRouterEnergyDataRegion() {
    DisplayRegion region;
    // Only the numeric energy totals inside the lower energy card.
    region.x = 83;
    region.y = 416;
    region.width = 694;
    region.height = 41;
    return region;
}

DisplayRegion dashboardBodyRegion() {
    DisplayRegion region;
    region.x = 0;
    region.y = ScreenStyle::HeaderHeight;
    region.width = ScreenStyle::Width;
    region.height = ScreenStyle::Height - ScreenStyle::HeaderHeight;
    return region;
}

DisplayRegion homeEnergyFlowDataRegion(const LayoutWidget& widget) {
    DisplayRegion region;
    // Energy-flow has a large static illustration. Its live values and
    // directional arrows occupy only the upper/middle band of the card.
    // Keeping the minute refresh out of the lower half avoids a visually
    // near-full-card e-paper flash.
    region.x = widget.x + 8;
    region.y = widget.y + 38;
    region.width = widget.width > 16 ? widget.width - 16 : widget.width;

    // Keep this comfortably below the hybrid driver's 96k-pixel threshold
    // so the proven register-partial waveform is used. For the current
    // ~524 px-wide card, 170 px is ~89k pixels.
    const int16_t desiredHeight = 170;
    const int16_t availableHeight =
        widget.height > 50 ? widget.height - 50 : widget.height;
    region.height = min<int16_t>(desiredHeight, availableHeight);
    return region;
}

enum class HomeDataGroup : uint8_t {
    Weather,
    Energy,
    Indoor,
    Battery,
    Rf
};

bool widgetUsesGroup(
    const HomeLayoutWidgetConfig& widget,
    HomeDataGroup group) {

    for (const auto& element : widget.elements) {
        const String& source = element.source;
        switch (group) {
            case HomeDataGroup::Weather:
                if (source.startsWith("weather.")) return true;
                break;
            case HomeDataGroup::Energy:
                if (source.startsWith("solar.") ||
                    source.startsWith("azrouter.")) return true;
                break;
            case HomeDataGroup::Indoor:
                if (source.startsWith("inside.")) return true;
                break;
            case HomeDataGroup::Battery:
                if (source.startsWith("battery.")) return true;
                break;
            case HomeDataGroup::Rf:
                if (source.startsWith("rf.")) return true;
                break;
        }
    }
    return false;
}

bool widgetUsesRfSlot(
    const HomeLayoutWidgetConfig& widget,
    const String& slotId) {

    const String prefix = "rf." + slotId + ".";
    for (const auto& element : widget.elements) {
        if (element.source.startsWith(prefix)) return true;
    }
    return false;
}

uint8_t configuredRfSlotForObservation(
    const RfSensorsConfig& config,
    const RfSensorObservation& observation) {

    for (uint8_t i = 0; i < config.sensorCount && i < MaxRfSensors; ++i) {
        const RfSensorConfig& sensor = config.sensors[i];
        if (sensor.protocol != observation.protocol ||
            sensor.sensorId != observation.sensorId ||
            sensor.channel != observation.channel) {
            continue;
        }

        if (!sensor.slotId.startsWith("sensor")) return 0;
        const int slot = sensor.slotId.substring(6).toInt();
        return slot >= 1 && slot <= MaxRfSensors
            ? static_cast<uint8_t>(slot)
            : 0;
    }
    return 0;
}

DisplayRegion homeRfSensorRegion(
    const HomeLayoutConfig& config,
    const DataModel& dataModel,
    uint8_t rfSensorSlot) {

    if (rfSensorSlot == 0 || rfSensorSlot > MaxRfSensors) {
        return DisplayRegion();
    }

    const String slotId = "sensor" + String(rfSensorSlot);
    ScreenLayout layout;
    HomeLayout::buildResolved(config, dataModel, layout);

    DisplayRegion dirty;
    for (uint8_t i = 0; i < layout.count(); ++i) {
        const LayoutWidget& widget = layout[i];
        const HomeLayoutWidgetConfig* configured =
            HomeLayout::findWidget(config, widget.id);
        bool matches = false;

        if (widget.type == LayoutWidgetType::HomeRfSensorCard) {
            matches =
                configured != nullptr &&
                configured->rfSensorSlot == rfSensorSlot;
        } else if (configured != nullptr && !configured->elements.empty()) {
            matches = widgetUsesRfSlot(*configured, slotId);
        }

        if (!matches) continue;

        DisplayRegion widgetRegion;
        if (widget.type == LayoutWidgetType::HomeCustomCard) {
            widgetRegion.x = widget.x;
            widgetRegion.y = widget.y;
            widgetRegion.width = widget.width;
            widgetRegion.height = widget.height;
        } else {
            widgetRegion.x = widget.x + 8;
            widgetRegion.y = widget.y + 38;
            widgetRegion.width =
                widget.width > 16 ? widget.width - 16 : widget.width;
            widgetRegion.height =
                widget.height > 46 ? widget.height - 46 : widget.height;
        }
        dirty = unionDisplayRegions(dirty, widgetRegion);
    }

    return dirty;
}

uint8_t homeDataRegions(
    const HomeLayoutConfig& config,
    const DataModel& dataModel,
    HomeDataGroup group,
    DisplayRegion* regions,
    uint8_t capacity) {

    if (regions == nullptr || capacity == 0) return 0;

    ScreenLayout layout;
    HomeLayout::buildResolved(config, dataModel, layout);

    uint8_t count = 0;
    for (uint8_t i = 0; i < layout.count() && count < capacity; ++i) {
        const LayoutWidget& widget = layout[i];
        bool matches = false;

        switch (widget.type) {
            case LayoutWidgetType::HomeWeatherCard:
            case LayoutWidgetType::HomeEnergyCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                if (configured != nullptr && !configured->elements.empty()) {
                    matches = widgetUsesGroup(*configured, group);
                } else if (widget.type == LayoutWidgetType::HomeWeatherCard) {
                    matches = group == HomeDataGroup::Weather;
                } else {
                    matches = group == HomeDataGroup::Energy;
                }
                break;
            }
            case LayoutWidgetType::HomeIndoorCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                matches = configured != nullptr && !configured->elements.empty()
                    ? widgetUsesGroup(*configured, group)
                    : group == HomeDataGroup::Indoor;
                break;
            }
            case LayoutWidgetType::HomeFveCard:
            case LayoutWidgetType::HomeAZRouterCard:
                matches = group == HomeDataGroup::Energy;
                break;
            case LayoutWidgetType::HomePoolCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                matches = configured != nullptr && !configured->elements.empty()
                    ? widgetUsesGroup(*configured, group)
                    : group == HomeDataGroup::Indoor;
                break;
            }
            case LayoutWidgetType::HomeConsumptionCard:
            case LayoutWidgetType::HomeEnergyFlowCard:
                matches = group == HomeDataGroup::Energy;
                break;
            case LayoutWidgetType::HomeRfSensorCard:
                matches = group == HomeDataGroup::Rf;
                break;
            case LayoutWidgetType::HomeCustomCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                matches =
                    configured != nullptr &&
                    widgetUsesGroup(*configured, group);
                break;
            }
        }

        if (!matches) continue;

        DisplayRegion region;
        if (widget.type == LayoutWidgetType::HomeCustomCard) {
            region.x = widget.x;
            region.y = widget.y;
            region.width = widget.width;
            region.height = widget.height;
        } else if (widget.type == LayoutWidgetType::HomeEnergyFlowCard) {
            region = homeEnergyFlowDataRegion(widget);
        } else {
            region.x = widget.x + 8;
            region.y = widget.y + 38;
            region.width =
                widget.width > 16 ? widget.width - 16 : widget.width;
            region.height =
                widget.height > 46 ? widget.height - 46 : widget.height;
        }

        if (region.valid()) regions[count++] = region;
    }

    return count;
}

DisplayRegion homeDataRegion(
    const HomeLayoutConfig& config,
    const DataModel& dataModel,
    HomeDataGroup group) {

    ScreenLayout layout;
    HomeLayout::buildResolved(config, dataModel, layout);

    DisplayRegion dirty;
    for (uint8_t i = 0; i < layout.count(); ++i) {
        const LayoutWidget& widget = layout[i];
        bool matches = false;

        switch (widget.type) {
            case LayoutWidgetType::HomeWeatherCard:
            case LayoutWidgetType::HomeEnergyCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                if (configured != nullptr && !configured->elements.empty()) {
                    matches = widgetUsesGroup(*configured, group);
                } else if (widget.type == LayoutWidgetType::HomeWeatherCard) {
                    matches = group == HomeDataGroup::Weather;
                } else {
                    matches = group == HomeDataGroup::Energy;
                }
                break;
            }
            case LayoutWidgetType::HomeIndoorCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                matches = configured != nullptr && !configured->elements.empty()
                    ? widgetUsesGroup(*configured, group)
                    : group == HomeDataGroup::Indoor;
                break;
            }
            case LayoutWidgetType::HomeFveCard:
            case LayoutWidgetType::HomeAZRouterCard:
                matches = group == HomeDataGroup::Energy;
                break;
            case LayoutWidgetType::HomePoolCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                matches = configured != nullptr && !configured->elements.empty()
                    ? widgetUsesGroup(*configured, group)
                    : group == HomeDataGroup::Indoor;
                break;
            }
            case LayoutWidgetType::HomeConsumptionCard:
                matches = group == HomeDataGroup::Energy;
                break;
            case LayoutWidgetType::HomeRfSensorCard:
                matches = group == HomeDataGroup::Rf;
                break;
            case LayoutWidgetType::HomeCustomCard: {
                const HomeLayoutWidgetConfig* configured =
                    HomeLayout::findWidget(config, widget.id);
                matches =
                    configured != nullptr &&
                    widgetUsesGroup(*configured, group);
                break;
            }
        }

        if (!matches) continue;

        DisplayRegion widgetRegion;
        if (widget.type == LayoutWidgetType::HomeCustomCard) {
            // Custom elements may occupy the whole card, so keep the complete
            // widget as the dirty region.
            widgetRegion.x = widget.x;
            widgetRegion.y = widget.y;
            widgetRegion.width = widget.width;
            widgetRegion.height = widget.height;
        } else if (widget.type == LayoutWidgetType::HomeEnergyFlowCard) {
            widgetRegion = homeEnergyFlowDataRegion(widget);
        } else {
            // Predefined cards have static frame/title chrome. Automatic data
            // updates only need the inner content area, which makes e-paper
            // activity substantially less visible.
            widgetRegion.x = widget.x + 8;
            widgetRegion.y = widget.y + 38;
            widgetRegion.width = widget.width > 16 ? widget.width - 16 : widget.width;
            widgetRegion.height = widget.height > 46 ? widget.height - 46 : widget.height;
        }
        dirty = unionDisplayRegions(dirty, widgetRegion);
    }

    return dirty;
}

DisplayRegion navigationFocusRegion(
    const NavigationState& state,
    const NavigationLayout& layout) {

    if (state.area != NavigationArea::Page || state.focusId.isEmpty()) {
        return DisplayRegion();
    }

    const int index = layout.find(state.focusId);
    if (index < 0) return DisplayRegion();

    return navigationFocusMarkerRegion(layout.elements[index].bounds);
}

DisplayRegion navigationDirtyRegion(
    const NavigationState& previousState,
    const NavigationLayout& previousLayout,
    const NavigationState& currentState,
    const NavigationLayout& currentLayout) {

    if (previousState.area == NavigationArea::Sidebar &&
        currentState.area == NavigationArea::Sidebar) {
        return sidebarRegion();
    }

    if (previousState.area == NavigationArea::Page &&
        currentState.area == NavigationArea::Page) {
        DisplayRegion dirty;

        const int oldIndex = previousLayout.find(previousState.focusId);
        if (oldIndex >= 0) {
            dirty = navigationFocusMarkerRegion(
                previousLayout.elements[oldIndex].bounds);
        }

        const int newIndex = currentLayout.find(currentState.focusId);
        if (newIndex >= 0) {
            dirty = unionDisplayRegions(
                dirty,
                navigationFocusMarkerRegion(
                    currentLayout.elements[newIndex].bounds));
        }

        return dirty;
    }

    if (previousState.area == NavigationArea::Sidebar &&
        currentState.area == NavigationArea::Page) {
        return navigationFocusRegion(currentState, currentLayout);
    }

    if (previousState.area == NavigationArea::Page &&
        currentState.area == NavigationArea::Sidebar) {
        const int oldIndex = previousLayout.find(previousState.focusId);
        return oldIndex >= 0
            ? navigationFocusMarkerRegion(previousLayout.elements[oldIndex].bounds)
            : DisplayRegion();
    }

    // Pager changes can affect page content/dots and keep the broader handling.
    return DisplayRegion();
}

}

DashboardApp::DashboardApp()
    : _rfSensorManager(_dataModel),
      _epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCK, EPD_MISO, EPD_MOSI),
      _displayPreview(),
      _displayManager(_epaperDisplay, &_displayPreview),
      _displayWorker(_displayManager),
      _navigationController(_screenManager, _dataModel) {
}

void DashboardApp::setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==========================================");
    Serial.printf("  %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);
    Serial.printf("  Build: %s\n", FIRMWARE_BUILD_DATE);
    Serial.println("==========================================");

    // Verify the physically connected 433 MHz transceiver before the display
    // takes ownership of the global SPI object with its own MISO pin.
    Cc1101Diagnostics::probe();
    Cc1101RawReceiver::begin();

    _configManager.begin();

    _webServer = new (std::nothrow) DashboardWebServer(
        80, _dataModel, _screenManager, _configManager.get());
    if (_webServer == nullptr) {
        Serial.println(
            "[APP] CHYBA: WebServer nelze alokovat na heapu. "
            "Pokracuji bez WebUI.");
    }

    _rfSensorManager.applyConfig(_configManager.get().rfSensors);
    Cc1101RawReceiver::onSensorObservation(
        [this](const RfSensorObservation& observation) {
            if (!_rfSensorManager.observe(observation)) return;

            const unsigned long now = millis();
            if (_lastRfSensorDisplayRefresh == 0 ||
                now - _lastRfSensorDisplayRefresh >= 60000UL) {
                _lastRfSensorDisplayRefresh = now;

                if (_screenManager.getActiveScreenId() == "home") {
                    const auto& config = _configManager.get();
                    const uint8_t rfSensorSlot =
                        configuredRfSlotForObservation(
                            config.rfSensors,
                            observation);
                    const DisplayRegion region =
                        homeRfSensorRegion(
                            config.display.homeLayout,
                            _dataModel,
                            rfSensorSlot);
                    if (region.valid()) {
                        requestAutomaticRegionRefresh(
                            region, true, "rf-sensor");
                    }
                }
            }
        });

    _memoryHeavyGate = xSemaphoreCreateMutex();
    if (_memoryHeavyGate == nullptr) {
        Serial.println("[APP] VAROVANI: memory-heavy gate se nepodarilo vytvorit.");
    } else {
        _displayWorker.setMemoryHeavyGate(_memoryHeavyGate);
        _weatherWorker.setMemoryHeavyGate(_memoryHeavyGate);
        Serial.println("[APP] Memory-heavy gate pripraven pro Display/TLS.");
    }

    _networkClientGate = xSemaphoreCreateMutex();
    if (_networkClientGate == nullptr) {
        Serial.println("[APP] VAROVANI: network-client gate se nepodarilo vytvorit.");
    } else {
        if (_webServer != nullptr)
            _webServer->setNetworkClientGate(_networkClientGate);
        _weatherWorker.setNetworkClientGate(_networkClientGate);
        _goodweWorker.setNetworkClientGate(_networkClientGate);
        Serial.println("[APP] Network-client gate pripraven pro Web/Weather/GoodWe/AZRouter.");
    }

    _displayPreview.init(); // tiled preview; komprimovany snapshot se drzi mimo TLS DRAM
    const auto& cfg = _configManager.get();
    _homeScreen.setLayoutConfig(&cfg.display.homeLayout);

    // Local environmental sensor is independent of Wi-Fi. The 4-pin BME280
    // shares the preferred I2C bus on SDA GPIO21 / SCL GPIO22.
    if (_bme280Sensor.update(_dataModel.inside)) {
        sampleInsideHistory(_dataModel.inside, _insideHistory);
    }
    _lastBme280Sync = millis();
    _lastBme280DisplayRefresh = _lastBme280Sync;

    // MAX17048 shares the same I2C bus (SDA GPIO21 / SCL GPIO22).
    _max17048Sensor.update(_dataModel.battery);
    _lastBatterySync = millis();
    _lastBatteryDisplayRefresh = _lastBatterySync;

    applyWifiAddressing(cfg.wifi);

    _dataModel.solar.enabled = cfg.goodwe.enabled;
    _dataModel.azrouter.enabled = cfg.azrouter.enabled;
    registerScreens();
    if (!_screenManager.activateScreen(cfg.display.defaultScreen)) {
        _screenManager.activateScreen("home");
    }
    _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
    _dataModel.pool.enabled = cfg.pool.enabled;
    _dataModel.weather.enabled = cfg.weather.enabled;
    const WeatherLocation* initialWeatherLocation = cfg.weather.activeLocation();
    const int initialWeatherIndex =
        initialWeatherLocation != nullptr
            ? weatherLocationIndexById(cfg.weather, initialWeatherLocation->id)
            : -1;
    _weatherDisplayLocationIndex =
        initialWeatherIndex >= 0 ? static_cast<uint8_t>(initialWeatherIndex) : 0;
    _weatherDisplayLocationId =
        initialWeatherLocation != nullptr ? initialWeatherLocation->id : "";
    _dataModel.weather.locationId = _weatherDisplayLocationId;
    _dataModel.weather.locationName =
        initialWeatherLocation != nullptr ? initialWeatherLocation->name : "";
    _dataModel.weather.locationIndex = _weatherDisplayLocationIndex;
    _dataModel.weather.locationCount =
        min<uint8_t>(cfg.weather.locationCount, MaxWeatherLocations);
    _navigationController.syncToActiveScreen();
    _joystick.begin();

    _wifiManager.onStatusChange([this](bool connected, const String& ip) {
        Serial.printf("[APP] Wi-Fi zmena stavu -> Connected: %d, IP: %s\n", connected, ip.c_str());

        if (connected && _pendingWifiSave && _wifiManager.getSsid() == _pendingWifiSsid) {
            _configManager.setWifi(_pendingWifiSsid, _pendingWifiPassword);
            Serial.printf("[WIFI] Rucne vybrana sit '%s' se uspesne pripojila a byla ulozena.\n",
                          _pendingWifiSsid.c_str());
            _pendingWifiSave = false;
            _pendingWifiSsid = "";
            _pendingWifiPassword = "";
        }

        _dataModel.system.wifiConnected = connected;
        _dataModel.system.wifiSignalLevel = _wifiSignalLevel.update(connected, _wifiManager.getRssi(), millis());
        _dataModel.system.ipAddress = ip;
        requestAutomaticRegionRefresh(headerRegion(), true, "wifi");
    });

    _wifiManager.onKnownNetworkLookup([this](const String& ssid, String& password) {
        return _configManager.getAutoJoinWifiPassword(ssid, password);
    });
    _wifiManager.onAutoNetworkSelected([this](const String& ssid, const String& password) {
        _pendingWifiSave = false;
        _pendingWifiSsid = "";
        _pendingWifiPassword = "";
        _configManager.setWifi(ssid, password);
        Serial.printf("[WIFI] Automaticky obnovena znama sit '%s' a nastavena jako aktivni.\n", ssid.c_str());
        // The status callback refreshes the header when connectivity changes.
    });

    bool wifiOk = false;
    const bool hasConfiguredWifi = !cfg.wifi.ssid.isEmpty() && cfg.wifi.ssid != "VASE_WIFI";
    const bool configuredWifiAllowed = hasConfiguredWifi && _configManager.isWifiAutoConnectEnabled(cfg.wifi.ssid);

    if (configuredWifiAllowed) {
        _wifiManager.begin(cfg.wifi.ssid, cfg.wifi.password, cfg.system.hostname);
        wifiOk = _wifiManager.waitForConnection(8000);
    } else {
        if (hasConfiguredWifi) {
            Serial.printf("[WIFI] Sit '%s' byla rucne odpojena. Po restartu ji automaticky nezkousim.\n", cfg.wifi.ssid.c_str());
        }
        _wifiManager.begin("", "", cfg.system.hostname);
    }

    _timeService.begin(cfg.system.timezone, cfg.system.ntpServer);
    if (wifiOk) {
        unsigned long ntpWait = millis();
        while (!_timeService.isSynced() && (millis() - ntpWait < 2000)) {
            _timeService.loop();
            delay(100);
        }
    }

    const bool previousAccessPoint = _dataModel.system.wifiAccessPoint;
    _dataModel.system.wifiAccessPoint = _wifiManager.isConfigAccessPoint();
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    const uint8_t previousSignalLevel = _dataModel.system.wifiSignalLevel;
    _dataModel.system.wifiSignalLevel = _wifiSignalLevel.update(_dataModel.system.wifiConnected, _dataModel.system.wifiRssi, millis());
    if (previousSignalLevel != _dataModel.system.wifiSignalLevel ||
        previousAccessPoint != _dataModel.system.wifiAccessPoint) {
        const DisplayRegion region = headerRegion();
        requestNavigationDisplayRefresh(false, 0UL, &region, true);
    }
    _dataModel.system.ipAddress = _wifiManager.getIpAddress();
    _dataModel.system.ntpSynced = _timeService.isSynced();
    _dataModel.system.timeStr = _timeService.getTimeStr();
    _dataModel.system.dateStr = _timeService.getDateStr();
    _dataModel.system.dayOfWeekStr = _timeService.getDayOfWeekStr();

    _navigationController.onSubpageChange(
        [this](const String& screenId, uint8_t subpageIndex) {
            onNavigationSubpageChanged(screenId, subpageIndex);
        });

    _navigationController.onChange([this](bool fullRefresh) {
        if (_handlingNavigationInput) {
            _navigationInputChanged = true;
            _navigationInputFullRefresh =
                _navigationInputFullRefresh || fullRefresh;
            return;
        }

        syncWeatherDisplayForActiveScreen(false);
        requestNavigationDisplayRefresh(
            fullRefresh, 50UL, nullptr, true, "navigation-controller-change");
    });

    if (_webServer != nullptr) {
        _webServer->onScreenChange([this](const String& screenId) { onScreenSwitchRequested(screenId); });
        _webServer->onRefresh([this](bool full) { onRefreshRequested(full); });
        _webServer->onDisplayStatus([this]() { return _displayWorker.getStatus(); });
        _webServer->setDisplayPreview(&_displayPreview);
        _webServer->setNavigationController(&_navigationController);
        _webServer->onNavigationAction(
            [this](NavigationAction action) {
                return handleNavigationAction(action, true);
            });
        _webServer->onControlAction(
            [this](ControlAction action) {
                return handleControlAction(action, true);
            });
    
        _webServer->onSystemConfig([this](const SystemConfig& system) {
            const String previousHostname = _configManager.get().system.hostname;
            _configManager.setSystem(system);
            _timeService.begin(system.timezone, system.ntpServer);
            _dataModel.system.ntpSynced = false;
            _dataModel.system.timeStr = "";
            _dataModel.system.dateStr = "";
            _dataModel.system.dayOfWeekStr = "";
            if (previousHostname != system.hostname) {
                const auto& current = _configManager.get();
                applyWifiAddressing(current.wifi);
                if (!current.wifi.ssid.isEmpty() && _configManager.isWifiAutoConnectEnabled(current.wifi.ssid))
                    _wifiManager.begin(current.wifi.ssid, current.wifi.password, system.hostname);
                else _wifiManager.begin("", "", system.hostname);
            }
            requestAutomaticDisplayRefresh();
            Serial.println("[CONFIG] System ulozen a aplikovan za behu.");
        });
    
        _webServer->onWifiConfig([this](const String& ssid, const String& password) {
            const auto& current = _configManager.get();
            _pendingWifiSave = true;
            _pendingWifiSsid = ssid;
            _pendingWifiPassword = password;
            applyWifiAddressing(current.wifi);
            Serial.printf("[WIFI] Rucne zkousim sit '%s'; ulozim ji az po uspesnem pripojeni.\n", ssid.c_str());
            _wifiManager.begin(ssid, password, current.system.hostname);
        });
    
        _webServer->onWifiNetworkConfig([this](const WifiConfig& wifi) {
            _pendingWifiSave = false;
            _pendingWifiSsid = "";
            _pendingWifiPassword = "";
            _configManager.setWifiNetworkConfig(wifi);
            const auto& current = _configManager.get();
            applyWifiAddressing(current.wifi);
            Serial.printf("[CONFIG] IP rezim ulozen: %s. Obnovuji Wi-Fi pripojeni.\n", current.wifi.dhcp ? "DHCP" : "STATIC");
            if (!current.wifi.ssid.isEmpty() && _configManager.isWifiAutoConnectEnabled(current.wifi.ssid))
                _wifiManager.begin(current.wifi.ssid, current.wifi.password, current.system.hostname);
            else _wifiManager.begin("", "", current.system.hostname);
        });
    
        _webServer->onWifiScan([this]() { return _wifiManager.scanNetworksJson(); });
        _webServer->onWifiKnownNetworks([this]() { return _configManager.getKnownWifiNetworksJson(); });
    
        _webServer->onWifiConnectKnown([this](const String& ssid) {
            String password;
            if (!_configManager.getKnownWifiPassword(ssid, password)) return false;
            const auto& current = _configManager.get();
            _pendingWifiSave = true;
            _pendingWifiSsid = ssid;
            _pendingWifiPassword = password;
            applyWifiAddressing(current.wifi);
            Serial.printf("[WIFI] Rucne zkousim znamou sit '%s'; auto-connect povolim az po uspesnem pripojeni.\n", ssid.c_str());
            _wifiManager.begin(ssid, password, current.system.hostname);
            return true;
        });
    
        _webServer->onWifiDisconnect([this]() {
            _pendingWifiSave = false;
            _pendingWifiSsid = "";
            _pendingWifiPassword = "";
            const String activeSsid = _configManager.get().wifi.ssid;
            if (!activeSsid.isEmpty() && activeSsid != "VASE_WIFI") {
                if (_configManager.setWifiAutoConnectEnabled(activeSsid, false))
                    Serial.printf("[WIFI] Sit '%s' byla rucne odpojena a zustane vyradena z auto-connectu do rucniho Pripojit.\n", activeSsid.c_str());
            }
            _wifiManager.disconnectToConfigAccessPoint();
            requestAutomaticDisplayRefresh();
        });
    
        _webServer->onWifiForget([this](const String& ssid) {
            if (_pendingWifiSave && _pendingWifiSsid == ssid) {
                _pendingWifiSave = false;
                _pendingWifiSsid = "";
                _pendingWifiPassword = "";
            }
            const bool wasActive = _configManager.get().wifi.ssid == ssid;
            if (!_configManager.forgetWifi(ssid)) return false;
            if (wasActive) { _wifiManager.disconnectToConfigAccessPoint(); requestAutomaticDisplayRefresh(); }
            Serial.printf("[WIFI] Sit '%s' byla zapomenuta.\n", ssid.c_str());
            return true;
        });
    
        _webServer->onSourceConfig([this](const GoodWeConfig& goodwe, const AZRouterConfig& azrouter) {
            const bool visibilityChanged =
                _configManager.get().goodwe.enabled != goodwe.enabled ||
                _configManager.get().azrouter.enabled != azrouter.enabled;
    
            _configManager.setSources(goodwe, azrouter);
            _dataModel.solar.enabled = goodwe.enabled;
            _dataModel.azrouter.enabled = azrouter.enabled;
    
            if (!_goodweWorker.reconfigure(goodwe)) {
                Serial.println("[CONFIG] Nepodarilo se aplikovat GoodWe worker konfiguraci.");
            }
            if (!goodwe.enabled) {
                _dataModel.solar.status.recordError("Disabled");
                _dataModel.solar.historyCount = 0;
            }
    
            if (azrouter.enabled) {
                _azrouterClient.begin(
                    azrouter.host,
                    azrouter.port,
                    azrouter.authEnabled ? azrouter.username : String(),
                    azrouter.authEnabled ? azrouter.password : String());
            } else {
                _dataModel.azrouter.authenticated = false;
                _dataModel.azrouter.authMode = "disabled";
                _dataModel.azrouter.status.recordError("Disabled");
            }
    
            setSolarScreenEnabled(goodwe.enabled);
            setAZRouterScreenEnabled(azrouter.enabled);
            _navigationController.syncToActiveScreen(false);
    
            _goodweFailureStreak = 0;
            _azrouterFailureStreak = 0;
            _lastGoodweSync = millis() - goodwe.pollIntervalSeconds * 1000UL;
            _lastAzrouterSync = millis() - azrouter.pollIntervalSeconds * 1000UL;
            _dataModel.updateSystemMetrics();
    
            if (visibilityChanged) requestDisplayRefresh(true, 2500, "config-data-source-visibility");
            else requestAutomaticDisplayRefresh();
    
            Serial.println("[CONFIG] Datove zdroje ulozeny a aplikovany za behu.");
        });
    
        _webServer->onPoolConfig([this](const PoolConfig& pool) {
            const bool enabledChanged = _configManager.get().pool.enabled != pool.enabled;
            _configManager.setPool(pool);
            _dataModel.pool.enabled = pool.enabled;
            setPoolScreenEnabled(pool.enabled);
            _navigationController.syncToActiveScreen(false);
            if (enabledChanged) requestDisplayRefresh(true, 2500, "config-pool-enabled");
            else requestAutomaticDisplayRefresh("config-pool-values");
            Serial.println("[CONFIG] Bazen ulozen a aplikovan za behu.");
        });
    
        _webServer->onHomeLayoutConfig([this](const HomeLayoutConfig& layout) {
            if (!_configManager.setHomeLayout(layout)) return false;
            _navigationController.syncToActiveScreen(false);
            requestDisplayRefresh(true, 2500, "config-home-layout");
            Serial.println("[CONFIG] Home layout ulozen a aplikovan za behu.");
            return true;
        });
    
        _webServer->onWeatherConfig([this](const WeatherConfig& weather) {
            // Keep this callback light on loopTask stack. WeatherConfig and
            // especially WeatherData are large enough that local copies here
            // can trip the 8 kB Arduino loopTask stack.
            const WeatherConfig& previous = _configManager.get().weather;
            const bool enabledChanged = previous.enabled != weather.enabled;
            const bool providerChanged = previous.provider != weather.provider;
            const bool activeLocationChanged =
                previous.activeLocationId != weather.activeLocationId;
            const bool locationChanged =
                activeLocationChanged ||
                fabs(previous.latitude - weather.latitude) > 0.00001 ||
                fabs(previous.longitude - weather.longitude) > 0.00001;

            _configManager.setWeather(weather);
            const WeatherConfig& applied = _configManager.get().weather;

            Serial.printf(
                "[CONFIG] Pocasi ulozeno: provider=%s active=%s%s\n",
                applied.provider.c_str(),
                applied.activeLocationId.c_str(),
                activeLocationChanged ? " active-change" : "");

            if (!_weatherWorker.reconfigure(applied)) {
                Serial.println("[CONFIG] Nepodarilo se aplikovat konfiguraci pocasi za behu.");
            }

            const WeatherLocation* activeLocation = applied.activeLocation();
            const int activeIndex =
                activeLocation != nullptr
                    ? weatherLocationIndexById(applied, activeLocation->id)
                    : -1;

            _weatherDisplayLocationIndex =
                activeIndex >= 0 ? static_cast<uint8_t>(activeIndex) : 0;
            _weatherDisplayLocationId =
                activeLocation != nullptr ? activeLocation->id : "";

            if (providerChanged || locationChanged) {
                // Invalidate the existing forecast in place instead of
                // constructing a large temporary WeatherData on loopTask.
                _dataModel.weather.enabled = applied.enabled;
                _dataModel.weather.provider =
                    weatherProviderLabel(applied.provider);
                _dataModel.weather.locationId = _weatherDisplayLocationId;
                _dataModel.weather.locationName =
                    activeLocation != nullptr ? activeLocation->name : "";
                _dataModel.weather.locationIndex =
                    _weatherDisplayLocationIndex;
                _dataModel.weather.locationCount =
                    min<uint8_t>(applied.locationCount, MaxWeatherLocations);
                _dataModel.weather.dailyCount = 0;
                _dataModel.weather.hourlyCount = 0;
                _dataModel.weather.lastUpdateMs = 0;
                _dataModel.weather.status.available = false;
                _dataModel.weather.status.lastSuccessMs = 0;
                _dataModel.weather.status.lastAttemptMs = millis();
                _dataModel.weather.status.lastError = "Aktualizuji pocasi";
            } else {
                selectWeatherDisplayLocation(
                    _weatherDisplayLocationIndex,
                    false);
            }

            if (!applied.enabled) {
                _dataModel.weather.status.recordError("Weather disabled");
            }

            setWeatherScreensEnabled(applied.enabled);
            _navigationController.syncToActiveScreen(false);
            if (enabledChanged) {
                requestDisplayRefresh(true, 2500, "config-weather-enabled");
            } else if (_screenManager.getActiveScreenId() == "home") {
                DisplayRegion regions[MaxHomeLayoutWidgets];
                const uint8_t count =
                    homeDataRegions(
                        _configManager.get().display.homeLayout,
                        _dataModel,
                        HomeDataGroup::Weather,
                        regions,
                        MaxHomeLayoutWidgets);
                for (uint8_t i = 0; i < count; ++i) {
                    requestAutomaticRegionRefresh(
                        regions[i], true, "config-weather-values");
                }
            } else if (isWeatherScreenId(_screenManager.getActiveScreenId())) {
                requestAutomaticRegionRefresh(
                    pageRegion(), true, "config-weather-values");
            }

            Serial.println("[CONFIG] Pocasi ulozeno a aplikovano za behu.");
        });
    
        _webServer->onRfSensorManagement(
            [this]() {
                return _rfSensorManager.statusJson();
            },
            [this](uint32_t durationMs) {
                _rfSensorManager.startScan(durationMs);
                Serial.printf(
                    "[RF-SENSORS] Scan spusten na %lu s.\n",
                    static_cast<unsigned long>(durationMs / 1000UL));
            },
            [this](const String& bindingKey, const String& name, String& error) {
                RfSensorsConfig updated;
                if (!_rfSensorManager.addDiscoveredSensor(
                        bindingKey, name, updated, error)) {
                    return false;
                }
                if (!_configManager.setRfSensors(updated)) {
                    error = "Konfiguraci čidla se nepodařilo uložit.";
                    return false;
                }
                _rfSensorManager.applyConfig(_configManager.get().rfSensors);
                requestAutomaticDisplayRefresh();
                return true;
            },
            [this](const String& slotId, const String& name, String& error) {
                const RfSensorsConfig previousRf = _configManager.get().rfSensors;

                if (slotId == "bme280") {
                    const String oldName =
                        previousRf.bme280Name.isEmpty()
                            ? String("Inside")
                            : previousRf.bme280Name;

                    if (!_configManager.setBme280Name(name)) {
                        error = "Nový název čidla se nepodařilo uložit.";
                        return false;
                    }

                    const String newName =
                        _configManager.get().rfSensors.bme280Name.isEmpty()
                            ? String("Inside")
                            : _configManager.get().rfSensors.bme280Name;

                    // Temperature widgets are user-defined. Renaming the
                    // physical/logical sensor must not rewrite widget titles
                    // or allocate/copy the complete HomeLayoutConfig on loopTask.
                    Serial.printf(
                        "[RF-SENSORS] BME280 prejmenovan: %s -> %s\n",
                        oldName.c_str(),
                        newName.c_str());
                    return true;
                }

                if (!_configManager.renameRfSensor(slotId, name, error)) {
                    return false;
                }

                _rfSensorManager.applyConfig(_configManager.get().rfSensors);

                // Saved sensor names and user-defined widget titles are separate.
                // Do not copy/rewrite Home layout here.
                Serial.printf(
                    "[RF-SENSORS] %s prejmenovan na '%s'.\n",
                    slotId.c_str(),
                    name.c_str());
                return true;
            },
            [this](const String& slotId, const String& bindingKey, String& error) {
                RfSensorsConfig updated;
                if (!_rfSensorManager.rebindSensor(
                        slotId, bindingKey, updated, error)) {
                    return false;
                }
                if (!_configManager.setRfSensors(updated)) {
                    error = "Nové přiřazení čidla se nepodařilo uložit.";
                    return false;
                }
                _rfSensorManager.applyConfig(_configManager.get().rfSensors);
                requestAutomaticDisplayRefresh();
                Serial.printf(
                    "[RF-SENSORS] %s znovu prirazeno na %s.\n",
                    slotId.c_str(),
                    bindingKey.c_str());
                return true;
            },
            [this](const String& slotId, String& error) {
                RfSensorsConfig updated;
                if (!_rfSensorManager.removeSensor(slotId, updated, error)) {
                    return false;
                }
                if (!_configManager.setRfSensors(updated)) {
                    error = "Čidlo se nepodařilo odebrat z konfigurace.";
                    return false;
                }
                _rfSensorManager.applyConfig(_configManager.get().rfSensors);
                requestAutomaticDisplayRefresh();
                return true;
            });
    
        _webServer->onFactoryReset([this]() { return _configManager.resetToFactoryDefaults(); });
        _webServer->onConfigImport([this](const AppConfig& config) { return _configManager.setUserConfiguration(config); });
    
        _webServer->enableTimezoneUiExtension();
        _webServer->begin();
    }

    if (!_goodweWorker.begin(cfg.goodwe)) {
        Serial.println("[GOODWE-WORKER] Inicializace selhala.");
    }
    if (cfg.azrouter.enabled) {
        _azrouterClient.begin(
            cfg.azrouter.host,
            cfg.azrouter.port,
            cfg.azrouter.authEnabled ? cfg.azrouter.username : String(),
            cfg.azrouter.authEnabled ? cfg.azrouter.password : String());
    }
    _weatherWorker.begin(cfg.weather);

    _lastGoodweSync = millis();
    _lastAzrouterSync = millis();
    _displayInitNotBefore = millis() + 1500;
    requestDisplayRefresh(true, 0, "boot");
    Serial.println("[APP] Inicializace uspesne dokoncena.");
}

void DashboardApp::registerScreens() {
    _screenManager.registerScreen(&_homeScreen);
    if (_configManager.get().goodwe.enabled) {
        _screenManager.registerScreen(&_solarScreen);
    }
    if (_configManager.get().azrouter.enabled) {
        _screenManager.registerScreen(&_azrouterScreen);
    }
    if (_configManager.get().pool.enabled) {
        _screenManager.registerScreen(&_poolScreen);
    }
    if (_configManager.get().weather.enabled) {
        _screenManager.registerScreen(&_weatherScreen);
        for (auto& screen : _weatherHourlyScreens) _screenManager.registerScreen(&screen);
    }
    _screenManager.registerScreen(&_diagnosticsScreen);
}

void DashboardApp::setSolarScreenEnabled(bool enabled) {
    const bool solarWasActive = _screenManager.getActiveScreenId() == "solar";

    if (enabled) {
        // Solar belongs directly after Home in the visual sidebar. Inserting it
        // at the canonical position keeps UP/DOWN navigation aligned even when
        // the page is re-enabled at runtime.
        _screenManager.registerScreenAt(&_solarScreen, 1);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("solar");

    if (solarWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 2500, "solar-screen-disabled-while-active");
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::setAZRouterScreenEnabled(bool enabled) {
    const bool azrouterWasActive = _screenManager.getActiveScreenId() == "azrouter";

    if (enabled) {
        const uint8_t position = _configManager.get().goodwe.enabled ? 2 : 1;
        _screenManager.registerScreenAt(&_azrouterScreen, position);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("azrouter");

    if (azrouterWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 2500, "azrouter-screen-disabled-while-active");
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::setPoolScreenEnabled(bool enabled) {
    const bool poolWasActive = _screenManager.getActiveScreenId() == "pool";

    if (enabled) {
        _screenManager.registerScreen(&_poolScreen);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("pool");

    if (poolWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 2500, "pool-screen-disabled-while-active");
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::setWeatherScreensEnabled(bool enabled) {
    const String activeId = _screenManager.getActiveScreenId();
    const bool weatherWasActive = activeId == "weather" || activeId.startsWith("weather-hourly-");

    if (enabled) {
        _screenManager.registerScreen(&_weatherScreen);
        for (auto& screen : _weatherHourlyScreens) _screenManager.registerScreen(&screen);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("weather");
    for (uint8_t i = 0; i < WeatherForecastDayCount; ++i) {
        _screenManager.unregisterScreen("weather-hourly-" + String(i));
    }

    if (weatherWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 2500, "weather-screen-disabled-while-active");
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::requestDisplayRefresh(bool full, unsigned long delayMs,
                                        const char* reason) {
    if (!_displayEnabled) return;

    Serial.printf(
        "[REFRESH][REQ][%lu ms] WHOLE type=%s reason=%s screen=%s delay=%lu ms pending=%d pendingFull=%d\n",
        millis(),
        full ? "FULL" : "PARTIAL",
        reason != nullptr ? reason : "unspecified",
        _screenManager.getActiveScreenId().c_str(),
        delayMs,
        _pendingRefresh,
        _pendingFullRefresh);

    // A whole-screen request supersedes queued automatic dirty regions,
    // because it will render the newest DataModel everywhere.
    clearDeferredAutomaticRegions();

    const bool hadPending = _pendingRefresh;
    _pendingRefresh = true;
    _pendingFullRefresh = _pendingFullRefresh || full;
    if (!hadPending || full || _pendingRefreshReason.isEmpty()) {
        _pendingRefreshReason = reason != nullptr ? reason : "unspecified";
    }

    // Ordinary data refreshes may modify any part of the screen, so they
    // intentionally discard a pending cursor-only dirty region.
    _pendingDisplayRegionValid = false;
    _pendingCapturePreview = true;

    const unsigned long requestedAt = millis() + delayMs;
    if (_displayRefreshNotBefore == 0 ||
        static_cast<long>(requestedAt - _displayRefreshNotBefore) > 0) {
        _displayRefreshNotBefore = requestedAt;
    }
}

void DashboardApp::requestNavigationDisplayRefresh(
    bool full,
    unsigned long delayMs,
    const DisplayRegion* region,
    bool capturePreview,
    const char* reason) {

    if (!_displayEnabled) return;

    if (full || region == nullptr || !region->valid()) {
        Serial.printf(
            "[REFRESH][REQ][%lu ms] NAV type=%s scope=%s reason=%s screen=%s delay=%lu ms pending=%d pendingFull=%d\n",
            millis(),
            full ? "FULL" : "PARTIAL",
            (region != nullptr && region->valid()) ? "REGION" : "WHOLE",
            reason != nullptr ? reason : "navigation",
            _screenManager.getActiveScreenId().c_str(),
            delayMs,
            _pendingRefresh,
            _pendingFullRefresh);
    }

    const bool hadPending = _pendingRefresh;
    _pendingRefresh = true;
    _pendingFullRefresh = _pendingFullRefresh || full;
    if (!hadPending || full || _pendingRefreshReason.isEmpty()) {
        _pendingRefreshReason = reason != nullptr ? reason : "navigation";
    }

    if (_pendingFullRefresh) {
        _pendingDisplayRegionValid = false;
        _pendingCapturePreview = true;
    } else if (!hadPending) {
        if (region != nullptr && region->valid()) {
            _pendingDisplayRegion = *region;
            _pendingDisplayRegionValid = true;
        } else {
            _pendingDisplayRegionValid = false;
        }
        _pendingCapturePreview = capturePreview;
    } else if (_pendingDisplayRegionValid &&
               region != nullptr &&
               region->valid()) {
        _pendingDisplayRegion =
            unionDisplayRegions(_pendingDisplayRegion, *region);
        _pendingCapturePreview =
            _pendingCapturePreview || capturePreview;
    } else {
        // An older pending request already needs the whole screen, or this
        // navigation action itself needs the whole screen.
        _pendingDisplayRegionValid = false;
        _pendingCapturePreview =
            _pendingCapturePreview || capturePreview;
    }

    // Navigation is interactive and outranks a delayed automatic refresh.
    // A short resettable deadline still coalesces contact bounce / rapid taps.
    _displayRefreshNotBefore = millis() + delayMs;
}

void DashboardApp::requestAutomaticDisplayRefresh(const char* reason) {
    if (!_displayEnabled) return;

    unsigned long delayMs = 0;
    if (_lastScreenRender != 0) {
        const unsigned long elapsed = millis() - _lastScreenRender;
        constexpr unsigned long CoalesceWindowMs = 5000;
        if (elapsed < CoalesceWindowMs) delayMs = CoalesceWindowMs - elapsed;
    }
    requestDisplayRefresh(false, delayMs, reason);
}

void DashboardApp::requestAutomaticRegionRefresh(
    const DisplayRegion& region,
    bool capturePreview,
    const char* reason) {

    if (!_displayEnabled || !region.valid()) return;

    // A queued/running FULL refresh already renders the newest DataModel for
    // the whole screen. Do not accumulate automatic dirty regions while that
    // refresh is still in the display worker; otherwise they run immediately
    // after the full refresh and cause redundant e-paper flashing.
    const DisplayTaskStatus displayStatus = _displayWorker.getStatus();
    if (displayStatus.pendingFull ||
        displayStatus.state == DisplayTaskState::RenderingFull) {
        return;
    }

    const auto sameRegion = [](const DisplayRegion& a, const DisplayRegion& b) {
        return a.x == b.x &&
               a.y == b.y &&
               a.width == b.width &&
               a.height == b.height;
    };

    const auto containsRegion = [](const DisplayRegion& outer,
                                   const DisplayRegion& inner) {
        return inner.x >= outer.x &&
               inner.y >= outer.y &&
               inner.x + inner.width <= outer.x + outer.width &&
               inner.y + inner.height <= outer.y + outer.height;
    };

    const char* queuedReason =
        reason != nullptr ? reason : "automatic-region";

    if (_pendingRefresh &&
        (_pendingFullRefresh || !_pendingDisplayRegionValid)) {
        _pendingCapturePreview =
            _pendingCapturePreview || capturePreview;
        return;
    }

    if (_pendingRefresh &&
        _pendingDisplayRegionValid &&
        (sameRegion(_pendingDisplayRegion, region) ||
         containsRegion(_pendingDisplayRegion, region))) {
        _pendingCapturePreview =
            _pendingCapturePreview || capturePreview;
        return;
    }

    if (!_pendingRefresh && _deferredAutomaticRegionCount == 0) {
        requestNavigationDisplayRefresh(
            false, 0UL, &region, capturePreview, queuedReason);
        return;
    }

    for (uint8_t i = 0; i < _deferredAutomaticRegionCount; ++i) {
        DeferredAutomaticRefresh& queued = _deferredAutomaticRegions[i];
        if (sameRegion(queued.region, region) ||
            containsRegion(queued.region, region)) {
            queued.capturePreview =
                queued.capturePreview || capturePreview;
            return;
        }
    }

    for (uint8_t i = 0; i < _deferredAutomaticRegionCount;) {
        if (!containsRegion(region, _deferredAutomaticRegions[i].region)) {
            ++i;
            continue;
        }

        capturePreview =
            capturePreview ||
            _deferredAutomaticRegions[i].capturePreview;

        for (uint8_t j = i + 1;
             j < _deferredAutomaticRegionCount;
             ++j) {
            _deferredAutomaticRegions[j - 1] =
                _deferredAutomaticRegions[j];
        }
        --_deferredAutomaticRegionCount;
    }

    if (_deferredAutomaticRegionCount >= MaxDeferredAutomaticRegions) {
        Serial.printf(
            "[DISPLAY][QUEUE] plna %u/%u; zahazuji reason=%s "
            "region=%d,%d %dx%d\n",
            static_cast<unsigned>(_deferredAutomaticRegionCount),
            static_cast<unsigned>(MaxDeferredAutomaticRegions),
            queuedReason,
            region.x,
            region.y,
            region.width,
            region.height);
        return;
    }

    DeferredAutomaticRefresh& slot =
        _deferredAutomaticRegions[_deferredAutomaticRegionCount++];
    slot.region = region;
    slot.capturePreview = capturePreview;
    slot.reason = queuedReason;

    Serial.printf(
        "[DISPLAY][QUEUE] deferred %u/%u reason=%s region=%d,%d %dx%d\n",
        static_cast<unsigned>(_deferredAutomaticRegionCount),
        static_cast<unsigned>(MaxDeferredAutomaticRegions),
        slot.reason,
        slot.region.x,
        slot.region.y,
        slot.region.width,
        slot.region.height);
}

void DashboardApp::clearDeferredAutomaticRegions() {
    _deferredAutomaticRegionCount = 0;
}

bool DashboardApp::popDeferredAutomaticRegion(
    DisplayRegion& region,
    bool& capturePreview,
    String& reason) {

    if (_deferredAutomaticRegionCount == 0) return false;

    const DeferredAutomaticRefresh next =
        _deferredAutomaticRegions[0];

    region = next.region;
    capturePreview = next.capturePreview;
    reason = next.reason != nullptr
        ? next.reason
        : "automatic-region";

    for (uint8_t i = 1;
         i < _deferredAutomaticRegionCount;
         ++i) {
        _deferredAutomaticRegions[i - 1] =
            _deferredAutomaticRegions[i];
    }
    --_deferredAutomaticRegionCount;

    return true;
}


bool DashboardApp::handleNavigationAction(
    NavigationAction action,
    bool capturePreview) {
    if (!_displayEnabled) return false;

    const NavigationState previousNavigation =
        _navigationController.getState();
    _previousNavigationLayout.clear();
    if (previousNavigation.area == NavigationArea::Page) {
        _navigationController.buildCurrentLayout(_previousNavigationLayout);
    }

    _navigationInputChanged = false;
    _navigationInputFullRefresh = false;
    _handlingNavigationInput = true;
    const bool handled = _navigationController.handleAction(action);
    _handlingNavigationInput = false;

    if (!_navigationInputChanged) return handled;

    const NavigationState currentNavigation =
        _navigationController.getState();
    const bool subpageChanged =
        previousNavigation.subpageIndex != currentNavigation.subpageIndex;

    _currentNavigationLayout.clear();
    if (currentNavigation.area == NavigationArea::Page) {
        _navigationController.buildCurrentLayout(_currentNavigationLayout);
    }

    // Moving the cursor inside the sidebar/page does not change the
    // active weather context. Avoid rebuilding WeatherData on every navigation
    // action: that structure is large enough to put loopTask close to its
    // stack limit, especially when navigation comes through WebUI.
    if (_navigationInputFullRefresh) {
        syncWeatherDisplayForActiveScreen(false);
    }

    const DisplayRegion dirtyRegion =
        navigationDirtyRegion(
            previousNavigation,
            _previousNavigationLayout,
            currentNavigation,
            _currentNavigationLayout);

    const bool enteredPageFromSidebar =
        previousNavigation.area == NavigationArea::Sidebar &&
        currentNavigation.area == NavigationArea::Page &&
        !subpageChanged;

    if (_navigationInputFullRefresh) {
        _deferredNavigationRegionValid = false;
        clearDeferredAutomaticRegions();

        // Main-screen switch: refresh sidebar + page, but leave the header
        // untouched. This is the full dashboard body below HeaderHeight.
        const DisplayRegion region = dashboardBodyRegion();
        requestNavigationDisplayRefresh(
            false, 40UL, &region, capturePreview);
    } else if (enteredPageFromSidebar) {
        // Sidebar selection remains visible while browsing page widgets.
        // RIGHT therefore only needs to draw the compact widget focus marker.
        _deferredNavigationRegionValid = false;
        if (dirtyRegion.valid()) {
            requestNavigationDisplayRefresh(
                false, 40UL, &dirtyRegion, capturePreview);
        }
    } else if (
        previousNavigation.area == NavigationArea::Page &&
        currentNavigation.area == NavigationArea::Sidebar &&
        dirtyRegion.valid()) {
        // LEFT back to sidebar: sidebar cursor was never removed, so only
        // erase the old compact widget focus marker.
        _deferredNavigationRegionValid = false;
        requestNavigationDisplayRefresh(
            false, 40UL, &dirtyRegion, capturePreview);
    } else if (subpageChanged) {
        // Pager/subpage switch (FVE, weather locations, ...): only the page
        // changes. Sidebar and header stay physically untouched.
        const DisplayRegion region = pageRegion();
        requestNavigationDisplayRefresh(
            false, 40UL, &region, capturePreview);
    } else if (dirtyRegion.valid()) {
        requestNavigationDisplayRefresh(
            false, 40UL, &dirtyRegion, capturePreview);
    } else {
        // Area transitions can affect both navigation chrome and page focus.
        // Keep them below the header as well.
        const DisplayRegion region = dashboardBodyRegion();
        requestNavigationDisplayRefresh(
            false, 40UL, &region, capturePreview);
    }

    return handled;
}

bool DashboardApp::handleControlAction(
    ControlAction action,
    bool capturePreview) {
    switch (action) {
        case ControlAction::SetLong:
            setDisplayEnabled(!_displayEnabled);
            return true;
        case ControlAction::ResetShort:
            return resetUiToHome(capturePreview);
        case ControlAction::SetShort:
            // Reserved for a future context/settings action.
            return false;
        case ControlAction::None:
            return false;
    }
    return false;
}

void DashboardApp::setDisplayEnabled(bool enabled) {
    if (_displayEnabled == enabled) return;

    _displayEnabled = enabled;
    clearDeferredAutomaticRegions();
    _pendingRefresh = true;
    _pendingFullRefresh = true;
    _pendingRefreshReason = enabled ? "display-soft-on" : "display-soft-off";
    _pendingDisplayRegionValid = false;
    _pendingCapturePreview = false;
    _displayRefreshNotBefore = millis();

    if (!enabled) {
        _pendingBlankDisplay = true;
        Serial.println("[DISPLAY] soft OFF -> full white erase requested");
    } else {
        _pendingBlankDisplay = false;
        _pendingCapturePreview = true;
        Serial.println("[DISPLAY] soft ON -> full redraw requested");
    }
}

bool DashboardApp::resetUiToHome(bool capturePreview) {
    if (!_screenManager.activateScreen("home")) {
        Serial.println("[KEY] RESET: Home screen is not available.");
        return false;
    }

    _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
    clearDeferredAutomaticRegions();
    _navigationController.syncToActiveScreen(false);
    syncWeatherDisplayForActiveScreen(false);
    Serial.println("[KEY] RESET: UI -> Home/sidebar");
    const DisplayRegion region = dashboardBodyRegion();
    requestNavigationDisplayRefresh(false, 40UL, &region, capturePreview);
    return true;
}

void DashboardApp::onScreenSwitchRequested(const String& screenId) {
    clearDeferredAutomaticRegions();
    _navigationController.syncToActiveScreen(false);
    syncWeatherDisplayForActiveScreen(false);
    Serial.printf("[APP][%lu ms] Pozadavek na prepnuti obrazovky: %s\n", millis(), screenId.c_str());
    const DisplayRegion region = dashboardBodyRegion();
    requestNavigationDisplayRefresh(false, 100, &region, true);
}

void DashboardApp::onNavigationSubpageChanged(
    const String& screenId,
    uint8_t subpageIndex) {

    if (screenId == "diagnostics") {
        // The third diagnostics subpage is an exact 800x480 reference image
        // and intentionally replaces all dashboard chrome. Entering or leaving
        // it therefore needs one deliberate full redraw.
        if (subpageIndex == 2 || _dataModel.system.navigationSubpageIndex == 2) {
            requestDisplayRefresh(true, 0, "diagnostics-v2-reference");
        }
        return;
    }

    if (screenId != "weather") return;
    selectWeatherDisplayLocation(subpageIndex, false);
}

void DashboardApp::selectWeatherDisplayLocation(
    uint8_t index,
    bool requestRefresh) {
    const WeatherConfig& weather = _configManager.get().weather;
    const uint8_t count =
        min<uint8_t>(weather.locationCount, MaxWeatherLocations);

    if (!weather.enabled || count == 0) {
        _weatherDisplayLocationId = "";
        _weatherDisplayLocationIndex = 0;
        _dataModel.weather.enabled = false;
        _dataModel.weather.locationId = "";
        _dataModel.weather.locationName = "";
        _dataModel.weather.locationIndex = 0;
        _dataModel.weather.locationCount = 0;
        return;
    }

    if (index >= count) index = 0;

    const WeatherLocation& location = weather.locations[index];
    _weatherDisplayLocationIndex = index;
    _weatherDisplayLocationId = location.id;

    WeatherData selected;
    if (_weatherWorker.copyCached(location.id, selected)) {
        selected.locationId = location.id;
        selected.locationName = location.name;
        selected.locationIndex = index;
        selected.locationCount = count;
        _dataModel.weather = selected;
    } else {
        WeatherData pending;
        pending.enabled = weather.enabled;
        pending.provider = weatherProviderLabel(weather.provider);
        pending.locationId = location.id;
        pending.locationName = location.name;
        pending.locationIndex = index;
        pending.locationCount = count;
        pending.status.available = false;
        pending.status.lastAttemptMs = millis();
        pending.status.lastError = "Nacitam data lokality";
        _dataModel.weather = pending;
        _weatherWorker.requestLocation(location.id);
    }

    if (requestRefresh) {
        const String activeScreenId =
            _screenManager.getActiveScreenId();
        if (isWeatherScreenId(activeScreenId)) {
            requestAutomaticRegionRefresh(pageRegion(), true, "weather-page");
        } else if (activeScreenId == "home") {
            DisplayRegion regions[MaxHomeLayoutWidgets];
            const uint8_t count =
                homeDataRegions(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Weather,
                    regions,
                    MaxHomeLayoutWidgets);
            for (uint8_t i = 0; i < count; ++i) {
                requestAutomaticRegionRefresh(
                    regions[i],
                    true,
                    "weather");
            }
        }
    }
}

void DashboardApp::syncWeatherDisplayForActiveScreen(
    bool requestRefresh) {
    const WeatherConfig& weather = _configManager.get().weather;
    if (!weather.enabled || weather.locationCount == 0) return;

    const String screenId = _screenManager.getActiveScreenId();
    if (isWeatherScreenId(screenId)) {
        int index = weatherLocationIndexById(
            weather,
            _weatherDisplayLocationId);
        if (index < 0) {
            const WeatherLocation* active = weather.activeLocation();
            index =
                active != nullptr
                    ? weatherLocationIndexById(weather, active->id)
                    : 0;
        }
        selectWeatherDisplayLocation(
            index >= 0 ? static_cast<uint8_t>(index) : 0,
            requestRefresh);
        return;
    }

    // Non-weather pages do not need to rebuild the selected WeatherData.
    // Home is the only ordinary page that displays weather values directly.
    // Skipping this on Solar/AZRouter/Pool/Diagnostics keeps the large weather
    // cache copy off loopTask's already shallow navigation call stack.
    if (screenId != "home") return;

    const WeatherLocation* active = weather.activeLocation();
    int index =
        active != nullptr
            ? weatherLocationIndexById(weather, active->id)
            : 0;
    selectWeatherDisplayLocation(
        index >= 0 ? static_cast<uint8_t>(index) : 0,
        requestRefresh);
}

void DashboardApp::refreshWeatherDisplayFromCache(
    bool requestRefresh) {
    const WeatherConfig& weather = _configManager.get().weather;
    if (!weather.enabled || _weatherDisplayLocationId.isEmpty()) return;

    WeatherData cached;
    if (!_weatherWorker.copyCached(
            _weatherDisplayLocationId,
            cached)) {
        return;
    }

    int index = weatherLocationIndexById(
        weather,
        _weatherDisplayLocationId);
    if (index < 0) return;

    cached.locationIndex = static_cast<uint8_t>(index);
    cached.locationCount =
        min<uint8_t>(weather.locationCount, MaxWeatherLocations);

    const bool changed =
        weatherDisplayDataChanged(cached, _dataModel.weather);
    if (!changed) return;

    _dataModel.weather = cached;
    if (requestRefresh) {
        const String activeScreenId =
            _screenManager.getActiveScreenId();
        if (isWeatherScreenId(activeScreenId)) {
            requestAutomaticRegionRefresh(pageRegion(), true, "weather-page");
        } else if (activeScreenId == "home") {
            DisplayRegion regions[MaxHomeLayoutWidgets];
            const uint8_t count =
                homeDataRegions(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Weather,
                    regions,
                    MaxHomeLayoutWidgets);
            for (uint8_t i = 0; i < count; ++i) {
                requestAutomaticRegionRefresh(
                    regions[i],
                    true,
                    "weather");
            }
        }
    }
}

void DashboardApp::onRefreshRequested(bool full) {
    Serial.printf("[APP][%lu ms] Pozadavek na refresh displeje (Full: %d)\n", millis(), full);
    requestDisplayRefresh(
        full,
        100,
        full ? "web-api-full-refresh" : "web-api-partial-refresh");
}

void DashboardApp::loop() {
    Performance::Scope loopTiming(Performance::Loop);
    _wifiManager.loop();
    _timeService.loop();
    if (_webServer != nullptr)
        _webServer->loop();

    const DisplayTaskStatus displayStatus = _displayWorker.getStatus();
    const bool displayElectricallyActive =
        displayStatus.state == DisplayTaskState::Initializing ||
        displayStatus.state == DisplayTaskState::RenderingPartial ||
        displayStatus.state == DisplayTaskState::RenderingFull;
    const bool displayRfSuppressed =
        displayElectricallyActive ||
        displayStatus.state == DisplayTaskState::Queued ||
        displayStatus.pending;

    // E-paper activity can inject false transitions on GPIO18. Keep the rest
    // of the joystick responsive during rendering and suppress only that pin.
    NavigationAction joystickAction;
    const bool joystickEvent =
        _joystick.poll(joystickAction, displayElectricallyActive);
    if (joystickEvent) {
        handleNavigationAction(joystickAction, false);
    }

    // SET/RESET are on GPIO34/35 with external pull-ups and are not affected
    // by the observed GPIO18 display noise, so keep them responsive as well.
    ControlAction controlAction;
    if (_joystick.pollControl(controlAction)) {
        handleControlAction(controlAction, false);
    }

    const bool displayInitDelayElapsed = static_cast<long>(millis() - _displayInitNotBefore) >= 0;
    const bool displayInitFallbackElapsed = static_cast<long>(millis() - _displayInitNotBefore) >= 5000;
    if (!_displayWorkerStarted && displayInitDelayElapsed && (_timeService.isSynced() || displayInitFallbackElapsed)) {
        _displayWorkerStarted = _displayWorker.begin();
        if (_displayWorkerStarted && _lastTelemetryDisplayRefresh == 0) {
            _lastTelemetryDisplayRefresh = millis();
        }
    }

    Cc1101RawReceiver::setSuppressed(displayRfSuppressed);
    Cc1101RawReceiver::loop();
    _rfSensorManager.loop();
    if (displayStatus.lastCompletedMs != 0 && displayStatus.lastCompletedMs != _lastScreenRender) {
        _lastScreenRender = displayStatus.lastCompletedMs;

        // Finish Sidebar -> Page navigation with a second small update.
        // Keeping it deferred avoids joining sidebar and page focus into one
        // large bounding rectangle that would trigger a slow OTP refresh.
        if (_deferredNavigationRegionValid &&
            !displayStatus.pending &&
            displayStatus.state == DisplayTaskState::Idle &&
            !_pendingRefresh) {

            const DisplayRegion deferred = _deferredNavigationRegion;
            const bool capturePreview = _deferredNavigationCapturePreview;
            _deferredNavigationRegionValid = false;

            requestNavigationDisplayRefresh(
                false, 0UL, &deferred, capturePreview);
        }

        if (!_deferredNavigationRegionValid &&
            !_pendingRefresh &&
            !displayStatus.pending &&
            displayStatus.state == DisplayTaskState::Idle) {

            DisplayRegion deferredAutomatic;
            bool capturePreview = true;
            String reason;
            if (popDeferredAutomaticRegion(
                    deferredAutomatic,
                    capturePreview,
                    reason)) {
                requestNavigationDisplayRefresh(
                    false, 0UL,
                    &deferredAutomatic,
                    capturePreview,
                    reason.isEmpty() ? "automatic-region" : reason.c_str());
            }
        }
    }

    const String previousTimeStr = _dataModel.system.timeStr;
    const String previousDateStr = _dataModel.system.dateStr;
    const String previousDayOfWeekStr = _dataModel.system.dayOfWeekStr;

    const bool wasWifiConnected = _dataModel.system.wifiConnected;
    const bool previousAccessPoint = _dataModel.system.wifiAccessPoint;
    _dataModel.system.wifiAccessPoint = _wifiManager.isConfigAccessPoint();
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    const uint8_t previousSignalLevel = _dataModel.system.wifiSignalLevel;
    _dataModel.system.wifiSignalLevel = _wifiSignalLevel.update(_dataModel.system.wifiConnected, _dataModel.system.wifiRssi, millis());
    if (previousSignalLevel != _dataModel.system.wifiSignalLevel ||
        previousAccessPoint != _dataModel.system.wifiAccessPoint) {
        const DisplayRegion region = headerRegion();
        requestAutomaticRegionRefresh(region, true, "wifi");
    }
    _dataModel.system.ipAddress = _wifiManager.getIpAddress();
    _dataModel.system.ntpSynced = _timeService.isSynced();
    _dataModel.system.timeStr = _timeService.getTimeStr();
    _dataModel.system.dateStr = _timeService.getDateStr();
    _dataModel.system.dayOfWeekStr = _timeService.getDayOfWeekStr();

    if (wasWifiConnected && !_dataModel.system.wifiConnected) {
        _dataModel.solar.status.recordError("WiFi unavailable");
        _dataModel.azrouter.status.recordError("WiFi unavailable");
        _dataModel.weather.status.recordError("WiFi unavailable");
        _dataModel.updateSystemMetrics();

        const String activeScreenId =
            _screenManager.getActiveScreenId();
        if (activeScreenId == "home") {
            DisplayRegion energyRegions[ScreenLayout::MaxWidgets];
            const uint8_t energyRegionCount =
                homeDataRegions(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Energy,
                    energyRegions,
                    ScreenLayout::MaxWidgets);
            for (uint8_t i = 0; i < energyRegionCount; ++i) {
                requestAutomaticRegionRefresh(
                    energyRegions[i], true, "energy");
            }

            const DisplayRegion weatherRegion =
                homeDataRegion(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Weather);
            if (weatherRegion.valid()) {
                requestAutomaticRegionRefresh(weatherRegion, true, "weather");
            }
        } else if (activeScreenId == "solar" ||
                   activeScreenId == "azrouter") {
            requestAutomaticRegionRefresh(pageRegion(), true, "energy-page");
        } else if (isWeatherScreenId(activeScreenId)) {
            requestAutomaticRegionRefresh(pageRegion(), true, "weather-page");
        }
    }

    const bool timeChanged =
        previousTimeStr != _dataModel.system.timeStr ||
        previousDateStr != _dataModel.system.dateStr ||
        previousDayOfWeekStr != _dataModel.system.dayOfWeekStr;
    if (timeChanged) {
        const DisplayRegion region = headerRegion();
        requestAutomaticRegionRefresh(region, true, "clock");
    }

    WeatherData weatherUpdate;
    if (_weatherWorker.takeLatest(weatherUpdate)) {
        const WeatherConfig& weather = _configManager.get().weather;
        const String activeScreenId = _screenManager.getActiveScreenId();

        // Home and non-weather screens always consume the configured active
        // location. Weather pager may temporarily display another cached
        // location without changing persistent configuration.
        const bool shouldApply =
            !isWeatherScreenId(activeScreenId) ||
            weatherUpdate.locationId == _weatherDisplayLocationId;

        if (shouldApply) {
            int index = weatherLocationIndexById(
                weather,
                weatherUpdate.locationId);
            if (index < 0) index = 0;

            weatherUpdate.locationIndex =
                static_cast<uint8_t>(index);
            weatherUpdate.locationCount =
                min<uint8_t>(weather.locationCount, MaxWeatherLocations);

            const bool changed =
                weatherDisplayDataChanged(
                    weatherUpdate,
                    _dataModel.weather);
            _dataModel.weather = weatherUpdate;
            if (changed) {
                if (isWeatherScreenId(activeScreenId)) {
                    const DisplayRegion region = pageRegion();
                    requestAutomaticRegionRefresh(region, true);
                } else if (activeScreenId == "home") {
                    const DisplayRegion region =
                        homeDataRegion(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Weather);
            if (region.valid()) {
                requestAutomaticRegionRefresh(region, true, "weather");
            }
                }
            }
        }
    }

    const unsigned long weatherCacheNow = millis();
    if (isWeatherScreenId(_screenManager.getActiveScreenId()) &&
        weatherCacheNow - _lastWeatherDisplayCacheCheck >= 1000UL) {
        _lastWeatherDisplayCacheCheck = weatherCacheNow;
        refreshWeatherDisplayFromCache(true);
    }

    if (displayStatus.ready &&
        _pendingRefresh &&
        static_cast<long>(millis() - _displayRefreshNotBefore) >= 0) {

        const DisplayRegion* region =
            _pendingDisplayRegionValid ? &_pendingDisplayRegion : nullptr;

        IScreen* screenToRender =
            _pendingBlankDisplay
                ? static_cast<IScreen*>(&_blankDisplayScreen)
                : _screenManager.getActiveScreen();

        // Isolate the shared SPI/display timing before the worker can start
        // rendering. Waiting for the next loop iteration would leave a short
        // race where CC1101 GDO0 interrupts are still attached during the
        // first bytes of the e-paper transfer.
        Cc1101RawReceiver::setSuppressed(true);

        Serial.printf(
            "[REFRESH][ENQUEUE][%lu ms] type=%s scope=%s reason=%s screen=%s",
            millis(),
            _pendingFullRefresh ? "FULL" : "PARTIAL",
            region != nullptr ? "REGION" : "WHOLE",
            _pendingRefreshReason.isEmpty() ? "unspecified" : _pendingRefreshReason.c_str(),
            screenToRender != nullptr ? screenToRender->getId().c_str() : "null");
        if (region != nullptr) {
            Serial.printf(
                " region=%d,%d %dx%d",
                region->x, region->y, region->width, region->height);
        }
        Serial.println();

        if (_displayWorker.enqueue(
                screenToRender,
                _dataModel,
                _pendingFullRefresh,
                region,
                _pendingCapturePreview)) {
            _pendingRefresh = false;
            _pendingFullRefresh = false;
            _pendingRefreshReason = "";
            _pendingBlankDisplay = false;
            _pendingDisplayRegionValid = false;
            _pendingCapturePreview = true;
            _displayRefreshNotBefore = 0;
        }
    }

    unsigned long now = millis();

    if (now - _lastBme280Sync >= Bme280PollIntervalMs) {
        const bool wasAvailable = _dataModel.inside.status.available;
        const float previousTemperature = _dataModel.inside.temperatureC;
        const int previousHumidity = _dataModel.inside.humidityPercent;
        const float previousPressure = _dataModel.inside.pressureHpa;

        const bool success = _bme280Sensor.update(_dataModel.inside);
        _lastBme280Sync = millis();

        if (success) {
            sampleInsideHistory(_dataModel.inside, _insideHistory);
        }

        const bool availabilityChanged =
            wasAvailable != _dataModel.inside.status.available;
        const bool valuesChanged =
            success &&
            (fabsf(previousTemperature - _dataModel.inside.temperatureC) >= 0.2f ||
             abs(previousHumidity - _dataModel.inside.humidityPercent) >= 1 ||
             fabsf(previousPressure - _dataModel.inside.pressureHpa) >= 1.0f);

        const unsigned long refreshNow = millis();
        const bool refreshIntervalElapsed =
            _lastBme280DisplayRefresh == 0 ||
            refreshNow - _lastBme280DisplayRefresh >= Bme280DisplayRefreshIntervalMs;

        if (availabilityChanged || (valuesChanged && refreshIntervalElapsed)) {
            _lastBme280DisplayRefresh = refreshNow;

            if (_screenManager.getActiveScreenId() == "home") {
                const DisplayRegion region =
                    homeDataRegion(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Indoor);
                if (region.valid()) {
                    requestAutomaticRegionRefresh(region, true, "bme280");
                }
            }
        }
    }

    now = millis();
    if (now - _lastBatterySync >= BatteryPollIntervalMs) {
        const bool wasAvailable = _dataModel.battery.status.available;
        const float previousVoltage = _dataModel.battery.voltageV;
        const float previousSoc = _dataModel.battery.socPercent;
        const float previousRate =
            _dataModel.battery.changeRatePercentPerHour;
        const uint8_t previousAlertFlags =
            _dataModel.battery.alertFlags;

        const bool success =
            _max17048Sensor.update(_dataModel.battery);
        _lastBatterySync = millis();

        const bool availabilityChanged =
            wasAvailable != _dataModel.battery.status.available;
        const bool alertChanged =
            previousAlertFlags != _dataModel.battery.alertFlags;
        const bool socChanged =
            success &&
            fabsf(previousSoc - _dataModel.battery.socPercent) >= 1.0f;
        const bool valuesChanged =
            success &&
            (fabsf(previousVoltage - _dataModel.battery.voltageV) >= 0.02f ||
             socChanged ||
             fabsf(previousRate -
                   _dataModel.battery.changeRatePercentPerHour) >= 1.0f);

        const unsigned long refreshNow = millis();
        const bool refreshIntervalElapsed =
            _lastBatteryDisplayRefresh == 0 ||
            refreshNow - _lastBatteryDisplayRefresh >=
                BatteryDisplayRefreshIntervalMs;

        if (availabilityChanged ||
            alertChanged ||
            (valuesChanged && refreshIntervalElapsed)) {

            _lastBatteryDisplayRefresh = refreshNow;

            // Device battery is always visible in the header when MAX17048 is
            // present. Voltage/rate-only changes do not need to redraw it.
            if (availabilityChanged ||
                alertChanged ||
                (socChanged && refreshIntervalElapsed)) {
                requestAutomaticRegionRefresh(headerRegion(), true, "battery-header");
            }

            if (_screenManager.getActiveScreenId() == "home") {
                const DisplayRegion region =
                    homeDataRegion(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Battery);
                if (region.valid()) {
                    requestAutomaticRegionRefresh(region, true, "battery");
                }
            }
        }
    }

    now = millis();
    if (_wifiManager.isConnected()) {
        const auto& cfg = _configManager.get();
        bool goodweAvailabilityChanged = false;
        bool azrouterAvailabilityChanged = false;
        bool goodweTelemetryUpdated = false;
        bool azrouterTelemetryUpdated = false;
        SolarData goodweResult;
        bool goodweResultSuccess = false;
        uint32_t goodweCompletedMs = 0;
        if (_goodweWorker.takeLatest(
                goodweResult,
                goodweResultSuccess,
                goodweCompletedMs)) {
            const bool wasAvailable =
                _dataModel.solar.status.available;

            applyGoodWeWorkerResult(
                _dataModel.solar,
                goodweResult,
                goodweResultSuccess);

            _lastGoodweSync = goodweCompletedMs;
            _goodweFailureStreak =
                goodweResultSuccess
                    ? 0
                    : nextFailureStreak(_goodweFailureStreak);
            goodweTelemetryUpdated =
                goodweTelemetryUpdated || goodweResultSuccess;
            goodweAvailabilityChanged =
                goodweAvailabilityChanged ||
                wasAvailable != _dataModel.solar.status.available;

            if (!goodweResultSuccess) {
                Serial.printf(
                    "[GOODWE-WORKER] Dalsi pokus za %lu ms "
                    "(chyby v rade: %u)\n",
                    pollDelayMs(
                        cfg.goodwe.pollIntervalSeconds,
                        _goodweFailureStreak),
                    _goodweFailureStreak);
            }
        }

        const uint32_t goodweDelayMs =
            pollDelayMs(
                cfg.goodwe.pollIntervalSeconds,
                _goodweFailureStreak);
        if (cfg.goodwe.enabled &&
            now - _lastGoodweSync >= goodweDelayMs) {
            _goodweWorker.requestPoll();
        }

        now = millis();
        const uint32_t azrouterDelayMs = pollDelayMs(cfg.azrouter.pollIntervalSeconds, _azrouterFailureStreak);
        if (cfg.azrouter.enabled && now - _lastAzrouterSync >= azrouterDelayMs) {
            const bool gateTaken =
                _networkClientGate == nullptr ||
                xSemaphoreTake(_networkClientGate, 0) == pdTRUE;

            if (gateTaken) {
                const bool wasAvailable = _dataModel.azrouter.status.available;
                const bool success = _azrouterClient.update(_dataModel.azrouter);
                if (_networkClientGate != nullptr) {
                    xSemaphoreGive(_networkClientGate);
                }

                _lastAzrouterSync = millis();
                _azrouterFailureStreak =
                    success ? 0 : nextFailureStreak(_azrouterFailureStreak);
                azrouterTelemetryUpdated = azrouterTelemetryUpdated || success;
                azrouterAvailabilityChanged =
                    azrouterAvailabilityChanged ||
                    wasAvailable != _dataModel.azrouter.status.available;
                if (!success) {
                    Serial.printf(
                        "[AZROUTER] Dalsi pokus za %lu ms (chyby v rade: %u)\n",
                        pollDelayMs(
                            cfg.azrouter.pollIntervalSeconds,
                            _azrouterFailureStreak),
                        _azrouterFailureStreak);
                }

                // Service WebUI again after the AZRouter burst.
                if (_webServer != nullptr) {
                    _webServer->loop();
                }
            }
            // If Weather/Web owns the shared gate, leave _lastAzrouterSync
            // untouched. The poll is retried as soon as the gate is free.
        }

        _dataModel.updateSystemMetrics();

        const String activeScreenId = _screenManager.getActiveScreenId();

        // Availability belongs to a specific source. A GoodWe failure must
        // not force an AZRouter page redraw (and vice versa).
        if (activeScreenId == "solar" && goodweAvailabilityChanged) {
            const DisplayRegion region = pageRegion();
            requestAutomaticRegionRefresh(
                region, true, "solar-availability");
        } else if (activeScreenId == "azrouter" &&
                   azrouterAvailabilityChanged) {
            const DisplayRegion region = pageRegion();
            requestAutomaticRegionRefresh(
                region, true, "azrouter-availability");
        } else if (activeScreenId == "home" &&
                   (goodweAvailabilityChanged ||
                    azrouterAvailabilityChanged)) {
            DisplayRegion energyRegions[ScreenLayout::MaxWidgets];
            const uint8_t energyRegionCount =
                homeDataRegions(
                    _configManager.get().display.homeLayout,
                    _dataModel,
                    HomeDataGroup::Energy,
                    energyRegions,
                    ScreenLayout::MaxWidgets);
            for (uint8_t i = 0; i < energyRegionCount; ++i) {
                requestAutomaticRegionRefresh(
                    energyRegions[i], true, "energy");
            }
        }

        // Do not redraw the whole panel merely because one minute elapsed.
        // Live GoodWe/AZ values are refreshed only on screens that actually
        // show them, and only in the page/content area. The header clock has
        // its own small partial refresh above.
        const unsigned long telemetryNow = millis();
        const bool activeTelemetryUpdated =
            (activeScreenId == "home" &&
             (goodweTelemetryUpdated || azrouterTelemetryUpdated)) ||
            (activeScreenId == "solar" && goodweTelemetryUpdated) ||
            (activeScreenId == "azrouter" && azrouterTelemetryUpdated);

        if (activeTelemetryUpdated &&
            telemetryNow - _lastTelemetryDisplayRefresh >= 60000UL) {

            _lastTelemetryDisplayRefresh = telemetryNow;

            if (activeScreenId == "solar") {
                requestAutomaticRegionRefresh(
                    solarLiveDataRegion(), true, "solar-live");

                // Solar history is bucketed into 15-minute slots. Updating the
                // large graph every minute made the e-paper behave like a
                // near-full page refresh. Keep live KPIs at 1 minute, but redraw
                // the graph only once per history interval.
                constexpr unsigned long SolarHistoryDisplayIntervalMs =
                    static_cast<unsigned long>(SolarHistoryIntervalMinutes) *
                    60UL * 1000UL;
                if (_lastSolarHistoryDisplayRefresh == 0) {
                    _lastSolarHistoryDisplayRefresh = telemetryNow;
                } else if (telemetryNow - _lastSolarHistoryDisplayRefresh >=
                           SolarHistoryDisplayIntervalMs) {
                    _lastSolarHistoryDisplayRefresh = telemetryNow;
                    requestAutomaticRegionRefresh(
                        solarHistoryDataRegion(), true, "solar-history");
                }
            } else if (activeScreenId == "azrouter") {
                // Keep the frequent update below the panel's slow large-region
                // threshold. The master card carries the most useful live
                // values and can therefore stay on a one-minute cadence.
                requestAutomaticRegionRefresh(
                    azRouterMasterDataRegion(), true, "azrouter-master");

                constexpr unsigned long AzRouterPhaseDisplayIntervalMs =
                    5UL * 60UL * 1000UL;
                if (_lastAzRouterPhaseDisplayRefresh == 0) {
                    _lastAzRouterPhaseDisplayRefresh = telemetryNow;
                } else if (telemetryNow - _lastAzRouterPhaseDisplayRefresh >=
                           AzRouterPhaseDisplayIntervalMs) {
                    _lastAzRouterPhaseDisplayRefresh = telemetryNow;
                    requestAutomaticRegionRefresh(
                        azRouterPhaseDataRegion(), true, "azrouter-phases");
                }

                constexpr unsigned long AzRouterEnergyDisplayIntervalMs =
                    15UL * 60UL * 1000UL;
                if (_lastAzRouterEnergyDisplayRefresh == 0) {
                    _lastAzRouterEnergyDisplayRefresh = telemetryNow;
                } else if (telemetryNow - _lastAzRouterEnergyDisplayRefresh >=
                           AzRouterEnergyDisplayIntervalMs) {
                    _lastAzRouterEnergyDisplayRefresh = telemetryNow;
                    requestAutomaticRegionRefresh(
                        azRouterEnergyDataRegion(), true, "azrouter-energy");
                }
            } else {
                DisplayRegion energyRegions[ScreenLayout::MaxWidgets];
                const uint8_t energyRegionCount =
                    homeDataRegions(
                        _configManager.get().display.homeLayout,
                        _dataModel,
                        HomeDataGroup::Energy,
                        energyRegions,
                        ScreenLayout::MaxWidgets);
                for (uint8_t i = 0; i < energyRegionCount; ++i) {
                    requestAutomaticRegionRefresh(
                        energyRegions[i], true, "energy");
                }
            }
        }
    }

    delay(20);
    Performance::report();
}
