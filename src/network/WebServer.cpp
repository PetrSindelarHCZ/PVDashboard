#include "WebServer.h"
#include "../diagnostics/Performance.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include "../../include/Version.h"

namespace {
    static const char* GITHUB_RELEASE_API_URL = "https://api.github.com/repos/PetrSindelarHCZ/PVDashboard/releases/latest";
    static const char* GITHUB_FIRMWARE_ASSET_NAME = "firmware.bin";
    static const char* GITHUB_MANIFEST_ASSET_NAME = "dashboard-manifest.json";

    static bool compareVersions(const String& currentVersion, const String& candidateVersion) {
        if (currentVersion == candidateVersion) {
            return false;
        }

        int currentMajor = 0;
        int currentMinor = 0;
        int currentPatch = 0;
        int candidateMajor = 0;
        int candidateMinor = 0;
        int candidatePatch = 0;

        sscanf(currentVersion.c_str(), "%d.%d.%d", &currentMajor, &currentMinor, &currentPatch);
        sscanf(candidateVersion.c_str(), "%d.%d.%d", &candidateMajor, &candidateMinor, &candidatePatch);

        if (candidateMajor > currentMajor) return true;
        if (candidateMajor < currentMajor) return false;
        if (candidateMinor > currentMinor) return true;
        if (candidateMinor < currentMinor) return false;
        return candidatePatch > currentPatch;
    }

    static String sha256ToHex(const uint8_t* hash, size_t length) {
        String out;
        for (size_t i = 0; i < length; ++i) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", hash[i]);
            out += hex;
        }
        return out;
    }

    static bool calculateSha256(const uint8_t* data, size_t length, String& result) {
        mbedtls_sha256_context ctx;
        uint8_t digest[32];
        mbedtls_sha256_init(&ctx);
        if (mbedtls_sha256_starts_ret(&ctx, 0) != 0) {
            mbedtls_sha256_free(&ctx);
            return false;
        }
        if (mbedtls_sha256_update_ret(&ctx, data, length) != 0) {
            mbedtls_sha256_free(&ctx);
            return false;
        }
        if (mbedtls_sha256_finish_ret(&ctx, digest) != 0) {
            mbedtls_sha256_free(&ctx);
            return false;
        }
        mbedtls_sha256_free(&ctx);
        result = sha256ToHex(digest, sizeof(digest));
        return true;
    }

    static bool httpGetString(const String& url, String& payload) {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        if (!http.begin(client, url)) {
            return false;
        }
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.addHeader("Accept", "application/json");
        http.addHeader("User-Agent", "ESP32-Dashboard-Updater");
        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            return false;
        }
        payload = http.getString();
        http.end();
        return !payload.isEmpty();
    }

    static bool downloadFirmwareBinary(const String& url, const String& expectedSha256) {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        if (!http.begin(client, url)) {
            return false;
        }
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.addHeader("Accept", "application/octet-stream");
        http.addHeader("User-Agent", "ESP32-Dashboard-Updater");

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            return false;
        }

        size_t total = http.getSize();
        if (!Update.begin(total > 0 ? total : UPDATE_SIZE_UNKNOWN)) {
            http.end();
            return false;
        }

        WiFiClient* stream = http.getStreamPtr();
        uint8_t buffer[1024];
        size_t received = 0;
        String downloadedSha256;
        uint8_t digestBuffer[32];
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        if (mbedtls_sha256_starts_ret(&ctx, 0) != 0) {
            http.end();
            Update.abort();
            return false;
        }

        while (stream->available()) {
            size_t len = stream->readBytes(reinterpret_cast<char*>(buffer), sizeof(buffer));
            if (len == 0) {
                break;
            }
            if (mbedtls_sha256_update_ret(&ctx, buffer, len) != 0) {
                http.end();
                Update.abort();
                mbedtls_sha256_free(&ctx);
                return false;
            }
            if (Update.write(buffer, len) != len) {
                http.end();
                Update.abort();
                mbedtls_sha256_free(&ctx);
                return false;
            }
            received += len;
        }

        if (mbedtls_sha256_finish_ret(&ctx, digestBuffer) != 0) {
            http.end();
            Update.abort();
            mbedtls_sha256_free(&ctx);
            return false;
        }
        mbedtls_sha256_free(&ctx);

        http.end();

        if (!Update.end()) {
            return false;
        }

        if (!expectedSha256.isEmpty()) {
            downloadedSha256 = sha256ToHex(digestBuffer, sizeof(digestBuffer));
            if (downloadedSha256 != expectedSha256) {
                Serial.printf("[OTA] GitHub SHA256 mismatch. expected=%s actual=%s\n", expectedSha256.c_str(), downloadedSha256.c_str());
                return false;
            }
        }

        return Update.isFinished() && received > 0;
    }

    static String buildGithubReleaseCheckJson(bool updateAvailable, const String& version, const String& downloadUrl, const String& sha256) {
        JsonDocument doc;
        doc["status"] = updateAvailable ? "update_available" : "up_to_date";
        doc["current_version"] = FIRMWARE_VERSION;
        doc["latest_version"] = version;
        doc["download_url"] = downloadUrl;
        doc["sha256"] = sha256;
        String response;
        serializeJson(doc, response);
        return response;
    }

    static String findGitHubAssetDownloadUrl(const JsonArray& assets, const String& preferredName) {
        for (const JsonVariant asset : assets) {
            const char* name = asset["name"] | "";
            const char* url = asset["browser_download_url"] | "";
            if (name && url && (String(name) == preferredName || String(name).endsWith(preferredName))) {
                return String(url);
            }
        }
        if (!assets.isNull() && assets.size() > 0) {
            const JsonVariant firstAsset = assets[0];
            const char* url = firstAsset["browser_download_url"] | "";
            if (url) {
                return String(url);
            }
        }
        return String();
    }

    static String findGitHubAssetSha256(const JsonArray& assets, const String& preferredName) {
        for (const JsonVariant asset : assets) {
            const char* name = asset["name"] | "";
            const char* url = asset["browser_download_url"] | "";
            if (name && url && (String(name) == preferredName + ".sha256" || String(name).endsWith(preferredName + ".sha256"))) {
                String hash;
                if (httpGetString(String(url), hash) && !hash.isEmpty()) {
                    hash.trim();
                    return hash;
                }
            }
        }
        return String();
    }
}

