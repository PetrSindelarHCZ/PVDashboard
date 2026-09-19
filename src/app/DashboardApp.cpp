#include "DashboardApp.h"
#include <math.h>
#include "../diagnostics/Performance.h"
#include "../../include/AppConfig.h"
#include "../../include/Version.h"

namespace {
constexpr uint32_t MinimumPollIntervalMs = 1000;
constexpr uint32_t MaximumBackoffMs = 300000;
constexpr uint8_t MaximumBackoffShift = 5;
constexpr uint32_t Bme280PollIntervalMs = 10000;
constexpr uint32_t Bme280DisplayRefreshIntervalMs = 60000;

uint32_t pollDelayMs(uint32_t intervalSeconds, uint8_t failureStreak) {
    uint64_t baseMs = static_cast<uint64_t>(intervalSeconds) * 1000ULL;
    if (baseMs < MinimumPollIntervalMs) baseMs = MinimumPollIntervalMs;
    const uint8_t shift = failureStreak < MaximumBackoffShift ? failureStreak : MaximumBackoffShift;
    uint64_t delayMs = baseMs << shift;
    const uint64_t maximumMs = baseMs > MaximumBackoffMs ? baseMs : MaximumBackoffMs;
    if (delayMs > maximumMs) delayMs = maximumMs;
    return static_cast<uint32_t>(delayMs);
}

uint8_t nextFailureStreak(uint8_t current) {
    return current < MaximumBackoffShift ? current + 1 : MaximumBackoffShift;
}

bool applyWifiAddressing(const WifiConfig& wifi) {
    if (wifi.dhcp) {
        // Při přechodu ze statické adresy nejdřív ukončíme aktivní STA spojení.
        // Jinak může lwIP ještě krátce obsluhovat původní statickou IP, než se
        // DHCP klient skutečně rozběhne a získá novou lease.
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[WIFI] Prechod na DHCP: odpojuji STA pred zmenou adresace...");
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(false);
            const unsigned long disconnectStarted = millis();
            while (WiFi.status() == WL_CONNECTED && millis() - disconnectStarted < 500UL) {
                delay(5);
            }
        }
        const bool ok = WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        Serial.printf("[WIFI] IP konfigurace: DHCP (%s)\n", ok ? "OK" : "CHYBA");
        return ok;
    }

    IPAddress ip, gateway, subnet, dns1, dns2;
    if (!ip.fromString(wifi.ipAddress) || !gateway.fromString(wifi.gateway) ||
        !subnet.fromString(wifi.subnetMask) || !dns1.fromString(wifi.dns1)) {
        Serial.println("[WIFI] Neplatna staticka IP konfigurace.");
        return false;
    }
    if (!wifi.dns2.isEmpty()) dns2.fromString(wifi.dns2);
    const bool ok = WiFi.config(ip, gateway, subnet, dns1, dns2);
    Serial.printf("[WIFI] IP konfigurace: STATIC %s / %s | GW %s | DNS %s%s%s (%s)\n",
                  wifi.ipAddress.c_str(), wifi.subnetMask.c_str(), wifi.gateway.c_str(), wifi.dns1.c_str(),
                  wifi.dns2.isEmpty() ? "" : ", ", wifi.dns2.c_str(), ok ? "OK" : "CHYBA");
    return ok;
}

bool isWeatherScreenId(const String& screenId) {
    return screenId == "weather" || screenId.startsWith("weather-hourly-");
}

int weatherLocationIndexById(const WeatherConfig& weather, const String& locationId) {
    for (uint8_t i = 0;
         i < weather.locationCount && i < MaxWeatherLocations;
         ++i) {
        if (weather.locations[i].id == locationId) return static_cast<int>(i);
    }
    return -1;
}

const char* weatherProviderLabel(const String& provider) {
    if (provider == "met-no") return "MET Norway";
    if (provider == "open-meteo") return "Open-Meteo";
    return provider.c_str();
}

bool weatherDisplayDataChanged(const WeatherData& a, const WeatherData& b) {
    return a.status.available != b.status.available ||
           a.status.lastError != b.status.lastError ||
           a.lastUpdateMs != b.lastUpdateMs ||
           a.provider != b.provider ||
           a.locationId != b.locationId ||
           a.locationName != b.locationName ||
           a.dailyCount != b.dailyCount ||
           a.hourlyCount != b.hourlyCount;
}

}

