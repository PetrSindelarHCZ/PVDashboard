#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../../data/DataModel.h"
#include "../../diagnostics/Performance.h"

class AZRouterClient {
public:
    AZRouterClient();

    void begin(const String& host, uint16_t port,
               const String& username = "",
               const String& password = "");
    bool update(AZRouterData& azData);

private:
    String _host;
    uint16_t _port = 80;
    String _username;
    String _password;
    String _bearerToken;
    String _sessionCookie;
    bool _loginCompleted = false;

    bool credentialsConfigured() const;
    void clearSession();
    void addAuthHeaders(HTTPClient& http);
    bool login(String& errorMessage);
    bool getJson(const char* path, JsonDocument& doc,
                 Performance::Metric metric,
                 String& errorMessage,
                 bool allowRelogin = true);
    static String firstCookiePair(const String& setCookie);
};