// Mobile-first HTML rozhraní s velkými tlačítky pro ovládání z telefonu
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard Ovládání</title>
    <style>
        :root {
            --bg: #121418;
            --card-bg: #1e222b;
            --card-border: #2d3340;
            --text: #f0f2f5;
            --text-sub: #9ba1b0;
            --primary: #3b82f6;
            --primary-hover: #2563eb;
            --success: #10b981;
            --warning: #f59e0b;
            --danger: #ef4444;
            --active-border: #60a5fa;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
        body { background: var(--bg); color: var(--text); padding: 16px; display: flex; justify-content: center; }
        .container { width: 100%; max-width: 480px; display: flex; flex-direction: column; gap: 16px; }
        .header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 8px; border-bottom: 1px solid var(--card-border); }
        .header h1 { font-size: 1.25rem; font-weight: 700; }
        .badge { background: #1e3a5f; color: #93c5fd; padding: 4px 8px; border-radius: 6px; font-size: 0.75rem; font-weight: 600; }
        .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 16px; display: flex; flex-direction: column; gap: 12px; }
        .card-title { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-sub); font-weight: 700; }
        .btn-group { display: flex; flex-direction: column; gap: 10px; }
        .btn {
            background: #282e3c;
            color: var(--text);
            border: 1px solid var(--card-border);
            padding: 14px 16px;
            border-radius: 10px;
            font-size: 1rem;
            font-weight: 600;
            display: flex;
            justify-content: space-between;
            align-items: center;
            cursor: pointer;
            transition: all 0.2s ease;
            text-decoration: none;
            -webkit-tap-highlight-color: transparent;
        }
        .btn:active { transform: scale(0.98); }
        .btn.active {
            background: #1e3a8a;
            border-color: var(--active-border);
            color: #ffffff;
            box-shadow: 0 0 12px rgba(59, 130, 246, 0.4);
        }
        .btn-secondary { background: #1c2230; font-size: 0.9rem; padding: 12px; }
        .btn-danger { background: #3b1818; border-color: #5c2424; color: #fca5a5; }
        .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; font-size: 0.85rem; }
        .status-item { background: #171a21; padding: 10px; border-radius: 8px; border: 1px solid #232834; }
        .status-label { color: var(--text-sub); font-size: 0.75rem; }
        .status-value { font-weight: 600; margin-top: 4px; }
        .wifi-input { width: 100%; background: #171a21; color: var(--text); border: 1px solid var(--card-border); border-radius: 8px; padding: 12px; font-size: 1rem; }
        .source-grid { display: grid; grid-template-columns: 1fr 100px 90px; gap: 8px; align-items: center; }
        .source-grid label { color: var(--text-sub); font-size: 0.8rem; }
        .source-grid input { min-width: 0; }
        .status-value.ok { color: var(--success); }
        .toast {
            position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%);
            background: #3b82f6; color: white; padding: 10px 20px; border-radius: 30px;
            font-size: 0.9rem; font-weight: 600; opacity: 0; transition: opacity 0.3s ease;
            pointer-events: none; box-shadow: 0 4px 12px rgba(0,0,0,0.5); z-index: 100;
        }
        .toast.show { opacity: 1; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>E-Paper Dashboard</h1>
            <span class="badge" id="fwVer">v0.1.0</span>
        </div>

        <div class="card">
            <div class="card-title">Aktivní obrazovka displeje</div>
            <div class="btn-group" id="screensList">
                <button class="btn" onclick="activate('home')">🏠 Hlavní souhrn <span class="tag" id="tag-home"></span></button>
                <button class="btn" onclick="activate('solar')">☀️ Fotovoltaika (FVE) <span class="tag" id="tag-solar"></span></button>
                <button class="btn" onclick="activate('pool')">🏊 Bazén <span class="tag" id="tag-pool"></span></button>
                <button class="btn" onclick="activate('weather')">🌤️ Počasí <span class="tag" id="tag-weather"></span></button>
                <button class="btn" onclick="activate('diagnostics')">⚙️ Diagnostika <span class="tag" id="tag-diagnostics"></span></button>
            </div>
        </div>

        <div class="card">
            <div class="card-title">Ovládání displeje</div>
            <div class="btn-group">
                <button class="btn btn-secondary" onclick="triggerRefresh(false)">🔄 Obnovit displej (Partial)</button>
                <button class="btn btn-secondary" onclick="triggerRefresh(true)">✨ Plný refresh (Full)</button>
            </div>
        </div>

        <div class="card">
            <div class="card-title">Systémový stav</div>
            <div class="status-grid">
                <div class="status-item">
                    <div class="status-label">Aktivní displej</div>
                    <div class="status-value ok" id="statScreen">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Wi-Fi Signál</div>
                    <div class="status-value" id="statWifi">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">GoodWe Invertor</div>
                    <div class="status-value" id="statGoodwe">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">GoodWe aktualizace</div>
                    <div class="status-value" id="statGoodweUpdate">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">AZ Router</div>
                    <div class="status-value" id="statAzrouter">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">AZ aktualizace</div>
                    <div class="status-value" id="statAzrouterUpdate">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Volná paměť RAM</div>
                    <div class="status-value" id="statHeap">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Doba běhu (Uptime)</div>
                    <div class="status-value" id="statUptime">-</div>
                </div>
            </div>
        </div>

        <div class="card">
            <div class="card-title">Systém</div>
            <button class="btn btn-danger" onclick="confirmRestart()">⚠️ Restartovat ESP32</button>
        </div>

        <div class="card">
            <div class="card-title">Konfigurace Wi-Fi</div>
            <form onsubmit="saveWifi(event)">
                <button class="btn btn-secondary" type="button" onclick="scanWifi()">Vyhledat okolní Wi-Fi</button>
                <select class="wifi-input" id="wifiNetworks" onchange="selectWifi(this.value)">
                    <option value="">Vyber nalezenou síť</option>
                </select>
                <input class="wifi-input" id="wifiSsid" placeholder="Název Wi-Fi (SSID)" required>
                <input class="wifi-input" id="wifiPassword" type="password" placeholder="Heslo Wi-Fi">
                <button class="btn btn-secondary" type="submit">Uložit Wi-Fi a restartovat</button>
            </form>
        </div>

        <div class="card">
            <div class="card-title">Datové zdroje</div>
            <form onsubmit="saveSources(event)">
                <div class="source-grid">
                    <label>GoodWe host</label>
                    <input class="wifi-input" id="gwHost" placeholder="IP adresa" required>
                    <input class="wifi-input" id="gwPort" type="number" min="1" max="65535" placeholder="Port" required>
                    <label>Interval (s)</label>
                    <input class="wifi-input" id="gwInterval" type="number" min="1" max="3600" required>
                    <label><input id="gwEnabled" type="checkbox" checked> aktivní</label>
                    <label>AZRouter host</label>
                    <input class="wifi-input" id="azHost" placeholder="IP adresa" required>
                    <input class="wifi-input" id="azPort" type="number" min="1" max="65535" placeholder="Port" required>
                    <label>Interval (s)</label>
                    <input class="wifi-input" id="azInterval" type="number" min="1" max="3600" required>
                    <label><input id="azEnabled" type="checkbox" checked> aktivní</label>
                </div>
                <button class="btn btn-secondary" type="submit">Uložit zdroje a restartovat</button>
            </form>
        </div>

        <div class="card">
            <div class="card-title">Aktualizace firmware</div>
            <form action="/api/update" method="post" enctype="multipart/form-data">
                <input class="wifi-input" type="file" name="firmware" accept=".bin" required>
                <button class="btn btn-secondary" type="submit">Nahrát firmware a restartovat</button>
            </form>
        </div>
    </div>

    <div class="toast" id="toast">Provedeno</div>

    <script>
        function showToast(msg) {
            const t = document.getElementById('toast');
            t.innerText = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 2000);
        }

        let sourceConfigLoaded = false;

        async function updateStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                document.getElementById('fwVer').innerText = 'v' + data.firmware;
                document.getElementById('statScreen').innerText = data.screen;
                document.getElementById('statWifi').innerText = data.wifi.rssi + ' dBm';
                document.getElementById('statGoodwe').innerText = data.goodwe.available ? 'OK' : 'Offline';
                document.getElementById('statAzrouter').innerText = data.azrouter.available ? 'OK' : 'Offline';
                document.getElementById('statGoodweUpdate').innerText = formatDataAge(data.goodwe.lastUpdateAgeSeconds);
                document.getElementById('statAzrouterUpdate').innerText = formatDataAge(data.azrouter.lastUpdateAgeSeconds);
                document.getElementById('statHeap').innerText = Math.round(data.freeHeap / 1024) + ' KB';

                if (!sourceConfigLoaded && data.sources) {
                    const gw = data.sources.goodwe;
                    const az = data.sources.azrouter;
                    document.getElementById('gwHost').value = gw.host;
                    document.getElementById('gwPort').value = gw.port;
                    document.getElementById('gwInterval').value = gw.interval;
                    document.getElementById('gwEnabled').checked = gw.enabled;
                    document.getElementById('azHost').value = az.host;
                    document.getElementById('azPort').value = az.port;
                    document.getElementById('azInterval').value = az.interval;
                    document.getElementById('azEnabled').checked = az.enabled;
                    sourceConfigLoaded = true;
                }
                
                const s = data.uptime;
                const hrs = Math.floor(s / 3600);
                const mins = Math.floor((s % 3600) / 60);
                document.getElementById('statUptime').innerText = `${hrs}h ${mins}m ${s % 60}s`;

                // Update active buttons
                ['home', 'solar', 'pool', 'weather', 'diagnostics'].forEach(id => {
                    const el = document.querySelector(`button[onclick="activate('${id}')"]`);
                    if (el) {
                        if (id === data.screen) {
                            el.classList.add('active');
                        } else {
                            el.classList.remove('active');
                        }
                    }
                });
            } catch (e) {
                console.error("Chyba cteni statusu:", e);
            }
        }

        function formatDataAge(seconds) {
            return seconds === null ? 'Nikdy' : 'před ' + seconds + ' s';
        }

        async function activate(screenId) {
            try {
                showToast('Přepínám na: ' + screenId);
                const res = await fetch(`/api/screens/${screenId}/activate`, { method: 'POST' });
                if (res.ok) {
                    showToast('Obrazovka přepnuta');
                    updateStatus();
                }
            } catch (e) {
                showToast('Chyba spojení');
            }
        }

        async function triggerRefresh(full) {
            try {
                showToast(full ? 'Plný refresh spuštěn...' : 'Částečný refresh...');
                const endpoint = full ? '/api/display/full-refresh' : '/api/display/refresh';
                await fetch(endpoint, { method: 'POST' });
                showToast('Displej aktualizován');
            } catch (e) {
                showToast('Chyba refresh');
            }
        }

        function confirmRestart() {
            if (confirm("Opravdu chcete restartovat zařízení ESP32?")) {
                fetch('/api/system/restart', { method: 'POST' });
                showToast('ESP32 se restartuje...');
            }
        }

        async function saveWifi(event) {
            event.preventDefault();
            const body = new URLSearchParams({
                ssid: document.getElementById('wifiSsid').value,
                password: document.getElementById('wifiPassword').value
            });
            const response = await fetch('/api/wifi/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body
            });
            showToast(response.ok ? 'Wi-Fi uložena, zařízení se restartuje' : 'Wi-Fi se nepodařilo uložit');
        }

        async function saveSources(event) {
            event.preventDefault();
            const body = new URLSearchParams({
                gwHost: document.getElementById('gwHost').value,
                gwPort: document.getElementById('gwPort').value,
                gwInterval: document.getElementById('gwInterval').value,
                gwEnabled: document.getElementById('gwEnabled').checked ? '1' : '0',
                azHost: document.getElementById('azHost').value,
                azPort: document.getElementById('azPort').value,
                azInterval: document.getElementById('azInterval').value,
                azEnabled: document.getElementById('azEnabled').checked ? '1' : '0'
            });
            const response = await fetch('/api/config/sources', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body
            });
            showToast(response.ok ? 'Zdroje uloženy, zařízení se restartuje' : 'Zdroje se nepodařilo uložit');
        }

        async function scanWifi() {
            showToast('Vyhledávám okolní Wi-Fi...');
            try {
                const response = await fetch('/api/wifi/scan');
                if (!response.ok) throw new Error('Scan failed');
                const networks = await response.json();
                const select = document.getElementById('wifiNetworks');
                select.innerHTML = '<option value="">Vyber nalezenou síť</option>';
                networks.forEach(network => {
                    const option = document.createElement('option');
                    option.value = network.ssid;
                    option.textContent = network.ssid + ' (' + network.rssi + ' dBm)' + (network.secure ? ' 🔒' : '');
                    select.appendChild(option);
                });
                showToast(networks.length ? 'Sítě nalezeny' : 'Žádná síť nenalezena');
            } catch (error) {
                showToast('Vyhledání Wi-Fi se nepodařilo');
            }
        }

        function selectWifi(ssid) {
            if (ssid) document.getElementById('wifiSsid').value = ssid;
        }

        setInterval(updateStatus, 3000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

DashboardWebServer::DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager, const AppConfig& config)
    : _server(port), _dataModel(dataModel), _screenManager(screenManager), _config(config) {
}