DashboardApp::DashboardApp()
    : _epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, EPD_SCK, EPD_MISO, EPD_MOSI),
      _displayPreview(),
      _displayManager(_epaperDisplay, &_displayPreview),
      _displayWorker(_displayManager),
      _navigationController(_screenManager, _dataModel),
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

    _configManager.begin();

    _memoryHeavyGate = xSemaphoreCreateMutex();
    if (_memoryHeavyGate == nullptr) {
        Serial.println("[APP] VAROVANI: memory-heavy gate se nepodarilo vytvorit.");
    } else {
        _displayWorker.setMemoryHeavyGate(_memoryHeavyGate);
        _weatherWorker.setMemoryHeavyGate(_memoryHeavyGate);
        Serial.println("[APP] Memory-heavy gate pripraven pro Display/TLS.");
    }

    _displayPreview.init(); // tiled preview; komprimovany snapshot se drzi mimo TLS DRAM
    const auto& cfg = _configManager.get();
    _homeScreen.setLayoutConfig(&cfg.display.homeLayout);

    // Local environmental sensor is independent of Wi-Fi. The 4-pin BME280
    // shares the preferred I2C bus on SDA GPIO21 / SCL GPIO22.
    _bme280Sensor.update(_dataModel.inside);
    _lastBme280Sync = millis();

    applyWifiAddressing(cfg.wifi);

    _dataModel.solar.enabled = cfg.goodwe.enabled;
    _dataModel.azrouter.enabled = cfg.azrouter.enabled;
    registerScreens();
    if (!_screenManager.activateScreen(cfg.display.defaultScreen)) {
        _screenManager.activateScreen("home");
    }
    _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
    _dataModel.pool.enabled = cfg.pool.enabled;
    _dataModel.weather.enabled = cfg.weather.enabled;
    const WeatherLocation* initialWeatherLocation = cfg.weather.activeLocation();
    const int initialWeatherIndex =
        initialWeatherLocation != nullptr
            ? weatherLocationIndexById(cfg.weather, initialWeatherLocation->id)
            : -1;
    _weatherDisplayLocationIndex =
        initialWeatherIndex >= 0 ? static_cast<uint8_t>(initialWeatherIndex) : 0;
    _weatherDisplayLocationId =
        initialWeatherLocation != nullptr ? initialWeatherLocation->id : "";
    _dataModel.weather.locationId = _weatherDisplayLocationId;
    _dataModel.weather.locationName =
        initialWeatherLocation != nullptr ? initialWeatherLocation->name : "";
    _dataModel.weather.locationIndex = _weatherDisplayLocationIndex;
    _dataModel.weather.locationCount =
        min<uint8_t>(cfg.weather.locationCount, MaxWeatherLocations);
    _navigationController.syncToActiveScreen();
    _joystick.begin();

    _wifiManager.onStatusChange([this](bool connected, const String& ip) {
        Serial.printf("[APP] Wi-Fi zmena stavu -> Connected: %d, IP: %s\n", connected, ip.c_str());

        if (connected && _pendingWifiSave && _wifiManager.getSsid() == _pendingWifiSsid) {
            _configManager.setWifi(_pendingWifiSsid, _pendingWifiPassword);
            Serial.printf("[WIFI] Rucne vybrana sit '%s' se uspesne pripojila a byla ulozena.\n",
                          _pendingWifiSsid.c_str());
            _pendingWifiSave = false;
            _pendingWifiSsid = "";
            _pendingWifiPassword = "";
        }

        _dataModel.system.wifiConnected = connected;
        _dataModel.system.wifiSignalLevel = _wifiSignalLevel.update(connected, _wifiManager.getRssi(), millis());
        _dataModel.system.ipAddress = ip;
        requestAutomaticDisplayRefresh();
    });

    _wifiManager.onKnownNetworkLookup([this](const String& ssid, String& password) {
        return _configManager.getAutoJoinWifiPassword(ssid, password);
    });
    _wifiManager.onAutoNetworkSelected([this](const String& ssid, const String& password) {
        _pendingWifiSave = false;
        _pendingWifiSsid = "";
        _pendingWifiPassword = "";
        _configManager.setWifi(ssid, password);
        Serial.printf("[WIFI] Automaticky obnovena znama sit '%s' a nastavena jako aktivni.\n", ssid.c_str());
        requestAutomaticDisplayRefresh();
    });

    bool wifiOk = false;
    const bool hasConfiguredWifi = !cfg.wifi.ssid.isEmpty() && cfg.wifi.ssid != "VASE_WIFI";
    const bool configuredWifiAllowed = hasConfiguredWifi && _configManager.isWifiAutoConnectEnabled(cfg.wifi.ssid);

    if (configuredWifiAllowed) {
        _wifiManager.begin(cfg.wifi.ssid, cfg.wifi.password, cfg.system.hostname);
        wifiOk = _wifiManager.waitForConnection(8000);
    } else {
        if (hasConfiguredWifi) {
            Serial.printf("[WIFI] Sit '%s' byla rucne odpojena. Po restartu ji automaticky nezkousim.\n", cfg.wifi.ssid.c_str());
        }
        _wifiManager.begin("", "", cfg.system.hostname);
    }

    _timeService.begin(cfg.system.timezone, cfg.system.ntpServer);
    if (wifiOk) {
        unsigned long ntpWait = millis();
        while (!_timeService.isSynced() && (millis() - ntpWait < 2000)) {
            _timeService.loop();
            delay(100);
        }
    }

    const bool previousAccessPoint = _dataModel.system.wifiAccessPoint;
    _dataModel.system.wifiAccessPoint = _wifiManager.isConfigAccessPoint();
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    const uint8_t previousSignalLevel = _dataModel.system.wifiSignalLevel;
    _dataModel.system.wifiSignalLevel = _wifiSignalLevel.update(_dataModel.system.wifiConnected, _dataModel.system.wifiRssi, millis());
    if (previousSignalLevel != _dataModel.system.wifiSignalLevel || previousAccessPoint != _dataModel.system.wifiAccessPoint) requestAutomaticDisplayRefresh();
    _dataModel.system.ipAddress = _wifiManager.getIpAddress();
    _dataModel.system.ntpSynced = _timeService.isSynced();
    _dataModel.system.timeStr = _timeService.getTimeStr();
    _dataModel.system.dateStr = _timeService.getDateStr();
    _dataModel.system.dayOfWeekStr = _timeService.getDayOfWeekStr();

    _navigationController.onSubpageChange(
        [this](const String& screenId, uint8_t subpageIndex) {
            onNavigationSubpageChanged(screenId, subpageIndex);
        });

    _navigationController.onChange([this](bool fullRefresh) {
        syncWeatherDisplayForActiveScreen(false);
        requestDisplayRefresh(fullRefresh, 50);
    });

    _webServer.onScreenChange([this](const String& screenId) { onScreenSwitchRequested(screenId); });
    _webServer.onRefresh([this](bool full) { onRefreshRequested(full); });
    _webServer.onDisplayStatus([this]() { return _displayWorker.getStatus(); });
    _webServer.setDisplayPreview(&_displayPreview);
    _webServer.setNavigationController(&_navigationController);

    _webServer.onSystemConfig([this](const SystemConfig& system) {
        const String previousHostname = _configManager.get().system.hostname;
        _configManager.setSystem(system);
        _timeService.begin(system.timezone, system.ntpServer);
        _dataModel.system.ntpSynced = false;
        _dataModel.system.timeStr = "";
        _dataModel.system.dateStr = "";
        _dataModel.system.dayOfWeekStr = "";
        if (previousHostname != system.hostname) {
            const auto& current = _configManager.get();
            applyWifiAddressing(current.wifi);
            if (!current.wifi.ssid.isEmpty() && _configManager.isWifiAutoConnectEnabled(current.wifi.ssid))
                _wifiManager.begin(current.wifi.ssid, current.wifi.password, system.hostname);
            else _wifiManager.begin("", "", system.hostname);
        }
        requestAutomaticDisplayRefresh();
        Serial.println("[CONFIG] System ulozen a aplikovan za behu.");
    });

    _webServer.onWifiConfig([this](const String& ssid, const String& password) {
        const auto& current = _configManager.get();
        _pendingWifiSave = true;
        _pendingWifiSsid = ssid;
        _pendingWifiPassword = password;
        applyWifiAddressing(current.wifi);
        Serial.printf("[WIFI] Rucne zkousim sit '%s'; ulozim ji az po uspesnem pripojeni.\n", ssid.c_str());
        _wifiManager.begin(ssid, password, current.system.hostname);
    });

    _webServer.onWifiNetworkConfig([this](const WifiConfig& wifi) {
        _pendingWifiSave = false;
        _pendingWifiSsid = "";
        _pendingWifiPassword = "";
        _configManager.setWifiNetworkConfig(wifi);
        const auto& current = _configManager.get();
        applyWifiAddressing(current.wifi);
        Serial.printf("[CONFIG] IP rezim ulozen: %s. Obnovuji Wi-Fi pripojeni.\n", current.wifi.dhcp ? "DHCP" : "STATIC");
        if (!current.wifi.ssid.isEmpty() && _configManager.isWifiAutoConnectEnabled(current.wifi.ssid))
            _wifiManager.begin(current.wifi.ssid, current.wifi.password, current.system.hostname);
        else _wifiManager.begin("", "", current.system.hostname);
    });

    _webServer.onWifiScan([this]() { return _wifiManager.scanNetworksJson(); });
    _webServer.onWifiKnownNetworks([this]() { return _configManager.getKnownWifiNetworksJson(); });

    _webServer.onWifiConnectKnown([this](const String& ssid) {
        String password;
        if (!_configManager.getKnownWifiPassword(ssid, password)) return false;
        const auto& current = _configManager.get();
        _pendingWifiSave = true;
        _pendingWifiSsid = ssid;
        _pendingWifiPassword = password;
        applyWifiAddressing(current.wifi);
        Serial.printf("[WIFI] Rucne zkousim znamou sit '%s'; auto-connect povolim az po uspesnem pripojeni.\n", ssid.c_str());
        _wifiManager.begin(ssid, password, current.system.hostname);
        return true;
    });

    _webServer.onWifiDisconnect([this]() {
        _pendingWifiSave = false;
        _pendingWifiSsid = "";
        _pendingWifiPassword = "";
        const String activeSsid = _configManager.get().wifi.ssid;
        if (!activeSsid.isEmpty() && activeSsid != "VASE_WIFI") {
            if (_configManager.setWifiAutoConnectEnabled(activeSsid, false))
                Serial.printf("[WIFI] Sit '%s' byla rucne odpojena a zustane vyradena z auto-connectu do rucniho Pripojit.\n", activeSsid.c_str());
        }
        _wifiManager.disconnectToConfigAccessPoint();
        requestAutomaticDisplayRefresh();
    });

    _webServer.onWifiForget([this](const String& ssid) {
        if (_pendingWifiSave && _pendingWifiSsid == ssid) {
            _pendingWifiSave = false;
            _pendingWifiSsid = "";
            _pendingWifiPassword = "";
        }
        const bool wasActive = _configManager.get().wifi.ssid == ssid;
        if (!_configManager.forgetWifi(ssid)) return false;
        if (wasActive) { _wifiManager.disconnectToConfigAccessPoint(); requestAutomaticDisplayRefresh(); }
        Serial.printf("[WIFI] Sit '%s' byla zapomenuta.\n", ssid.c_str());
        return true;
    });

    _webServer.onSourceConfig([this](const GoodWeConfig& goodwe, const AZRouterConfig& azrouter) {
        const bool visibilityChanged =
            _configManager.get().goodwe.enabled != goodwe.enabled ||
            _configManager.get().azrouter.enabled != azrouter.enabled;

        _configManager.setSources(goodwe, azrouter);
        _dataModel.solar.enabled = goodwe.enabled;
        _dataModel.azrouter.enabled = azrouter.enabled;

        if (goodwe.enabled) {
            _goodweClient.begin(goodwe.host, goodwe.port);
        } else {
            _dataModel.solar.status.recordError("Disabled");
            _dataModel.solar.historyCount = 0;
        }

        if (azrouter.enabled) {
            _azrouterClient.begin(azrouter.host, azrouter.port);
        } else {
            _dataModel.azrouter.status.recordError("Disabled");
        }

        setSolarScreenEnabled(goodwe.enabled || azrouter.enabled);
        _navigationController.syncToActiveScreen(false);

        _goodweFailureStreak = 0;
        _azrouterFailureStreak = 0;
        _lastGoodweSync = millis() - goodwe.pollIntervalSeconds * 1000UL;
        _lastAzrouterSync = millis() - azrouter.pollIntervalSeconds * 1000UL;
        _dataModel.updateSystemMetrics();

        if (visibilityChanged) requestDisplayRefresh(true, 100);
        else requestAutomaticDisplayRefresh();

        Serial.println("[CONFIG] Datove zdroje ulozeny a aplikovany za behu.");
    });

    _webServer.onPoolConfig([this](const PoolConfig& pool) {
        const bool enabledChanged = _configManager.get().pool.enabled != pool.enabled;
        _configManager.setPool(pool);
        _dataModel.pool.enabled = pool.enabled;
        setPoolScreenEnabled(pool.enabled);
        _navigationController.syncToActiveScreen(false);
        if (enabledChanged) requestDisplayRefresh(true, 100);
        else requestAutomaticDisplayRefresh();
        Serial.println("[CONFIG] Bazen ulozen a aplikovan za behu.");
    });

    _webServer.onHomeLayoutConfig([this](const HomeLayoutConfig& layout) {
        if (!_configManager.setHomeLayout(layout)) return false;
        _navigationController.syncToActiveScreen(false);
        requestDisplayRefresh(true, 100);
        Serial.println("[CONFIG] Home layout ulozen a aplikovan za behu.");
        return true;
    });

    _webServer.onWeatherConfig([this](const WeatherConfig& weather) {
        const WeatherConfig previous = _configManager.get().weather;
        const bool enabledChanged = previous.enabled != weather.enabled;
        const bool providerChanged = previous.provider != weather.provider;
        const bool locationChanged =
            fabs(previous.latitude - weather.latitude) > 0.00001 ||
            fabs(previous.longitude - weather.longitude) > 0.00001;

        _configManager.setWeather(weather);
        const WeatherConfig& applied = _configManager.get().weather;
        if (!_weatherWorker.reconfigure(applied)) {
            Serial.println("[CONFIG] Nepodarilo se aplikovat konfiguraci pocasi za behu.");
        }

        const WeatherLocation* activeLocation = applied.activeLocation();
        const int activeIndex =
            activeLocation != nullptr
                ? weatherLocationIndexById(applied, activeLocation->id)
                : -1;

        _weatherDisplayLocationIndex =
            activeIndex >= 0 ? static_cast<uint8_t>(activeIndex) : 0;
        _weatherDisplayLocationId =
            activeLocation != nullptr ? activeLocation->id : "";

        if (providerChanged || locationChanged) {
            // Starou predpoved nesmime po prepnuti zdroje/lokality vydavat za
            // data nove konfigurace. Cekame na prvni platnou odpoved workeru.
            WeatherData pending;
            pending.enabled = applied.enabled;
            pending.provider = weatherProviderLabel(applied.provider);
            pending.locationId = _weatherDisplayLocationId;
            pending.locationName =
                activeLocation != nullptr ? activeLocation->name : "";
            pending.locationIndex = _weatherDisplayLocationIndex;
            pending.locationCount =
                min<uint8_t>(applied.locationCount, MaxWeatherLocations);
            pending.status.available = false;
            pending.status.lastAttemptMs = millis();
            pending.status.lastError = "Aktualizuji pocasi";
            _dataModel.weather = pending;
        } else {
            selectWeatherDisplayLocation(
                _weatherDisplayLocationIndex,
                false);
        }

        if (!applied.enabled) {
            _dataModel.weather.status.recordError("Weather disabled");
        }

        setWeatherScreensEnabled(applied.enabled);
        _navigationController.syncToActiveScreen(false);
        if (enabledChanged) requestDisplayRefresh(true, 100);
        else requestAutomaticDisplayRefresh();
        Serial.println("[CONFIG] Pocasi ulozeno a aplikovano za behu.");
    });

    _webServer.onFactoryReset([this]() { return _configManager.resetToFactoryDefaults(); });
    _webServer.onConfigImport([this](const AppConfig& config) { return _configManager.setUserConfiguration(config); });

    _webServer.enableTimezoneUiExtension();
    _webServer.begin();

    if (cfg.goodwe.enabled) _goodweClient.begin(cfg.goodwe.host, cfg.goodwe.port);
    if (cfg.azrouter.enabled) _azrouterClient.begin(cfg.azrouter.host, cfg.azrouter.port);
    _weatherWorker.begin(cfg.weather);

    _lastGoodweSync = millis();
    _lastAzrouterSync = millis();
    _displayInitNotBefore = millis() + 1500;
    requestDisplayRefresh(true);
    Serial.println("[APP] Inicializace uspesne dokoncena.");
}

