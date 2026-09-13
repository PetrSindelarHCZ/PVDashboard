#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../../data/DataModel.h"

class AZRouterClient {
public:
    AZRouterClient();

    void begin(const String& host, uint16_t port);
    bool update(AZRouterData& azData);

private:
    String _host;
    uint16_t _port = 80;
    HTTPClient _http;
};
