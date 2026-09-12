#include "DashboardApp.h"
#include "../../include/AppConfig.h"
#include "../../include/Version.h"

DashboardApp::DashboardApp()
    : _epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCK, EPD_MISO, EPD_MOSI),
      _displayManager(_epaperDisplay),
      _webServer(80, _dataModel, _screenManager) {
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

    // 3. Inicializovat displej a vykreslit výchozí obrazovku
    _displayManager.init();
    _displayManager.renderScreen(_screenManager.getActiveScreen(), _dataModel, true);

    // 4. Připojení k Wi-Fi
    _wifiManager.begin(cfg.wifi.ssid, cfg.wifi.password, cfg.system.hostname);

    // 5. Spustit NTP časovou službu
    _timeService.begin(cfg.system.timezone, cfg.system.ntpServer);

    // 6. Nastavit a spustit webový server
    _webServer.onScreenChange([this](const String& screenId) {
        onScreenSwitchRequested(screenId);
    });

    _webServer.onRefresh([this](bool full) {
        onRefreshRequested(full);
    });

    _webServer.begin();

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
    _pendingFullRefresh = false;
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

    // Aktualizace systémových údajů v datovém modelu
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    _dataModel.system.ipAddress = _wifiManager.getIpAddress();
    _dataModel.system.ntpSynced = _timeService.isSynced();
    _dataModel.system.timeStr = _timeService.getTimeStr();
    _dataModel.system.dateStr = _timeService.getDateStr();
    _dataModel.system.dayOfWeekStr = _timeService.getDayOfWeekStr();

    // Zpracování požadavku na překreslení obrazovky
    if (_pendingRefresh) {
        _pendingRefresh = false;
        _displayManager.renderScreen(_screenManager.getActiveScreen(), _dataModel, _pendingFullRefresh);
        _pendingFullRefresh = false;
        _lastScreenRender = millis();
    }

    // Periodická aktualizace každých 10 sekund
    unsigned long now = millis();
    if (now - _lastDataSync >= STATUS_POLL_INTERVAL_MS) {
        _lastDataSync = now;
        _dataModel.updateSystemMetrics();
    }

    delay(20);
}