void DashboardWebServer::begin() {
    setupRoutes();
    _server.begin();
    Serial.println("[WEB] HTTP REST server bezi na portu 80.");
}

void DashboardWebServer::loop() {
    Performance::webTick();
    Performance::Scope timing(Performance::WebService);
    _server.handleClient();
}

void DashboardWebServer::onScreenChange(ScreenChangeCallback callback) {
    _screenCallback = callback;
}

void DashboardWebServer::onRefresh(RefreshCallback callback) {
    _refreshCallback = callback;
}

void DashboardWebServer::onWifiConfig(WifiConfigCallback callback) {
    _wifiConfigCallback = callback;
}

void DashboardWebServer::onWifiScan(WifiScanCallback callback) {
    _wifiScanCallback = callback;
}

void DashboardWebServer::onSourceConfig(SourceConfigCallback callback) {
    _sourceConfigCallback = callback;
}

void DashboardWebServer::setupRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    _server.on("/api/screens", HTTP_GET, [this]() { handleApiScreens(); });

    _server.on("/api/screens/home/activate", HTTP_POST, [this]() { handleApiActivateScreen("home"); });
    _server.on("/api/screens/solar/activate", HTTP_POST, [this]() { handleApiActivateScreen("solar"); });
    _server.on("/api/screens/pool/activate", HTTP_POST, [this]() { handleApiActivateScreen("pool"); });
    _server.on("/api/screens/weather/activate", HTTP_POST, [this]() { handleApiActivateScreen("weather"); });
    _server.on("/api/screens/diagnostics/activate", HTTP_POST, [this]() { handleApiActivateScreen("diagnostics"); });

    _server.on("/api/display/refresh", HTTP_POST, [this]() { handleApiRefresh(false); });
    _server.on("/api/display/full-refresh", HTTP_POST, [this]() { handleApiRefresh(true); });
    _server.on("/api/system/restart", HTTP_POST, [this]() { handleApiRestart(); });
    _server.on("/api/wifi/config", HTTP_POST, [this]() { handleApiWifiConfig(); });
    _server.on("/api/wifi/scan", HTTP_GET, [this]() { handleApiWifiScan(); });
    _server.on("/api/config/sources", HTTP_POST, [this]() { handleApiSourceConfig(); });
    _server.on("/api/update/check", HTTP_GET, [this]() { handleApiCheckForUpdate(); });
    _server.on("/api/update/github", HTTP_POST, [this]() { handleApiGithubUpdate(); });
    _server.on("/api/update", HTTP_POST,
                [this]() { handleApiUpdateComplete(); },
                [this]() { handleApiUpdateUpload(); });
}

