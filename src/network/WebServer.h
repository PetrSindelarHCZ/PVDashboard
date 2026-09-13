#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <functional>
#include "../config/ConfigSchema.h"
#include "../data/DataModel.h"
#include "../screens/ScreenManager.h"
#include "../update/OtaManager.h"

class DashboardWebServer {
public:
    using ScreenChangeCallback = std::function<void(const String& screenId)>;
    using RefreshCallback = std::function<void(bool full)>;
    using WifiConfigCallback = std::function<void(const String& ssid, const String& password)>;
    using WifiScanCallback = std::function<String()>;
    using SourceConfigCallback = std::function<void(const GoodWeConfig& goodwe, const AZRouterConfig& azrouter)>;

    DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager, const AppConfig& config);

    void begin();
    void loop();
    void onScreenChange(ScreenChangeCallback callback);
    void onRefresh(RefreshCallback callback);
    void onWifiConfig(WifiConfigCallback callback);
    void onWifiScan(WifiScanCallback callback);
    void onSourceConfig(SourceConfigCallback callback);

private:
    WebServer _server;
    DataModel& _dataModel;
    ScreenManager& _screenManager;
    const AppConfig& _config;
    ScreenChangeCallback _screenCallback;
    RefreshCallback _refreshCallback;
    WifiConfigCallback _wifiConfigCallback;
    WifiScanCallback _wifiScanCallback;
    SourceConfigCallback _sourceConfigCallback;
    OtaManager _otaManager;

    void setupRoutes();
    void handleRoot();
    void handleApiStatus();
    void handleApiScreens();
    void handleApiActivateScreen(const String& screenId);
    void handleApiRefresh(bool full);
    void handleApiRestart();
    void handleApiWifiConfig();
    void handleApiWifiScan();
    void handleApiSourceConfig();
    void handleApiCheckForUpdate();
    void handleApiGithubUpdate();
    void handleApiUpdateUpload();
    void handleApiUpdateComplete();
};
