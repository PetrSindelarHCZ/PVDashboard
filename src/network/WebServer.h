#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <functional>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../display/DisplayTaskStatus.h"
#include "../screens/ScreenManager.h"
#include "../update/OtaManager.h"

class DashboardWebServer {
public:
    using ScreenChangeCallback = std::function<void(const String& screenId)>;
    using RefreshCallback = std::function<void(bool full)>;
    using DisplayStatusCallback = std::function<DisplayTaskStatus()>;
    using SystemConfigCallback = std::function<void(const SystemConfig& system)>;
    using WifiConfigCallback = std::function<void(const String& ssid, const String& password)>;
    using WifiScanCallback = std::function<String()>;
    using SourceConfigCallback = std::function<void(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter)>;
    using WeatherConfigCallback = std::function<void(const WeatherConfig& weather)>;
    using FactoryResetCallback = std::function<bool()>;
    using ConfigImportCallback = std::function<bool(const AppConfig& config)>;

    DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager, const AppConfig& config);

    void begin();
    void enableTimezoneUiExtension();
    void loop();
    void onScreenChange(ScreenChangeCallback callback);
    void onRefresh(RefreshCallback callback);
    void onDisplayStatus(DisplayStatusCallback callback);
    void onSystemConfig(SystemConfigCallback callback);
    void onWifiConfig(WifiConfigCallback callback);
    void onWifiScan(WifiScanCallback callback);
    void onSourceConfig(SourceConfigCallback callback);
    void onWeatherConfig(WeatherConfigCallback callback);
    void onFactoryReset(FactoryResetCallback callback);
    void onConfigImport(ConfigImportCallback callback);

private:
    WebServer _server;
    DataModel& _dataModel;
    ScreenManager& _screenManager;
    const AppConfig& _config;
    ScreenChangeCallback _screenCallback;
    RefreshCallback _refreshCallback;
    DisplayStatusCallback _displayStatusCallback;
    SystemConfigCallback _systemConfigCallback;
    WifiConfigCallback _wifiConfigCallback;
    WifiScanCallback _wifiScanCallback;
    SourceConfigCallback _sourceConfigCallback;
    WeatherConfigCallback _weatherConfigCallback;
    FactoryResetCallback _factoryResetCallback;
    ConfigImportCallback _configImportCallback;
    OtaManager _otaManager;
    String _githubUpdateVersion;
    String _githubUpdateUrl;
    String _githubUpdateSha256;
    bool _timezoneUiEnabled = false;

    void setupRoutes();
    void handleRoot();
    void handleExtendedRoot();
    void handleApiTimezoneConfig();
    void handleApiSystemConfigV2();
    void loopLegacy();
    void handleApiStatus();
    void handleApiScreens();
    void handleApiActivateScreen(const String& screenId);
    void handleApiRefresh(bool full);
    void handleApiRestart();
    void handleApiFactoryReset();
    void handleApiConfigExport();
    void handleApiConfigImport();
    void handleApiSystemConfig();
    void handleApiWifiConfig();
    void handleApiWifiScan();
    void handleApiSourceConfig();
    void handleApiWeatherConfig();
    void handleApiCheckForUpdate();
    void handleApiGithubUpdate();
    void handleApiUpdateUpload();
    void handleApiUpdateComplete();
};