void DashboardWebServer::handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

void DashboardWebServer::handleApiStatus() {
    Performance::Scope timing(Performance::StatusHandler);
    _dataModel.updateSystemMetrics();
    JsonDocument doc;

    doc["firmware"] = FIRMWARE_VERSION;
    doc["uptime"] = _dataModel.system.uptimeSeconds;
    doc["freeHeap"] = _dataModel.system.freeHeapBytes;
    doc["screen"] = _screenManager.getActiveScreenId();
    JsonObject perf = doc["performance"].to<JsonObject>();
    perf["uptimeMs"] = millis();
    for (int i = 0; i < Performance::Count; ++i) {
        const auto metric = static_cast<Performance::Metric>(i);
        const auto& value = Performance::stats(metric);
        JsonObject item = perf[Performance::name(metric)].to<JsonObject>();
        item["count"] = value.count;
        item["avgMs"] = value.count ? (double)value.totalMs / value.count : 0;
        item["maxMs"] = value.maxMs;
        item["lastMs"] = value.lastMs;
        item["lastEndMs"] = value.lastEndMs;
    }

    JsonObject wifiObj = doc["wifi"].to<JsonObject>();
    wifiObj["connected"] = _dataModel.system.wifiConnected;
    wifiObj["rssi"] = _dataModel.system.wifiRssi;
    wifiObj["ip"] = _dataModel.system.ipAddress;

    JsonObject gwObj = doc["goodwe"].to<JsonObject>();
    gwObj["available"] = _dataModel.solar.status.available;
    gwObj["lastUpdateAgeSeconds"] = _dataModel.solar.status.lastSuccessMs == 0
        ? serialized(NULL)
        : serialized((millis() - _dataModel.solar.status.lastSuccessMs) / 1000);

    JsonObject azObj = doc["azrouter"].to<JsonObject>();
    azObj["available"] = _dataModel.azrouter.status.available;
    azObj["lastUpdateAgeSeconds"] = _dataModel.azrouter.status.lastSuccessMs == 0
        ? serialized(NULL)
        : serialized((millis() - _dataModel.azrouter.status.lastSuccessMs) / 1000);

    JsonObject sourcesObj = doc["sources"].to<JsonObject>();
    JsonObject sourceGw = sourcesObj["goodwe"].to<JsonObject>();
    sourceGw["enabled"] = _config.goodwe.enabled;
    sourceGw["host"] = _config.goodwe.host;
    sourceGw["port"] = _config.goodwe.port;
    sourceGw["interval"] = _config.goodwe.pollIntervalSeconds;
    JsonObject sourceAz = sourcesObj["azrouter"].to<JsonObject>();
    sourceAz["enabled"] = _config.azrouter.enabled;
    sourceAz["host"] = _config.azrouter.host;
    sourceAz["port"] = _config.azrouter.port;
    sourceAz["interval"] = _config.azrouter.pollIntervalSeconds;

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void DashboardWebServer::handleApiScreens() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto* s : _screenManager.getAllScreens()) {
        if (!s) continue;
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = s->getId();
        obj["title"] = s->getTitle();
        obj["active"] = (s->getId() == _screenManager.getActiveScreenId());
    }

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void DashboardWebServer::handleApiActivateScreen(const String& screenId) {
    Serial.printf("[WEB][%lu ms] Stisk tlacitka: prepnuti obrazovky na '%s'\n",
                  millis(), screenId.c_str());
    if (_screenManager.activateScreen(screenId)) {
        _dataModel.system.currentScreenId = screenId;
        if (_screenCallback) {
            _screenCallback(screenId);
        }
        _server.send(200, "application/json", "{\"status\":\"ok\",\"screen\":\"" + screenId + "\"}");
    } else {
        _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Screen not found\"}");
    }
}

