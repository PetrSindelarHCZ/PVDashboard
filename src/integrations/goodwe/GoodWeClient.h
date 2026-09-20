#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>
#include "../../data/DataModel.h"

// GoodWe ET series (GW10K-ET) UDP Protocol client
class GoodWeClient {
public:
    GoodWeClient();

    void begin(const String& host, uint16_t port);
    bool update(SolarData& solarData);

private:
    String _host;
    uint16_t _port = 8899;
    IPAddress _remoteIp;
    bool _remoteIpKnown = false;

    static uint16_t calculateCrc(const uint8_t* buffer, size_t length);
    static int16_t readInt16(const uint8_t* buffer, size_t offset);
    static uint16_t readUInt16(const uint8_t* buffer, size_t offset);
    static int32_t readInt32(const uint8_t* buffer, size_t offset);
    static uint32_t readUInt32(const uint8_t* buffer, size_t offset);
    bool readHoldingRegisters(uint16_t startRegister, uint16_t count, uint8_t* dataOut, size_t dataCapacity, size_t& dataLength);
};
