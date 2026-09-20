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
#include "../navigation/NavigationController.h"
#include "../input/FiveWayJoystick.h"
#include "../network/WifiManager.h"
#include "../network/WifiSignalLevel.h"
#include "../network/TimeService.h"
#include "../network/WebServer.h"
#include "../integrations/goodwe/GoodWeClient.h"
#include "../integrations/azrouter/AZRouterClient.h"
#include "../integrations/weather/WeatherWorker.h"
#include "../integrations/bme280/Bme280Sensor.h"

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
    NavigationController _navigationController;
    FiveWayJoystick _joystick;

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
    Bme280Sensor _bme280Sensor;

    // Serializuje pametove narocne operace: e-paper render/preview a weather TLS.
    // ESP32-WROOM bez PSRAM nema dost velky souvisly DRAM blok pro obe soucasne.
    SemaphoreHandle_t _memoryHeavyGate = nullptr;
    // Serializes local WebServer socket servicing against outbound weather TLS
    // so both do not compete for scarce internal Wi-Fi/TLS buffers.
    SemaphoreHandle_t _networkClientGate = nullptr;

    bool _pendingRefresh = false;
    bool _pendingFullRefresh = false;
    bool _displayWorkerStarted = false;
    bool _handlingPhysicalNavigation = false;
    bool _physicalNavigationChanged = false;
    bool _physicalNavigationFullRefresh = false;
    bool _pendingDisplayRegionValid = false;
    DisplayRegion _pendingDisplayRegion;
    bool _pendingCapturePreview = true;
    bool _pendingWifiSave = false;
    String _pendingWifiSsid;
    String _pendingWifiPassword;
    unsigned long _displayInitNotBefore = 0;
    unsigned long _displayRefreshNotBefore = 0;
    unsigned long _lastGoodweSync = 0;
    unsigned long _lastAzrouterSync = 0;
    unsigned long _lastBme280Sync = 0;
    unsigned long _lastBme280DisplayRefresh = 0;
    uint8_t _goodweFailureStreak = 0;
    uint8_t _azrouterFailureStreak = 0;
    unsigned long _lastScreenRender = 0;
    unsigned long _lastDisplayUpdate = 0;
    unsigned long _lastWeatherDisplayCacheCheck = 0;
    String _weatherDisplayLocationId;
    uint8_t _weatherDisplayLocationIndex = 0;

    void registerScreens();
    void setSolarScreenEnabled(bool enabled);
    void setPoolScreenEnabled(bool enabled);
    void setWeatherScreensEnabled(bool enabled);
    void requestDisplayRefresh(bool full, unsigned long delayMs = 0);
    void requestNavigationDisplayRefresh(bool full, unsigned long delayMs,
                                         const DisplayRegion* region = nullptr,
                                         bool capturePreview = true);
    void requestAutomaticDisplayRefresh();
    void onScreenSwitchRequested(const String& screenId);
    void onRefreshRequested(bool full);
    void onNavigationSubpageChanged(const String& screenId, uint8_t subpageIndex);
    void selectWeatherDisplayLocation(uint8_t index, bool requestRefresh);
    void syncWeatherDisplayForActiveScreen(bool requestRefresh);
    void refreshWeatherDisplayFromCache(bool requestRefresh);
};