void DashboardWebServer::handleApiRefresh(bool full) {
    Serial.printf("[WEB][%lu ms] Stisk tlacitka: %s refresh\n",
                  millis(), full ? "FULL" : "PARTIAL");
    if (_refreshCallback) {
        _refreshCallback(full);
    }
    _server.send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"" + String(full ? "full" : "partial") + "\"}");
}

void DashboardWebServer::handleApiRestart() {
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Restarting in 1s\"}");
    delay(1000);
    ESP.restart();
}

void DashboardWebServer::handleApiWifiConfig() {
    if (!_server.hasArg("ssid") || !_server.hasArg("password") || _server.arg("ssid").isEmpty()) {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID is required\"}");
        return;
    }

    if (_wifiConfigCallback) {
        _server.send(200, "application/json", "{\"status\":\"saved\"}");
        _wifiConfigCallback(_server.arg("ssid"), _server.arg("password"));
        return;
    }

    _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Wi-Fi configuration unavailable\"}");
}

void DashboardWebServer::handleApiWifiScan() {
    if (_wifiScanCallback) {
        _server.send(200, "application/json", _wifiScanCallback());
        return;
    }

    _server.send(503, "application/json", "[]");
}

void DashboardWebServer::handleApiSourceConfig() {
    const char* required[] = {"gwHost", "gwPort", "gwInterval", "azHost", "azPort", "azInterval"};
    for (const char* name : required) {
        if (!_server.hasArg(name) || _server.arg(name).isEmpty()) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing source setting\"}");
            return;
        }
    }

    long gwPort = _server.arg("gwPort").toInt();
    long azPort = _server.arg("azPort").toInt();
    long gwInterval = _server.arg("gwInterval").toInt();
    long azInterval = _server.arg("azInterval").toInt();
    if (gwPort < 1 || gwPort > 65535 || azPort < 1 || azPort > 65535 ||
        gwInterval < 1 || gwInterval > 3600 || azInterval < 1 || azInterval > 3600) {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid source setting\"}");
        return;
    }

    if (_sourceConfigCallback) {
        GoodWeConfig goodwe;
        goodwe.enabled = _server.arg("gwEnabled") == "1";
        goodwe.host = _server.arg("gwHost");
        goodwe.port = static_cast<uint16_t>(gwPort);
        goodwe.pollIntervalSeconds = static_cast<uint32_t>(gwInterval);

        AZRouterConfig azrouter;
        azrouter.enabled = _server.arg("azEnabled") == "1";
        azrouter.host = _server.arg("azHost");
        azrouter.port = static_cast<uint16_t>(azPort);
        azrouter.pollIntervalSeconds = static_cast<uint32_t>(azInterval);

        _server.send(200, "application/json", "{\"status\":\"saved\"}");
        _sourceConfigCallback(goodwe, azrouter);
        return;
    }

    _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Source configuration unavailable\"}");
}

