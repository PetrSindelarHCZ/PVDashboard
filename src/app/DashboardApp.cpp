#include "DashboardApp.h"
#include "../diagnostics/Performance.h"
#include "../../include/AppConfig.h"
#include "../../include/Version.h"

namespace {
constexpr uint32_t MinimumPollIntervalMs = 1000;
constexpr uint32_t MaximumBackoffMs = 300000;
constexpr uint8_t MaximumBackoffShift = 5;

uint32_t pollDelayMs(uint32_t intervalSeconds, uint8_t failureStreak) {
    uint64_t baseMs = static_cast<uint64_t>(intervalSeconds) * 1000ULL;
    if (baseMs < MinimumPollIntervalMs) {
        baseMs = MinimumPollIntervalMs;
    }

    const uint8_t shift = failureStreak < MaximumBackoffShift
        ? failureStreak
        : MaximumBackoffShift;
    uint64_t delayMs = baseMs << shift;
    const uint64_t maximumMs = baseMs > MaximumBackoffMs ? baseMs : MaximumBackoffMs;
    if (delayMs > maximumMs) {
        delayMs = maximumMs;
    }
    return static_cast<uint32_t>(delayMs);
}

uint8_t nextFailureStreak(uint8_t current) {
    return current < MaximumBackoffShift ? current + 1 : MaximumBackoffShift;
}
}

