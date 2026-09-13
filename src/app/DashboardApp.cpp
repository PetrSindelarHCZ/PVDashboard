#include "DashboardApp.h"
#include "../../include/AppConfig.h"
#include "../../include/Version.h"

DashboardApp::DashboardApp()
    : _epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCK, EPD_MISO, EPD_MOSI),
      _displayManager(_epaperDisplay),
    _webServer(80, _dataModel, _screenManager, _configManager.get()) {
}

void DashboardApp::setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==========================================");
    Serial.printf("  %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);
    Serial.printf("  Build: %s\n", FIRMWARE_BUILD_DATE);
    Serial.println("==========================================");

    // 1. Načíst konfiguraci
    _configManager.begin();
    const auto& cfg = _configManager.get();

    // 2. Zaregistrovat obrazovky
    registerScreens();
    _screenManager.activateScreen(cfg.display.defaultScreen);
    _dataModel.system.currentScreenId = cfg.display.defaultScreen;

    // 3. Připojení k Wi-Fi (před kreslením, abychom znali IP)
    _wifiManager.onStatusChange([this](bool connected, const String& ip) {
        Serial.printf("[APP] Wi-Fi zmena stavu -> Connected: %d, IP: %s\n", connected, ip.c_str());
        _dataModel.system.wifiConnected = connected;
        _dataModel.system.ipAddress = ip;
        _pendingRefresh = true;
    });

    _wifiManager.begin(cfg.wifi.ssid, cfg.wifi.password, cfg.system.hostname);
    bool wifiOk = _wifiManager.waitForConnection(8000);

    // 4. Spustit NTP časovou službu
    _timeService.begin(cfg.system.timezone, cfg.system.ntpServer);
    if (wifiOk) {
        // Krátké vyčkání na NTP synchronizaci
        unsigned long ntpWait = millis();
        while (!_timeService.isSynced() && (millis() - ntpWait < 2000)) {
            _timeService.loop();
            delay(100);
        }
    }

    // 5. Aktualizovat data pro první vykreslení
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    _dataModel.system.ipAddress = _wifiManager.getIpAddress();
    _dataModel.system.ntpSynced = _timeService.isSynced();
    _dataModel.system.timeStr = _timeService.getTimeStr();
    _dataModel.system.dateStr = _timeService.getDateStr();
    _dataModel.system.dayOfWeekStr = _timeService.getDayOfWeekStr();

    // 6. Nastavit a spustit webový server
    _webServer.onScreenChange([this](const String& screenId) {
        onScreenSwitchRequested(screenId);
    });

    _webServer.onRefresh([this](bool full) {
        onRefreshRequested(full);
    });

    _webServer.onWifiConfig([this](const String& ssid, const String& password) {
        _configManager.setWifi(ssid, password);
        Serial.println("[CONFIG] Wi-Fi ulozena, restartuji zarizeni...");
        delay(250);
        ESP.restart();
    });

    _webServer.onWifiScan([this]() {
        return _wifiManager.scanNetworksJson();
    });

    _webServer.onSourceConfig([this](const GoodWeConfig& goodwe, const AZRouterConfig& azrouter) {
        _configManager.setSources(goodwe, azrouter);
        Serial.println("[CONFIG] Zdroje ulozeny, restartuji zarizeni...");
        delay(250);
        ESP.restart();
    });

    _webServer.begin();

    // 7. Inicializovat datové klienty (GoodWe UDP, AZRouter HTTP)
    if (cfg.goodwe.enabled) {
        _goodweClient.begin(cfg.goodwe.host, cfg.goodwe.port);
    }
    if (cfg.azrouter.enabled) {
        _azrouterClient.begin(cfg.azrouter.host, cfg.azrouter.port);
    }

    // 8. První vyčtení dat (pokud jsme na síti)
    if (_wifiManager.isConnected()) {
        if (cfg.goodwe.enabled) {
            _goodweClient.update(_dataModel.solar);
        }
        if (cfg.azrouter.enabled) {
            _azrouterClient.update(_dataModel.azrouter);
        }
    }
    _dataModel.updateSystemMetrics();

    // 9. Inicializovat displej a vykreslit výchozí obrazovku (již se správnými daty, IP a časem)
    _displayManager.init();
    _displayManager.renderScreen(_screenManager.getActiveScreen(), _dataModel, true);
    _lastDisplayUpdate = millis();

    Serial.println("[APP] Inicializace uspesne dokoncena.");
}

