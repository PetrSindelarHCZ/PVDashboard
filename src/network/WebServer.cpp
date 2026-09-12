#include "WebServer.h"
#include <ArduinoJson.h>
#include "../../include/Version.h"

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
                    <div class="status-label">AZ Router</div>
                    <div class="status-value" id="statAzrouter">-</div>
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
    </div>

    <div class="toast" id="toast">Provedeno</div>

    <script>
        function showToast(msg) {
            const t = document.getElementById('toast');
            t.innerText = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 2000);
        }

        async function updateStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                document.getElementById('fwVer').innerText = 'v' + data.firmware;
                document.getElementById('statScreen').innerText = data.screen;
                document.getElementById('statWifi').innerText = data.wifi.rssi + ' dBm';
                document.getElementById('statGoodwe').innerText = data.goodwe.available ? 'OK' : 'Offline';
                document.getElementById('statAzrouter').innerText = data.azrouter.available ? 'OK' : 'Offline';
                document.getElementById('statHeap').innerText = Math.round(data.freeHeap / 1024) + ' KB';
                
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

        setInterval(updateStatus, 3000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

DashboardWebServer::DashboardWebServer(uint16_t port, DataModel& dataModel, ScreenManager& screenManager)
    : _server(port), _dataModel(dataModel), _screenManager(screenManager) {
}

void DashboardWebServer::begin() {
    setupRoutes();
    _server.begin();
    Serial.println("[WEB] HTTP REST server bezi na portu 80.");
}

void DashboardWebServer::loop() {
    _server.handleClient();
}

void DashboardWebServer::onScreenChange(ScreenChangeCallback callback) {
    _screenCallback = callback;
}

void DashboardWebServer::onRefresh(RefreshCallback callback) {
    _refreshCallback = callback;
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
}

void DashboardWebServer::handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

void DashboardWebServer::handleApiStatus() {
    _dataModel.updateSystemMetrics();
    JsonDocument doc;

    doc["firmware"] = FIRMWARE_VERSION;
    doc["uptime"] = _dataModel.system.uptimeSeconds;
    doc["freeHeap"] = _dataModel.system.freeHeapBytes;
    doc["screen"] = _screenManager.getActiveScreenId();

    JsonObject wifiObj = doc["wifi"].to<JsonObject>();
    wifiObj["connected"] = _dataModel.system.wifiConnected;
    wifiObj["rssi"] = _dataModel.system.wifiRssi;
    wifiObj["ip"] = _dataModel.system.ipAddress;

    JsonObject gwObj = doc["goodwe"].to<JsonObject>();
    gwObj["available"] = _dataModel.solar.status.available;

    JsonObject azObj = doc["azrouter"].to<JsonObject>();
    azObj["available"] = _dataModel.azrouter.status.available;

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
