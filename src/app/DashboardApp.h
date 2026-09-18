#pragma once
#include <Arduino.h>
#include "../config/ConfigManager.h"
#include "../data/DataModel.h"
#include "../display/EpaperDisplay.h"
#include "../display/DisplayPreview.h"
#include "../display/DisplayManager.h"
#include "../display/DisplayWorker.h"
#include "../screens/ScreenManager.h"
#include "../screens/HomeScreen.h"
#include "../screens/SolarScreen.h"
#include "../screens/PoolScreen.h"
#include "../screens/WeatherScreen.h"
#include "../screens/DiagnosticsScreen.h"
#include "../network/WifiManager.h"
#include "../network/WifiSignalLevel.h"
#include "../network/TimeService.h"
#include "../network/WebServer.h"
#include "../integrations/goodwe/GoodWeClient.h"
#include "../integrations/azrouter/AZRouterClient.h"
#include "../integrations/weather/WeatherWorker.h"

class DashboardApp {
public:
    DashboardApp();

    void setup();
    void loop();

private:
    ConfigManager _configManager;
    DataModel _dataModel;

    EpaperDisplay _epaperDisplay;
    DisplayPreview _displayPreview;
    DisplayManager _displayManager;
    DisplayWorker _displayWorker;
    ScreenManager _screenManager;

    HomeScreen _homeScreen;
    SolarScreen _solarScreen;
    PoolScreen _poolScreen;
    WeatherScreen _weatherScreen;
    WeatherScreen _weatherHourlyScreens[WeatherForecastDayCount] = {
        WeatherScreen(0), WeatherScreen(1), WeatherScreen(2), WeatherScreen(3)
    };
    DiagnosticsScreen _diagnosticsScreen;

    WifiManager _wifiManager;
    WifiSignalLevel _wifiSignalLevel;
    TimeService _timeService;
    DashboardWebServer _webServer;

    GoodWeClient _goodweClient;
    AZRouterClient _azrouterClient;
    WeatherWorker _weatherWorker;

    bool _pendingRefresh = false;
    bool _pendingFullRefresh = false;
    bool _displayWorkerStarted = false;
    bool _pendingWifiSave = false;
    String _pendingWifiSsid;
    String _pendingWifiPassword;
    unsigned long _displayInitNotBefore = 0;
    unsigned long _displayRefreshNotBefore = 0;
    unsigned long _lastGoodweSync = 0;
    unsigned long _lastAzrouterSync = 0;
    uint8_t _goodweFailureStreak = 0;
    uint8_t _azrouterFailureStreak = 0;
    unsigned long _lastScreenRender = 0;
    unsigned long _lastDisplayUpdate = 0;

    void registerScreens();
    void setWeatherScreensEnabled(bool enabled);
    void requestDisplayRefresh(bool full, unsigned long delayMs = 0);
    void requestAutomaticDisplayRefresh();
    void onScreenSwitchRequested(const String& screenId);
    void onRefreshRequested(bool full);
};