void DashboardApp::registerScreens() {
    _screenManager.registerScreen(&_homeScreen);
    _screenManager.registerScreen(&_solarScreen);
    _screenManager.registerScreen(&_poolScreen);
    _screenManager.registerScreen(&_weatherScreen);
    _screenManager.registerScreen(&_diagnosticsScreen);
}

void DashboardApp::onScreenSwitchRequested(const String& screenId) {
    Serial.printf("[APP] Pozadavek z webu na prepnuti obrazovky: %s\n", screenId.c_str());
    _pendingRefresh = true;
    _pendingFullRefresh = true; // Změna obrazovky provede Full Refresh pro dokonalý kontrast a eliminaci duchů
}

void DashboardApp::onRefreshRequested(bool full) {
    Serial.printf("[APP] Pozadavek z webu na refresh displeje (Full: %d)\n", full);
    _pendingRefresh = true;
    if (full) {
        _pendingFullRefresh = true;
    }
}

void DashboardApp::loop() {
    _wifiManager.loop();
    _timeService.loop();
    _webServer.loop();

    const String previousTimeStr = _dataModel.system.timeStr;
    const String previousDateStr = _dataModel.system.dateStr;
    const String previousDayOfWeekStr = _dataModel.system.dayOfWeekStr;

    // Aktualizace systémových údajů v datovém modelu
    const bool wasWifiConnected = _dataModel.system.wifiConnected;
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    _dataModel.system.ipAddress = _wifiManager.getIpAddress();
    _dataModel.system.ntpSynced = _timeService.isSynced();
    _dataModel.system.timeStr = _timeService.getTimeStr();
    _dataModel.system.dateStr = _timeService.getDateStr();
    _dataModel.system.dayOfWeekStr = _timeService.getDayOfWeekStr();

    if (wasWifiConnected && !_dataModel.system.wifiConnected) {
        _dataModel.solar.status.recordError("WiFi unavailable");
        _dataModel.azrouter.status.recordError("WiFi unavailable");
        _dataModel.updateSystemMetrics();
        _pendingRefresh = true;
        _pendingFullRefresh = false;
    }

    const bool timeChanged = previousTimeStr != _dataModel.system.timeStr ||
                             previousDateStr != _dataModel.system.dateStr ||
                             previousDayOfWeekStr != _dataModel.system.dayOfWeekStr;
    if (timeChanged) {
        _pendingRefresh = true;
        _pendingFullRefresh = false;
    }

    // Zpracování požadavku na překreslení obrazovky
    if (_pendingRefresh) {
        _pendingRefresh = false;
        _displayManager.renderScreen(_screenManager.getActiveScreen(), _dataModel, _pendingFullRefresh);
        _pendingFullRefresh = false;
        _lastScreenRender = millis();
    }

    // Periodické čtení dat podle intervalu každého zdroje.
    unsigned long now = millis();
    if (_wifiManager.isConnected()) {
        const auto& cfg = _configManager.get();
        bool availabilityChanged = false;

        if (cfg.goodwe.enabled && now - _lastGoodweSync >= cfg.goodwe.pollIntervalSeconds * 1000UL) {
            _lastGoodweSync = now;
            bool wasAvailable = _dataModel.solar.status.available;
            _goodweClient.update(_dataModel.solar);
            availabilityChanged |= wasAvailable != _dataModel.solar.status.available;
        }
        if (cfg.azrouter.enabled && now - _lastAzrouterSync >= cfg.azrouter.pollIntervalSeconds * 1000UL) {
            _lastAzrouterSync = now;
            bool wasAvailable = _dataModel.azrouter.status.available;
            _azrouterClient.update(_dataModel.azrouter);
            availabilityChanged |= wasAvailable != _dataModel.azrouter.status.available;
        }

        _dataModel.updateSystemMetrics();
        if (availabilityChanged) {
            _pendingRefresh = true;
            _pendingFullRefresh = false;
        }

        // Periodický částečný refresh displeje každou minutu.
        if (now - _lastDisplayUpdate >= 60000UL) {
            _lastDisplayUpdate = now;
            _pendingRefresh = true;
            _pendingFullRefresh = false;
        }
    }

    delay(20);
}