DashboardApp::DashboardApp()
    : _epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCK, EPD_MISO, EPD_MOSI),
      _displayManager(_epaperDisplay),
      _displayWorker(_displayManager),
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
        requestAutomaticDisplayRefresh();
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

    _webServer.onDisplayStatus([this]() {
        return _displayWorker.getStatus();
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

    _webServer.onWeatherConfig([this](const WeatherConfig& weather) {
        _configManager.setWeather(weather);
        Serial.println("[CONFIG] Pocasi ulozeno, restartuji zarizeni...");
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

    _weatherWorker.begin(cfg.weather);

    // 8. První čtení dat a inicializace displeje proběhnou až v hlavní smyčce.
    // Web server tak může začít odpovídat ještě před pomalými síťovými a e-paper operacemi.
    _lastGoodweSync = millis();
    _lastAzrouterSync = millis();
    _displayInitNotBefore = millis() + 1500;
    requestDisplayRefresh(true);

    Serial.println("[APP] Inicializace uspesne dokoncena.");
}

void DashboardApp::registerScreens() {
    _screenManager.registerScreen(&_homeScreen);
    _screenManager.registerScreen(&_solarScreen);
    _screenManager.registerScreen(&_poolScreen);
    _screenManager.registerScreen(&_weatherScreen);
    for (auto& screen : _weatherHourlyScreens) {
        _screenManager.registerScreen(&screen);
    }
    _screenManager.registerScreen(&_diagnosticsScreen);
}

void DashboardApp::requestDisplayRefresh(bool full, unsigned long delayMs) {
    _pendingRefresh = true;
    _pendingFullRefresh = _pendingFullRefresh || full;

    const unsigned long requestedAt = millis() + delayMs;
    if (_displayRefreshNotBefore == 0 ||
        static_cast<long>(requestedAt - _displayRefreshNotBefore) > 0) {
        _displayRefreshNotBefore = requestedAt;
    }
}

void DashboardApp::requestAutomaticDisplayRefresh() {
    unsigned long delayMs = 0;
    if (_lastScreenRender != 0) {
        const unsigned long elapsed = millis() - _lastScreenRender;
        constexpr unsigned long CoalesceWindowMs = 5000;
        if (elapsed < CoalesceWindowMs) {
            delayMs = CoalesceWindowMs - elapsed;
        }
    }
    requestDisplayRefresh(false, delayMs);
}

void DashboardApp::onScreenSwitchRequested(const String& screenId) {
    Serial.printf("[APP][%lu ms] Pozadavek na prepnuti obrazovky: %s\n", millis(), screenId.c_str());
    // Zmena celeho obsahu potrebuje plne vycisteni, aby se rozlozeni
    // predchozi obrazovky nepropisovalo do nove.
    requestDisplayRefresh(true, 100);
}

void DashboardApp::onRefreshRequested(bool full) {
    Serial.printf("[APP][%lu ms] Pozadavek na refresh displeje (Full: %d)\n", millis(), full);
    requestDisplayRefresh(full, 100);
}

void DashboardApp::loop() {
    Performance::Scope loopTiming(Performance::Loop);
    _wifiManager.loop();
    _timeService.loop();
    _webServer.loop();

    const bool displayInitDelayElapsed = static_cast<long>(millis() - _displayInitNotBefore) >= 0;
    const bool displayInitFallbackElapsed = static_cast<long>(millis() - _displayInitNotBefore) >= 5000;
    if (!_displayWorkerStarted && displayInitDelayElapsed && (_timeService.isSynced() || displayInitFallbackElapsed)) {
        _displayWorkerStarted = _displayWorker.begin();
        if (_displayWorkerStarted) {
            _lastDisplayUpdate = millis();
        }
    }

    const DisplayTaskStatus displayStatus = _displayWorker.getStatus();
    if (displayStatus.lastCompletedMs != 0 && displayStatus.lastCompletedMs != _lastScreenRender) {
        _lastScreenRender = displayStatus.lastCompletedMs;
        _lastDisplayUpdate = displayStatus.lastCompletedMs;
    }

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
        _dataModel.weather.status.recordError("WiFi unavailable");
        _dataModel.updateSystemMetrics();
        requestAutomaticDisplayRefresh();
    }

    const bool timeChanged = previousTimeStr != _dataModel.system.timeStr ||
                             previousDateStr != _dataModel.system.dateStr ||
                             previousDayOfWeekStr != _dataModel.system.dayOfWeekStr;
    if (timeChanged) {
        requestAutomaticDisplayRefresh();
    }

    WeatherData weatherUpdate;
    if (_weatherWorker.takeLatest(weatherUpdate)) {
        const bool changed = weatherUpdate.status.available != _dataModel.weather.status.available ||
                             weatherUpdate.lastUpdateMs != _dataModel.weather.lastUpdateMs;
        _dataModel.weather = weatherUpdate;
        if (changed) {
            requestAutomaticDisplayRefresh();
        }
    }
    // Po aktualizaci času předat konzistentní snímek workeru. Pokud už kreslí,
    // worker ponechá ve frontě nejnovější data a zachová požadavek na full refresh.
    if (displayStatus.ready && _pendingRefresh &&
        static_cast<long>(millis() - _displayRefreshNotBefore) >= 0) {
        if (_displayWorker.enqueue(_screenManager.getActiveScreen(), _dataModel, _pendingFullRefresh)) {
            _pendingRefresh = false;
            _pendingFullRefresh = false;
            _displayRefreshNotBefore = 0;
        }
    }

    // Periodické čtení dat podle intervalu každého zdroje.
    unsigned long now = millis();
    if (_wifiManager.isConnected()) {
        const auto& cfg = _configManager.get();
        bool availabilityChanged = false;

        const uint32_t goodweDelayMs = pollDelayMs(cfg.goodwe.pollIntervalSeconds, _goodweFailureStreak);
        if (cfg.goodwe.enabled && now - _lastGoodweSync >= goodweDelayMs) {
            const bool wasAvailable = _dataModel.solar.status.available;
            const bool success = _goodweClient.update(_dataModel.solar);
            _lastGoodweSync = millis();
            _goodweFailureStreak = success ? 0 : nextFailureStreak(_goodweFailureStreak);
            availabilityChanged |= wasAvailable != _dataModel.solar.status.available;
            if (!success) {
                Serial.printf("[GOODWE] Další pokus za %lu ms (chyby v řadě: %u)\n",
                              pollDelayMs(cfg.goodwe.pollIntervalSeconds, _goodweFailureStreak),
                              _goodweFailureStreak);
            }
        }

        now = millis();
        const uint32_t azrouterDelayMs = pollDelayMs(cfg.azrouter.pollIntervalSeconds, _azrouterFailureStreak);
        if (cfg.azrouter.enabled && now - _lastAzrouterSync >= azrouterDelayMs) {
            const bool wasAvailable = _dataModel.azrouter.status.available;
            const bool success = _azrouterClient.update(_dataModel.azrouter);
            _lastAzrouterSync = millis();
            _azrouterFailureStreak = success ? 0 : nextFailureStreak(_azrouterFailureStreak);
            availabilityChanged |= wasAvailable != _dataModel.azrouter.status.available;
            if (!success) {
                Serial.printf("[AZROUTER] Další pokus za %lu ms (chyby v řadě: %u)\n",
                              pollDelayMs(cfg.azrouter.pollIntervalSeconds, _azrouterFailureStreak),
                              _azrouterFailureStreak);
            }
        }

        _dataModel.updateSystemMetrics();
        if (availabilityChanged) {
            requestAutomaticDisplayRefresh();
        }

        // Periodický částečný refresh displeje každou minutu.
        if (now - _lastDisplayUpdate >= 60000UL) {
            _lastDisplayUpdate = now;
            requestAutomaticDisplayRefresh();
        }
    }

    delay(20);
    Performance::report();
}
