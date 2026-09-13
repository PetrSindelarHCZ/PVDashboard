#pragma once
#include <Arduino.h>
#include "../config/ConfigManager.h"
#include "../data/DataModel.h"
#include "../display/EpaperDisplay.h"
#include "../display/DisplayManager.h"
#include "../screens/ScreenManager.h"
#include "../screens/HomeScreen.h"
#include "../screens/SolarScreen.h"
#include "../screens/PoolScreen.h"
#include "../screens/WeatherScreen.h"
#include "../screens/DiagnosticsScreen.h"
#include "../network/WifiManager.h"
#include "../network/TimeService.h"
#include "../network/WebServer.h"
#include "../integrations/goodwe/GoodWeClient.h"
#include "../integrations/azrouter/AZRouterClient.h"

class DashboardApp {
public:
    DashboardApp();

    void setup();
    void loop();

private:
    ConfigManager _configManager;
    DataModel _dataModel;

    EpaperDisplay _epaperDisplay;
    DisplayManager _displayManager;
    ScreenManager _screenManager;

    HomeScreen _homeScreen;
    SolarScreen _solarScreen;
    PoolScreen _poolScreen;
    WeatherScreen _weatherScreen;
    DiagnosticsScreen _diagnosticsScreen;

    WifiManager _wifiManager;
    TimeService _timeService;
    DashboardWebServer _webServer;

    GoodWeClient _goodweClient;
    AZRouterClient _azrouterClient;

    bool _pendingRefresh = false;
    bool _pendingFullRefresh = false;
    unsigned long _lastGoodweSync = 0;
    unsigned long _lastAzrouterSync = 0;
    unsigned long _lastScreenRender = 0;
    unsigned long _lastDisplayUpdate = 0;

    void registerScreens();
    void onScreenSwitchRequested(const String& screenId);
    void onRefreshRequested(bool full);
};
