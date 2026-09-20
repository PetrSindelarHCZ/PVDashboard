#pragma once

#include <Arduino.h>
#include "../../config/ConfigSchema.h"
#include "../../data/DataModel.h"
#include "RfSensorTypes.h"

class RfSensorManager {
public:
    explicit RfSensorManager(DataModel& dataModel);

    void applyConfig(const RfSensorsConfig& config);
    bool observe(const RfSensorObservation& observation);
    void loop();

    void startScan(uint32_t durationMs = 30000);
    bool isScanning() const;
    uint32_t scanRemainingMs() const;
    String statusJson() const;

    bool addDiscoveredSensor(const String& bindingKey, const String& requestedName,
                             RfSensorsConfig& updated, String& error) const;
    bool renameSensor(const String& slotId, const String& requestedName,
                      RfSensorsConfig& updated, String& error) const;
    bool removeSensor(const String& slotId,
                      RfSensorsConfig& updated, String& error) const;

private:
    struct DiscoveredSensor {
        bool used = false;
        RfSensorObservation observation;
        uint32_t firstSeenMs = 0;
        uint32_t lastSeenMs = 0;
        uint32_t packetCount = 0;
    };

    DataModel& _dataModel;
    RfSensorsConfig _config;
    DiscoveredSensor _discovered[MaxRfDiscoveredSensors];
    uint32_t _scanStartedMs = 0;
    uint32_t _scanDurationMs = 0;

    static String normalizedName(String name);
    static String protocolLabel(const String& protocol);
    static String defaultSensorLabel(const RfSensorConfig& sensor);
    static bool sameBinding(const RfSensorConfig& sensor, const RfSensorObservation& observation);

    int configuredIndexFor(const RfSensorObservation& observation) const;
    int discoveredIndexFor(const String& bindingKey) const;
    int freeDiscoveredIndex() const;
    String nextSlotId(const RfSensorsConfig& config) const;
};