void DashboardApp::registerScreens() {
    _screenManager.registerScreen(&_homeScreen);
    if (_configManager.get().goodwe.enabled || _configManager.get().azrouter.enabled) {
        _screenManager.registerScreen(&_solarScreen);
    }
    if (_configManager.get().pool.enabled) {
        _screenManager.registerScreen(&_poolScreen);
    }
    if (_configManager.get().weather.enabled) {
        _screenManager.registerScreen(&_weatherScreen);
        for (auto& screen : _weatherHourlyScreens) _screenManager.registerScreen(&screen);
    }
    _screenManager.registerScreen(&_diagnosticsScreen);
}

void DashboardApp::setSolarScreenEnabled(bool enabled) {
    const bool solarWasActive = _screenManager.getActiveScreenId() == "solar";

    if (enabled) {
        // Solar belongs directly after Home in the visual sidebar. Inserting it
        // at the canonical position keeps UP/DOWN navigation aligned even when
        // the page is re-enabled at runtime.
        _screenManager.registerScreenAt(&_solarScreen, 1);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("solar");

    if (solarWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 100);
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::setPoolScreenEnabled(bool enabled) {
    const bool poolWasActive = _screenManager.getActiveScreenId() == "pool";

    if (enabled) {
        _screenManager.registerScreen(&_poolScreen);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("pool");

    if (poolWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 100);
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::setWeatherScreensEnabled(bool enabled) {
    const String activeId = _screenManager.getActiveScreenId();
    const bool weatherWasActive = activeId == "weather" || activeId.startsWith("weather-hourly-");

    if (enabled) {
        _screenManager.registerScreen(&_weatherScreen);
        for (auto& screen : _weatherHourlyScreens) _screenManager.registerScreen(&screen);
        _navigationController.syncToActiveScreen(false);
        return;
    }

    _screenManager.unregisterScreen("weather");
    for (uint8_t i = 0; i < WeatherForecastDayCount; ++i) {
        _screenManager.unregisterScreen("weather-hourly-" + String(i));
    }

    if (weatherWasActive) {
        _screenManager.activateScreen("home");
        _dataModel.system.currentScreenId = _screenManager.getActiveScreenId();
        requestDisplayRefresh(true, 100);
    }
    _navigationController.syncToActiveScreen(false);
}

void DashboardApp::requestDisplayRefresh(bool full, unsigned long delayMs) {
    _pendingRefresh = true;
    _pendingFullRefresh = _pendingFullRefresh || full;
    const unsigned long requestedAt = millis() + delayMs;
    if (_displayRefreshNotBefore == 0 || static_cast<long>(requestedAt - _displayRefreshNotBefore) > 0) _displayRefreshNotBefore = requestedAt;
}

void DashboardApp::requestAutomaticDisplayRefresh() {
    unsigned long delayMs = 0;
    if (_lastScreenRender != 0) {
        const unsigned long elapsed = millis() - _lastScreenRender;
        constexpr unsigned long CoalesceWindowMs = 5000;
        if (elapsed < CoalesceWindowMs) delayMs = CoalesceWindowMs - elapsed;
    }
    requestDisplayRefresh(false, delayMs);
}

void DashboardApp::onScreenSwitchRequested(const String& screenId) {
    _navigationController.syncToActiveScreen(false);
    syncWeatherDisplayForActiveScreen(false);
    Serial.printf("[APP][%lu ms] Pozadavek na prepnuti obrazovky: %s\n", millis(), screenId.c_str());
    requestDisplayRefresh(true, 100);
}

void DashboardApp::onNavigationSubpageChanged(
    const String& screenId,
    uint8_t subpageIndex) {
    if (screenId != "weather") return;
    selectWeatherDisplayLocation(subpageIndex, false);
}

void DashboardApp::selectWeatherDisplayLocation(
    uint8_t index,
    bool requestRefresh) {
    const WeatherConfig& weather = _configManager.get().weather;
    const uint8_t count =
        min<uint8_t>(weather.locationCount, MaxWeatherLocations);

    if (!weather.enabled || count == 0) {
        _weatherDisplayLocationId = "";
        _weatherDisplayLocationIndex = 0;
        _dataModel.weather.enabled = false;
        _dataModel.weather.locationId = "";
        _dataModel.weather.locationName = "";
        _dataModel.weather.locationIndex = 0;
        _dataModel.weather.locationCount = 0;
        return;
    }

    if (index >= count) index = 0;

    const WeatherLocation& location = weather.locations[index];
    _weatherDisplayLocationIndex = index;
    _weatherDisplayLocationId = location.id;

    WeatherData selected;
    if (_weatherWorker.copyCached(location.id, selected)) {
        selected.locationId = location.id;
        selected.locationName = location.name;
        selected.locationIndex = index;
        selected.locationCount = count;
        _dataModel.weather = selected;
    } else {
        WeatherData pending;
        pending.enabled = weather.enabled;
        pending.provider = weatherProviderLabel(weather.provider);
        pending.locationId = location.id;
        pending.locationName = location.name;
        pending.locationIndex = index;
        pending.locationCount = count;
        pending.status.available = false;
        pending.status.lastAttemptMs = millis();
        pending.status.lastError = "Nacitam data lokality";
        _dataModel.weather = pending;
        _weatherWorker.requestLocation(location.id);
    }

    if (requestRefresh) requestAutomaticDisplayRefresh();
}

void DashboardApp::syncWeatherDisplayForActiveScreen(
    bool requestRefresh) {
    const WeatherConfig& weather = _configManager.get().weather;
    if (!weather.enabled || weather.locationCount == 0) return;

    const String screenId = _screenManager.getActiveScreenId();
    if (isWeatherScreenId(screenId)) {
        int index = weatherLocationIndexById(
            weather,
            _weatherDisplayLocationId);
        if (index < 0) {
            const WeatherLocation* active = weather.activeLocation();
            index =
                active != nullptr
                    ? weatherLocationIndexById(weather, active->id)
                    : 0;
        }
        selectWeatherDisplayLocation(
            index >= 0 ? static_cast<uint8_t>(index) : 0,
            requestRefresh);
        return;
    }

    const WeatherLocation* active = weather.activeLocation();
    int index =
        active != nullptr
            ? weatherLocationIndexById(weather, active->id)
            : 0;
    selectWeatherDisplayLocation(
        index >= 0 ? static_cast<uint8_t>(index) : 0,
        requestRefresh);
}

void DashboardApp::refreshWeatherDisplayFromCache(
    bool requestRefresh) {
    const WeatherConfig& weather = _configManager.get().weather;
    if (!weather.enabled || _weatherDisplayLocationId.isEmpty()) return;

    WeatherData cached;
    if (!_weatherWorker.copyCached(
            _weatherDisplayLocationId,
            cached)) {
        return;
    }

    int index = weatherLocationIndexById(
        weather,
        _weatherDisplayLocationId);
    if (index < 0) return;

    cached.locationIndex = static_cast<uint8_t>(index);
    cached.locationCount =
        min<uint8_t>(weather.locationCount, MaxWeatherLocations);

    const bool changed =
        weatherDisplayDataChanged(cached, _dataModel.weather);
    if (!changed) return;

    _dataModel.weather = cached;
    if (requestRefresh) requestAutomaticDisplayRefresh();
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

    NavigationAction joystickAction;
    if (_joystick.poll(joystickAction)) {
        _navigationController.handleAction(joystickAction);
    }

    const bool displayInitDelayElapsed = static_cast<long>(millis() - _displayInitNotBefore) >= 0;
    const bool displayInitFallbackElapsed = static_cast<long>(millis() - _displayInitNotBefore) >= 5000;
    if (!_displayWorkerStarted && displayInitDelayElapsed && (_timeService.isSynced() || displayInitFallbackElapsed)) {
        _displayWorkerStarted = _displayWorker.begin();
        if (_displayWorkerStarted) _lastDisplayUpdate = millis();
    }

    const DisplayTaskStatus displayStatus = _displayWorker.getStatus();
    if (displayStatus.lastCompletedMs != 0 && displayStatus.lastCompletedMs != _lastScreenRender) {
        _lastScreenRender = displayStatus.lastCompletedMs;
        _lastDisplayUpdate = displayStatus.lastCompletedMs;
    }

    const String previousTimeStr = _dataModel.system.timeStr;
    const String previousDateStr = _dataModel.system.dateStr;
    const String previousDayOfWeekStr = _dataModel.system.dayOfWeekStr;

    const bool wasWifiConnected = _dataModel.system.wifiConnected;
    const bool previousAccessPoint = _dataModel.system.wifiAccessPoint;
    _dataModel.system.wifiAccessPoint = _wifiManager.isConfigAccessPoint();
    _dataModel.system.wifiConnected = _wifiManager.isConnected();
    _dataModel.system.wifiRssi = _wifiManager.getRssi();
    const uint8_t previousSignalLevel = _dataModel.system.wifiSignalLevel;
    _dataModel.system.wifiSignalLevel = _wifiSignalLevel.update(_dataModel.system.wifiConnected, _dataModel.system.wifiRssi, millis());
    if (previousSignalLevel != _dataModel.system.wifiSignalLevel || previousAccessPoint != _dataModel.system.wifiAccessPoint) requestAutomaticDisplayRefresh();
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

    const bool timeChanged = previousTimeStr != _dataModel.system.timeStr || previousDateStr != _dataModel.system.dateStr || previousDayOfWeekStr != _dataModel.system.dayOfWeekStr;
    if (timeChanged) requestAutomaticDisplayRefresh();

    WeatherData weatherUpdate;
    if (_weatherWorker.takeLatest(weatherUpdate)) {
        const WeatherConfig& weather = _configManager.get().weather;
        const String activeScreenId = _screenManager.getActiveScreenId();

        // Home and non-weather screens always consume the configured active
        // location. Weather pager may temporarily display another cached
        // location without changing persistent configuration.
        const bool shouldApply =
            !isWeatherScreenId(activeScreenId) ||
            weatherUpdate.locationId == _weatherDisplayLocationId;

        if (shouldApply) {
            int index = weatherLocationIndexById(
                weather,
                weatherUpdate.locationId);
            if (index < 0) index = 0;

            weatherUpdate.locationIndex =
                static_cast<uint8_t>(index);
            weatherUpdate.locationCount =
                min<uint8_t>(weather.locationCount, MaxWeatherLocations);

            const bool changed =
                weatherDisplayDataChanged(
                    weatherUpdate,
                    _dataModel.weather);
            _dataModel.weather = weatherUpdate;
            if (changed) requestAutomaticDisplayRefresh();
        }
    }

    const unsigned long weatherCacheNow = millis();
    if (isWeatherScreenId(_screenManager.getActiveScreenId()) &&
        weatherCacheNow - _lastWeatherDisplayCacheCheck >= 1000UL) {
        _lastWeatherDisplayCacheCheck = weatherCacheNow;
        refreshWeatherDisplayFromCache(true);
    }

    if (displayStatus.ready && _pendingRefresh && static_cast<long>(millis() - _displayRefreshNotBefore) >= 0) {
        if (_displayWorker.enqueue(_screenManager.getActiveScreen(), _dataModel, _pendingFullRefresh)) {
            _pendingRefresh = false; _pendingFullRefresh = false; _displayRefreshNotBefore = 0;
        }
    }

    unsigned long now = millis();

    if (now - _lastBme280Sync >= Bme280PollIntervalMs) {
        const bool wasAvailable = _dataModel.inside.status.available;
        const float previousTemperature = _dataModel.inside.temperatureC;
        const int previousHumidity = _dataModel.inside.humidityPercent;
        const float previousPressure = _dataModel.inside.pressureHpa;

        const bool success = _bme280Sensor.update(_dataModel.inside);
        _lastBme280Sync = millis();

        const bool availabilityChanged =
            wasAvailable != _dataModel.inside.status.available;
        const bool valuesChanged =
            success &&
            (fabsf(previousTemperature - _dataModel.inside.temperatureC) >= 0.2f ||
             abs(previousHumidity - _dataModel.inside.humidityPercent) >= 1 ||
             fabsf(previousPressure - _dataModel.inside.pressureHpa) >= 1.0f);

        const unsigned long refreshNow = millis();
        const bool refreshIntervalElapsed =
            _lastBme280DisplayRefresh == 0 ||
            refreshNow - _lastBme280DisplayRefresh >= Bme280DisplayRefreshIntervalMs;

        if (availabilityChanged || (valuesChanged && refreshIntervalElapsed)) {
            _lastBme280DisplayRefresh = refreshNow;
            requestAutomaticDisplayRefresh();
        }
    }

    now = millis();
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
            if (!success) Serial.printf("[GOODWE] Dalsi pokus za %lu ms (chyby v rade: %u)\n", pollDelayMs(cfg.goodwe.pollIntervalSeconds, _goodweFailureStreak), _goodweFailureStreak);
        }

        now = millis();
        const uint32_t azrouterDelayMs = pollDelayMs(cfg.azrouter.pollIntervalSeconds, _azrouterFailureStreak);
        if (cfg.azrouter.enabled && now - _lastAzrouterSync >= azrouterDelayMs) {
            const bool wasAvailable = _dataModel.azrouter.status.available;
            const bool success = _azrouterClient.update(_dataModel.azrouter);
            _lastAzrouterSync = millis();
            _azrouterFailureStreak = success ? 0 : nextFailureStreak(_azrouterFailureStreak);
            availabilityChanged |= wasAvailable != _dataModel.azrouter.status.available;
            if (!success) Serial.printf("[AZROUTER] Dalsi pokus za %lu ms (chyby v rade: %u)\n", pollDelayMs(cfg.azrouter.pollIntervalSeconds, _azrouterFailureStreak), _azrouterFailureStreak);
        }

        _dataModel.updateSystemMetrics();
        if (availabilityChanged) requestAutomaticDisplayRefresh();
        if (now - _lastDisplayUpdate >= 60000UL) { _lastDisplayUpdate = now; requestAutomaticDisplayRefresh(); }
    }

    delay(20);
    Performance::report();
}
