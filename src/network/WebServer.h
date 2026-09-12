#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <functional>
#include "../data/DataModel.h"
#include "../screens/ScreenManager.h"

class DashboardWebServer {
public:
    using ScreenChangeCallback = std::function<void(const String& screenId)>;
    using RefreshCallback = std::function<void(bool full)>;

    DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager);

    void begin();
    void loop();
    void onScreenChange(ScreenChangeCallback callback);
    void onRefresh(RefreshCallback callback);

private:
    WebServer _server;
    DataModel& _dataModel;
    ScreenManager& _screenManager;
    ScreenChangeCallback _screenCallback;
    RefreshCallback _refreshCallback;

    void setupRoutes();
    void handleRoot();
    void handleApiStatus();
    void handleApiScreens();
    void handleApiActivateScreen(const String& screenId);
    void handleApiRefresh(bool full);
    void handleApiRestart();
};