void DashboardWebServer::handleApiCheckForUpdate() {
    String payload;
    if (!httpGetString(String(GITHUB_RELEASE_API_URL), payload)) {
        _server.send(200, "application/json", buildGithubReleaseCheckJson(false, FIRMWARE_VERSION, "", ""));
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        _server.send(200, "application/json", buildGithubReleaseCheckJson(false, FIRMWARE_VERSION, "", ""));
        return;
    }

    const char* tagName = doc["tag_name"] | "";
    if (tagName[0] == '\0') {
        _server.send(200, "application/json", buildGithubReleaseCheckJson(false, FIRMWARE_VERSION, "", ""));
        return;
    }

    String latestVersion = String(tagName);
    latestVersion.replace("v", "");

    JsonArray assets = doc["assets"].as<JsonArray>();
    String downloadUrl = findGitHubAssetDownloadUrl(assets, String(GITHUB_FIRMWARE_ASSET_NAME));
    String sha256 = findGitHubAssetSha256(assets, String(GITHUB_FIRMWARE_ASSET_NAME));

    if (downloadUrl.isEmpty()) {
        _server.send(200, "application/json", buildGithubReleaseCheckJson(false, FIRMWARE_VERSION, "", ""));
        return;
    }

    bool updateAvailable = compareVersions(FIRMWARE_VERSION, latestVersion);
    _server.send(200, "application/json", buildGithubReleaseCheckJson(updateAvailable, latestVersion, downloadUrl, sha256));
}

void DashboardWebServer::handleApiGithubUpdate() {
    String url = _server.hasArg("url") ? _server.arg("url") : "";
    String sha256 = _server.hasArg("sha256") ? _server.arg("sha256") : "";
    if (url.isEmpty()) {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing GitHub download URL\"}");
        return;
    }

    if (!downloadFirmwareBinary(url, sha256)) {
        Serial.println("[OTA] GitHub binary download failed");
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Firmware download or validation failed\"}");
        return;
    }

    Serial.println("[OTA] GitHub firmware installed, restarting...");
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"GitHub firmware updated; restarting\"}");
    delay(500);
    ESP.restart();
}

void DashboardWebServer::handleApiUpdateUpload() {
    _otaManager.handleUpload(_server.upload());
}

void DashboardWebServer::handleApiUpdateComplete() {
    if (!_otaManager.finish()) {
        Serial.printf("[OTA] Upload failed: %s\n", _otaManager.error().c_str());
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"" + _otaManager.error() + "\"}");
        return;
    }

    Serial.println("[OTA] Firmware upload succeeded, restarting...");
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Firmware updated; restarting\"}");
    delay(500);
    ESP.restart();
}
