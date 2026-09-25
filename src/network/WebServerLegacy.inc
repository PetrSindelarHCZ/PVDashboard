#include "WebServer.h"
#include "../diagnostics/Performance.h"
#include "../config/ConfigBackup.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include "../../include/Version.h"
#include "../../include/FirmwareLimits.h"

namespace {
    static const char* GITHUB_LATEST_DOWNLOAD_BASE = "https://github.com/PetrSindelarHCZ/PVDashboard/releases/latest/download/";
    static const char* GITHUB_FIRMWARE_ASSET_NAME = "firmware.bin";
    static const char* GITHUB_MANIFEST_ASSET_NAME = "dashboard-manifest.json";

    static bool parseVersion(const String& version, int parts[4]) {
        parts[0] = parts[1] = parts[2] = parts[3] = 0;
        const int count = sscanf(version.c_str(), "%d.%d.%d.%d",
                                 &parts[0], &parts[1], &parts[2], &parts[3]);
        return count == 3 || count == 4;
    }

    static bool compareVersions(const String& currentVersion, const String& candidateVersion) {
        if (currentVersion == candidateVersion) return false;

        int current[4];
        int candidate[4];
        if (!parseVersion(currentVersion, current) || !parseVersion(candidateVersion, candidate)) {
            Serial.printf("[OTA] Invalid version format: current=%s candidate=%s\n",
                          currentVersion.c_str(), candidateVersion.c_str());
            return false;
        }

        for (size_t i = 0; i < 4; ++i) {
            if (candidate[i] > current[i]) return true;
            if (candidate[i] < current[i]) return false;
        }
        return false;
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

    static bool normalizeSha256(String value, String& normalized) {
        value.trim();
        const int separator = value.indexOf(' ');
        if (separator > 0) value = value.substring(0, separator);
        if (value.length() != 64) return false;
        value.toLowerCase();
        for (size_t i = 0; i < value.length(); ++i) {
            const char c = value[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        }
        normalized = value;
        return true;
    }

    static bool downloadFirmwareBinary(const String& url, const String& expectedSha256) {
        String normalizedExpectedSha256;
        if (!normalizeSha256(expectedSha256, normalizedExpectedSha256)) {
            Serial.println("[OTA] Missing or invalid GitHub SHA-256");
            return false;
        }

        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        if (!http.begin(client, url)) return false;
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setTimeout(10000);
        http.addHeader("Accept", "application/octet-stream");
        http.addHeader("User-Agent", "ESP32-Dashboard-Updater");

        const int code = http.GET();
        if (code != HTTP_CODE_OK) {
            http.end();
            return false;
        }

        const int total = http.getSize();
        if (total > 0 && static_cast<size_t>(total) > FirmwareLimits::MaxImageBytes) {
            Serial.printf("[OTA] GitHub firmware too large: %d > %u\n",
                          total, (unsigned int)FirmwareLimits::MaxImageBytes);
            http.end();
            return false;
        }
        if (!Update.begin(total > 0 ? static_cast<size_t>(total) : FirmwareLimits::MaxImageBytes)) {
            http.end();
            return false;
        }

        WiFiClient* stream = http.getStreamPtr();
        uint8_t buffer[1024];
        size_t received = 0;
        uint8_t digestBuffer[32];
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        if (mbedtls_sha256_starts_ret(&ctx, 0) != 0) {
            http.end();
            Update.abort();
            mbedtls_sha256_free(&ctx);
            return false;
        }

        bool streamOk = true;
        unsigned long lastDataAt = millis();
        while (http.connected() && (total < 0 || received < static_cast<size_t>(total))) {
            const size_t available = stream->available();
            if (available == 0) {
                if (millis() - lastDataAt > 10000UL) {
                    streamOk = false;
                    break;
                }
                delay(1);
                continue;
            }

            const size_t requested = available < sizeof(buffer) ? available : sizeof(buffer);
            const size_t len = stream->readBytes(reinterpret_cast<char*>(buffer), requested);
            if (len == 0) continue;
            lastDataAt = millis();

            if (received + len > FirmwareLimits::MaxImageBytes ||
                mbedtls_sha256_update_ret(&ctx, buffer, len) != 0 ||
                Update.write(buffer, len) != len) {
                streamOk = false;
                break;
            }
            received += len;
        }

        if (total > 0 && received != static_cast<size_t>(total)) streamOk = false;

        bool hashFinished = false;
        if (streamOk) hashFinished = mbedtls_sha256_finish_ret(&ctx, digestBuffer) == 0;
        mbedtls_sha256_free(&ctx);
        http.end();

        if (!streamOk || !hashFinished || received == 0) {
            Update.abort();
            return false;
        }

        const String downloadedSha256 = sha256ToHex(digestBuffer, sizeof(digestBuffer));
        if (downloadedSha256 != normalizedExpectedSha256) {
            Serial.printf("[OTA] GitHub SHA256 mismatch. expected=%s actual=%s\n",
                          normalizedExpectedSha256.c_str(), downloadedSha256.c_str());
            Update.abort();
            return false;
        }

        if (!Update.end(total <= 0)) {
            Update.abort();
            return false;
        }
        return Update.isFinished();
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
            if (name && url && String(name) == preferredName) return String(url);
        }
        return String();
    }

    static String findGitHubAssetSha256(const JsonArray& assets, const String& preferredName) {
        for (const JsonVariant asset : assets) {
            const char* name = asset["name"] | "";
            const char* url = asset["browser_download_url"] | "";
            if (name && url && String(name) == preferredName + ".sha256") {
                String hashPayload;
                String normalizedHash;
                if (httpGetString(String(url), hashPayload) &&
                    normalizeSha256(hashPayload, normalizedHash)) {
                    return normalizedHash;
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
        .card-title-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
        .btn.btn-inline { width: auto; min-height: 0; padding: 7px 11px; font-size: 0.78rem; line-height: 1.2; white-space: nowrap; }
        .field-help { color: var(--text-sub); font-size: 0.75rem; line-height: 1.35; }
        .timezone-picker { position: relative; }
        .timezone-results { position: absolute; left: 0; right: 0; top: calc(100% + 6px); z-index: 60; max-height: 300px; overflow-y: auto; background: #171a21; border: 1px solid var(--card-border); border-radius: 9px; box-shadow: 0 10px 24px rgba(0,0,0,.45); padding: 5px; }
        .timezone-result { width: 100%; border: 0; border-radius: 7px; background: transparent; color: var(--text); padding: 9px 10px; text-align: left; font-size: .88rem; line-height: 1.3; cursor: pointer; }
        .timezone-result:hover, .timezone-result:focus { background: #252b37; outline: none; }
        .timezone-no-result { color: var(--text-sub); padding: 10px; font-size: .82rem; }
        .source-grid.source-grid-wide { grid-template-columns: 145px minmax(260px, 1fr) 115px; }
        .source-device-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 14px; }
        .source-device-card { background: #171a21; border: 1px solid #2b3240; border-radius: 10px; padding: 14px; display: flex; flex-direction: column; gap: 12px; min-width: 0; }
        .source-device-header { display: flex; align-items: center; justify-content: space-between; gap: 12px; padding-bottom: 10px; border-bottom: 1px solid #282e3a; }
        .source-device-title { font-size: 1rem; font-weight: 700; }
        .source-enabled { display: inline-flex; align-items: center; gap: 7px; color: var(--text-sub); font-size: .82rem; white-space: nowrap; }
        .source-enabled input { width: 18px; height: 18px; accent-color: var(--primary); }
        .source-field { display: flex; flex-direction: column; gap: 6px; min-width: 0; }
        .source-field label { color: var(--text-sub); font-size: .78rem; }
        .source-field-row { display: grid; grid-template-columns: minmax(100px, .7fr) minmax(150px, 1fr); gap: 10px; }
        .source-save { margin-top: 14px; }
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
        .btn:disabled { opacity: 0.45; cursor: wait; transform: none; }
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
        .system-grid { display: grid; grid-template-columns: 1fr; gap: 10px; margin-bottom: 10px; }
        .system-grid label { color: var(--text-sub); font-size: 0.8rem; }
        .system-field { display: flex; flex-direction: column; gap: 6px; }
        .status-value.ok { color: var(--success); }
        .btn-primary { background: #1e3a8a; border-color: var(--active-border); }
        .update-message { color: var(--text-sub); font-size: 0.85rem; line-height: 1.4; }
        .update-message.ok { color: var(--success); }
        .update-message.error { color: #fca5a5; }
        [hidden] { display: none !important; }
        .toast {
            position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%);
            background: #3b82f6; color: white; padding: 10px 20px; border-radius: 30px;
            font-size: 0.9rem; font-weight: 600; opacity: 0; transition: opacity 0.3s ease;
            pointer-events: none; box-shadow: 0 4px 12px rgba(0,0,0,0.5); z-index: 100;
        }
        .toast.show { opacity: 1; }
    
        /* responsive-shell-v1 */
        .container { max-width: 1380px; }
        .app-shell { display: grid; grid-template-columns: 220px minmax(0, 1fr); gap: 18px; align-items: start; }
        .app-nav { position: sticky; top: 16px; display: flex; flex-direction: column; gap: 8px; padding: 10px; background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; }
        .app-nav-button { appearance: none; border: 1px solid transparent; border-radius: 9px; background: transparent; color: var(--text-sub); padding: 12px 14px; display: flex; align-items: center; gap: 10px; font-size: 0.92rem; font-weight: 650; text-align: left; cursor: pointer; }
        .app-nav-button:hover { background: #252b37; color: var(--text); }
        .app-nav-button.active { background: #1e3a5f; border-color: var(--active-border); color: #dbeafe; }
        .app-nav-icon { width: 1.5rem; text-align: center; font-size: 1.05rem; }
        .app-content { min-width: 0; }
        .app-view { min-width: 0; }
        .section-heading { margin: 2px 0 14px; }
        .section-heading h2 { font-size: 1.25rem; margin-bottom: 4px; }
        .section-heading p { color: var(--text-sub); font-size: 0.86rem; line-height: 1.45; }
        .section-grid {
            display: grid;
            grid-template-columns: repeat(2, minmax(0, 1fr));
            gap: 16px;
            align-items: stretch;
        }
        .section-grid > .card {
            min-width: 0;
            height: 100%;
        }
        .section-grid > .wide-card { grid-column: 1 / -1; }
        #screensList { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 10px; }
        #screensList > .source-grid { grid-column: 1 / -1; }
        .view-system .status-grid { grid-template-columns: repeat(4, minmax(0, 1fr)); }
        .view-settings .card form { width: 100%; }

        @media (max-width: 1099px) {
            .container { max-width: 980px; }
            .app-shell { display: block; }
            .app-nav { position: sticky; top: 0; z-index: 20; display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); margin-bottom: 16px; padding: 8px; }
            .app-nav-button { justify-content: center; text-align: center; }
            .section-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
            .view-system .status-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        }

        @media (max-width: 699px) {
            body { padding: 10px 10px 88px; }
            .container { max-width: none; gap: 12px; }
            .header { padding: 4px 2px 10px; }
            .header h1 { font-size: 1.05rem; }
            .app-shell { display: block; }
            .app-nav { position: fixed; left: 0; right: 0; bottom: 0; top: auto; z-index: 1000; margin: 0; border-radius: 0; border-left: 0; border-right: 0; border-bottom: 0; grid-template-columns: repeat(3, minmax(0, 1fr)); padding: 7px 8px calc(7px + env(safe-area-inset-bottom)); box-shadow: 0 -4px 18px rgba(0,0,0,.35); }
            .app-nav-button { min-width: 0; padding: 7px 4px; flex-direction: column; gap: 2px; font-size: .70rem; line-height: 1.1; }
            .app-nav-icon { width: auto; font-size: 1.1rem; }
            .section-heading { margin: 2px 2px 12px; }
            .section-heading h2 { font-size: 1.12rem; }
            .section-heading p { font-size: .80rem; }
            .section-grid { grid-template-columns: 1fr; gap: 12px; }
            .section-grid > .wide-card { grid-column: auto; }
            .card { padding: 14px; border-radius: 10px; }
            #screensList { grid-template-columns: 1fr; }
            #screensList > .source-grid { grid-column: auto; }
            .source-grid, .source-grid-wide { grid-template-columns: 1fr; }
            .source-device-grid { grid-template-columns: 1fr; }
            .source-field-row { grid-template-columns: 1fr; }
            .source-grid > span:empty { display: none; }
            .card-title-row { align-items: flex-start; }
            .btn.btn-inline { flex: 0 0 auto; min-height: 0; padding: 7px 9px; }
            .source-grid > label { margin-top: 4px; }
            .status-grid, .view-system .status-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 8px; }
            .btn { min-height: 48px; }
        }
</style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>E-Paper Dashboard</h1>
            <span class="badge" id="fwVer">v…</span>
        </div>

        <div class="card">
            <div class="card-title-row">
                <div class="card-title">Aktivní obrazovka displeje</div>
                <button class="btn btn-secondary btn-inline" type="button" data-display-action onclick="triggerRefresh(true)">✨ Full refresh</button>
            </div>
            <div class="btn-group" id="screensList">
                <button class="btn" data-display-action onclick="activate('home')">🏠 Hlavní souhrn <span class="tag" id="tag-home"></span></button>
                <button class="btn" data-display-action onclick="activate('solar')">☀️ Fotovoltaika (FVE) <span class="tag" id="tag-solar"></span></button>
                <button class="btn" data-display-action onclick="activate('pool')">🏊 Bazén <span class="tag" id="tag-pool"></span></button>
                <button class="btn" data-display-action onclick="activate('weather')">🌤️ Počasí <span class="tag" id="tag-weather"></span></button>
                <div class="source-grid">
                    <label for="weatherForecastDay">Hodinová předpověď</label>
                    <select class="wifi-input" id="weatherForecastDay">
                        <option value="0">1. den</option>
                        <option value="1">2. den</option>
                        <option value="2">3. den</option>
                        <option value="3">4. den</option>
                    </select>
                    <button class="btn btn-secondary" data-display-action onclick="activate('weather-hourly-' + document.getElementById('weatherForecastDay').value)">Zobrazit hodiny</button>
                </div>
                <button class="btn" data-display-action onclick="activate('diagnostics')">⚙️ Diagnostika <span class="tag" id="tag-diagnostics"></span></button>
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
                    <div class="status-label">Stav překreslení</div>
                    <div class="status-value" id="statDisplayState">-</div>
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
                    <div class="status-label">Počasí</div>
                    <div class="status-value" id="statWeather">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Počasí aktualizace</div>
                    <div class="status-value" id="statWeatherUpdate">-</div>
                </div>                <div class="status-item">
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
            <form onsubmit="saveSystem(event)">
                <div class="system-grid">
                    <div class="system-field">
                        <label for="systemHostname">Hostname</label>
                        <input class="wifi-input" id="systemHostname" maxlength="32" required>
                    </div>
                    <div class="system-field">
                        <label for="systemNtp">NTP server</label>
                        <input class="wifi-input" id="systemNtp" maxlength="253" required>
                    </div>
                    <div class="system-field">
                        <label for="systemTimezoneSearch">Časové pásmo</label>
                        <div class="timezone-picker">
                            <input class="wifi-input" id="systemTimezoneSearch" type="search" autocomplete="off"
                                   placeholder="Hledat město, zemi nebo UTC offset…"
                                   onfocus="filterTimezones(this.value, false)"
                                   oninput="filterTimezones(this.value, true)">
                            <div class="timezone-results" id="systemTimezoneResults" hidden></div>
                        <select id="systemTimezone" required hidden>
                            <optgroup label="Evropa — automatický letní/zimní čas">
                                <option value="GMT0BST,M3.5.0/1,M10.5.0">UTC+00 — Londýn, Velká Británie / Dublin, Irsko (GMT/BST)</option>
                                <option value="CET-1CEST,M3.5.0,M10.5.0/3">UTC+01 — Praha, Česko / Bratislava, Slovensko / Berlín, Německo / Vídeň, Rakousko (CET/CEST)</option>
                                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">UTC+02 — Helsinky, Finsko / Tallinn, Estonsko / Riga, Lotyšsko / Vilnius, Litva (EET/EEST)</option>
                            </optgroup>
                            <optgroup label="UTC pásma — reprezentativní místo / pevný posun">
                                <option value="UTC12">UTC−12 — Baker Island, USA</option>
                                <option value="UTC11">UTC−11 — Pago Pago, American Samoa</option>
                                <option value="UTC10">UTC−10 — Honolulu, Hawaii, USA</option>
                                <option value="UTC9:30">UTC−09:30 — Marquesas Islands, Francouzská Polynésie</option>
                                <option value="UTC9">UTC−09 — Gambier Islands, Francouzská Polynésie</option>
                                <option value="UTC8">UTC−08 — Pitcairn Islands</option>
                                <option value="UTC7">UTC−07 — Phoenix, Arizona, USA</option>
                                <option value="UTC6">UTC−06 — Guatemala City, Guatemala</option>
                                <option value="UTC5">UTC−05 — Bogotá, Colombia</option>
                                <option value="UTC4">UTC−04 — Santo Domingo, Dominikánská republika</option>
                                <option value="UTC3:30">UTC−03:30 — St. John's, Newfoundland, Kanada</option>
                                <option value="UTC3">UTC−03 — Buenos Aires, Argentina</option>
                                <option value="UTC2">UTC−02 — South Georgia and South Sandwich Islands</option>
                                <option value="UTC1">UTC−01 — Cabo Verde</option>
                                <option value="UTC0">UTC±00 — Reykjavík, Iceland</option>
                                <option value="UTC-1">UTC+01 — Lagos, Nigeria</option>
                                <option value="UTC-2">UTC+02 — Johannesburg, South Africa</option>
                                <option value="UTC-3">UTC+03 — Moskva, Rusko / Rijád, Saúdská Arábie</option>
                                <option value="UTC-3:30">UTC+03:30 — Teherán, Írán</option>
                                <option value="UTC-4">UTC+04 — Dubaj, Spojené arabské emiráty</option>
                                <option value="UTC-4:30">UTC+04:30 — Kábul, Afghánistán</option>
                                <option value="UTC-5">UTC+05 — Karáčí, Pákistán</option>
                                <option value="UTC-5:30">UTC+05:30 — Dillí, Bombaj, Indie</option>
                                <option value="UTC-5:45">UTC+05:45 — Káthmándú, Nepál</option>
                                <option value="UTC-6">UTC+06 — Dháka, Bangladéš</option>
                                <option value="UTC-6:30">UTC+06:30 — Yangon, Myanmar</option>
                                <option value="UTC-7">UTC+07 — Bangkok, Thajsko / Jakarta, Indonésie</option>
                                <option value="UTC-8">UTC+08 — Singapur / Peking, Čína / Perth, Austrálie</option>
                                <option value="UTC-8:45">UTC+08:45 — Eucla, Západní Austrálie</option>
                                <option value="UTC-9">UTC+09 — Tokio, Japonsko / Soul, Jižní Korea</option>
                                <option value="UTC-9:30">UTC+09:30 — Darwin, Austrálie</option>
                                <option value="UTC-10">UTC+10 — Brisbane, Austrálie</option>
                                <option value="UTC-10:30">UTC+10:30 — Lord Howe Island, Austrálie</option>
                                <option value="UTC-11">UTC+11 — Nouméa, Nová Kaledonie</option>
                                <option value="UTC-12">UTC+12 — Suva, Fidži</option>
                                <option value="UTC-12:45">UTC+12:45 — Chatham Islands, Nový Zéland</option>
                                <option value="UTC-13">UTC+13 — Apia, Samoa</option>
                                <option value="UTC-14">UTC+14 — Kiritimati, Kiribati</option>
                            </optgroup>
                        </select>
                        </div>
                        <div class="field-help">Praha, Londýn a Helsinky používají automatický letní/zimní čas. Ostatní položky představují pevný UTC posun daného pásma.</div>
                    </div>
                </div>
                <button class="btn btn-secondary" type="submit">Uložit systém a restartovat</button>
            </form>
            <div class="btn-group">
                <button class="btn btn-danger" onclick="confirmRestart()">⚠️ Restartovat ESP32</button>
                <button class="btn btn-danger" onclick="confirmFactoryReset()">🗑️ Obnovit tovární nastavení</button>
                <a class="btn btn-secondary" href="/api/config/export" download="pvdashboard-config.yaml">⬇️ Exportovat konfiguraci YAML</a>
                <input class="wifi-input" id="configImportFile" type="file" accept=".yaml,.yml,application/yaml,text/yaml">
                <button class="btn btn-secondary" onclick="importConfiguration()">⬆️ Importovat konfiguraci YAML</button>
            </div>
            <div class="update-message">Tovární reset odstraní Wi-Fi, datové zdroje i nastavení počasí. Po restartu se zařízení může spustit v síti Dashboard-Setup.</div>
            <div class="update-message">Export obsahuje heslo Wi-Fi v čitelné podobě. Záložní soubor uchovávejte bezpečně.</div>
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
                <div class="source-device-grid">
                    <section class="source-device-card">
                        <div class="source-device-header">
                            <div class="source-device-title">☀️ GoodWe</div>
                            <label class="source-enabled"><input id="gwEnabled" type="checkbox" checked> Aktivní</label>
                        </div>
                        <div class="source-field">
                            <label for="gwHost">Host / IP adresa</label>
                            <input class="wifi-input" id="gwHost" placeholder="např. 192.168.1.50">
                        </div>
                        <div class="source-field-row">
                            <div class="source-field">
                                <label for="gwPort">Port</label>
                                <input class="wifi-input" id="gwPort" type="number" min="1" max="65535" placeholder="8899" required>
                            </div>
                            <div class="source-field">
                                <label for="gwInterval">Interval aktualizace (s)</label>
                                <input class="wifi-input" id="gwInterval" type="number" min="1" max="3600" required>
                            </div>
                        </div>
                        <div class="field-help">GoodWe střídač · lokální síťové připojení</div>
                    </section>

                    <section class="source-device-card">
                        <div class="source-device-header">
                            <div class="source-device-title">♨️ AZRouter</div>
                            <label class="source-enabled"><input id="azEnabled" type="checkbox" checked> Aktivní</label>
                        </div>
                        <div class="source-field">
                            <label for="azHost">Host / IP adresa</label>
                            <input class="wifi-input" id="azHost" placeholder="např. 192.168.1.51">
                        </div>
                        <label class="source-enabled"><input id="azAuthEnabled" type="checkbox" onchange="toggleAzAuthFields()"> Použít přihlášení (auth)</label>
                        <div class="source-field-row" id="azAuthFields">
                            <div class="source-field">
                                <label for="azUser">Uživatel</label>
                                <input class="wifi-input" id="azUser" autocomplete="username"
                                       maxlength="64" placeholder="WebUI uživatel">
                            </div>
                            <div class="source-field">
                                <label for="azPassword">Heslo</label>
                                <input class="wifi-input" id="azPassword" type="password"
                                       autocomplete="current-password" maxlength="128"
                                       placeholder="prázdné = zachovat uložené">
                            </div>
                        </div>
                        <div class="source-field-row">
                            <div class="source-field">
                                <label for="azPort">Port</label>
                                <input class="wifi-input" id="azPort" type="number" min="1" max="65535" placeholder="80" required>
                            </div>
                            <div class="source-field">
                                <label for="azInterval">Interval aktualizace (s)</label>
                                <input class="wifi-input" id="azInterval" type="number" min="1" max="3600" required>
                            </div>
                        </div>
                        <div class="field-help" id="azAuthHelp">AZRouter · přihlášení používá stejné údaje jako WebUI.</div>
                    </section>
                </div>
                <button class="btn btn-secondary source-save" type="submit">Uložit datové zdroje a restartovat</button>
            </form>
        </div>

        <div class="card">
            <div class="card-title">Počasí</div>
            <form onsubmit="saveWeather(event)">
                <div class="timezone-picker">
                    <input class="wifi-input" id="weatherPlace" type="search" autocomplete="off"
                           placeholder="Obec nebo PSČ, např. Praha"
                           oninput="scheduleWeatherPlaceSearch()"
                           onfocus="showWeatherPlaceSuggestions()">
                    <div class="timezone-results" id="weatherPlaceResults" hidden></div>
                </div>
                <div class="field-help">Začněte psát obec nebo PSČ a vyberte návrh. Souřadnice se doplní automaticky.</div>
                <select class="wifi-input" id="weatherProvider">
                    <option value="open-meteo">Open-Meteo</option>
                    <option value="met-no">MET Norway</option>
                    <option value="chmi" disabled>ČHMÚ (připravuje se)</option>
                </select>
                <div class="source-grid">
                    <label>Zeměpisná šířka</label>
                    <input class="wifi-input" id="weatherLatitude" type="number" step="0.000001" min="-90" max="90" required>
                    <span></span>
                    <label>Zeměpisná délka</label>
                    <input class="wifi-input" id="weatherLongitude" type="number" step="0.000001" min="-180" max="180" required>
                    <span></span>
                    <label>Interval (s)</label>
                    <input class="wifi-input" id="weatherInterval" type="number" min="900" max="21600" required>
                    <label><input id="weatherEnabled" type="checkbox"> aktivní</label>
                </div>
                <div class="update-message">Výchozí místo: Praha. Zdroje: <a href="https://open-meteo.com/" target="_blank" rel="noopener">Open-Meteo</a> a <a href="https://www.met.no/" target="_blank" rel="noopener">MET Norway</a>.</div>
                <button class="btn btn-secondary" type="submit">Uložit počasí a restartovat</button>
            </form>
        </div>

        <div class="card pool-settings-card wide-card">
            <div class="card-title">Bazén</div>
            <form onsubmit="savePool(event)">
                <section class="source-device-card">
                    <div class="source-device-header">
                        <div class="source-device-title">🏊 Zobrazení bazénu</div>
                        <label class="source-enabled">
                            <input id="poolEnabled" type="checkbox" checked onchange="savePool(event)"> Aktivní
                        </label>
                    </div>
                    <div class="field-help">
                        Určuje, zda se zobrazí stránka Bazén a bazénové údaje na hlavním souhrnu.
                        Další nastavení bazénu doplníme později.
                    </div>
                </section>
            </form>
        </div>

        <div class="card">
            <div class="card-title">Demonstrační data</div>
            <div class="update-message">Vnitřní teploty a bazénová technologie zatím nejsou měřené. Hodnoty na těchto obrazovkách jsou demonstrační.</div>
        </div>
        <div class="card">
            <div class="card-title">Aktualizace z GitHubu</div>
            <div class="status-grid">
                <div class="status-item">
                    <div class="status-label">Nainstalovaná verze</div>
                    <div class="status-value" id="githubCurrentVersion">-</div>
                </div>
                <div class="status-item">
                    <div class="status-label">Poslední release</div>
                    <div class="status-value" id="githubLatestVersion">Nezkontrolováno</div>
                </div>
            </div>
            <div class="update-message" id="githubUpdateMessage">Kontrola se spustí až po stisku tlačítka.</div>
            <div class="btn-group">
                <button class="btn btn-secondary" id="githubCheckButton" type="button" onclick="checkGithubUpdate()">Zkontrolovat novou verzi</button>
                <button class="btn btn-primary" id="githubInstallButton" type="button" onclick="installGithubUpdate()" hidden disabled>Stáhnout a nainstalovat</button>
            </div>
        </div>

        <div class="card">
            <div class="card-title">Ruční aktualizace firmware</div>
            <form action="/api/update" method="post" enctype="multipart/form-data">
                <label for="firmwareSha256">SHA-256 ze souboru firmware.bin.sha256</label>
                <input class="wifi-input" id="firmwareSha256" type="text" name="sha256" pattern="[a-fA-F0-9]{64}" minlength="64" maxlength="64" required>
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

        let systemConfigLoaded = false;
        let sourceConfigLoaded = false;
        let poolConfigLoaded = false;
        let weatherConfigLoaded = false;
        let lastScreenId = null;
        let weatherSearchResults = [];
        let weatherSearchTimer = null;
        let weatherSearchController = null;
        let statusRequestInFlight = false;
        let displayCommandInFlight = false;
        let displayBusy = false;
        let githubUpdateBusy = false;
        let githubUpdateInfo = null;

        function setDisplayActionsDisabled() {
            const disabled = displayCommandInFlight || displayBusy;
            document.querySelectorAll('[data-display-action]').forEach(button => {
                button.disabled = disabled;
            });
        }

        function displayStateText(state) {
            const labels = {
                stopped: 'Zastaveno',
                initializing: 'Inicializace',
                idle: 'Připraven',
                queued: 'Ve frontě',
                rendering_partial: 'Částečný refresh',
                rendering_full: 'Plný refresh',
                error: 'Chyba'
            };
            return labels[state] || state || 'Neznámý';
        }

        function setPoolScreenVisibility(enabled) {
            const button = document.querySelector(`button[onclick="activate('pool')"]`);
            if (button) button.hidden = !enabled;
        }

        function setSolarScreenVisibility(goodweEnabled, azrouterEnabled) {
            const button = document.querySelector(`button[onclick="activate('solar')"]`);
            if (button) button.hidden = !(goodweEnabled || azrouterEnabled);
        }

        async function updateStatus() {
            if (document.hidden || statusRequestInFlight || githubUpdateBusy) return;
            statusRequestInFlight = true;
            const controller = new AbortController();
            // 2.5 s was too aggressive for a weak Wi-Fi link / busy ESP and
            // caused the browser to abort while WebServer was still sending.
            const timeout = setTimeout(() => controller.abort(), 8000);
            try {
                const res = await fetch('/api/status', {
                    signal: controller.signal,
                    cache: 'no-store'
                });
                if (!res.ok) throw new Error('Status HTTP ' + res.status);
                const data = await res.json();
                window.dashboardLastStatus = data;
                window.dispatchEvent(new CustomEvent('dashboard-status', { detail: data }));

                if (data.sources) {
                    setSolarScreenVisibility(
                        data.sources.goodwe?.enabled !== false,
                        data.sources.azrouter?.enabled !== false
                    );
                }

                document.getElementById('fwVer').innerText = 'v' + data.firmware;
                document.getElementById('githubCurrentVersion').innerText = 'v' + data.firmware;
                document.getElementById('statScreen').innerText = data.screen;
                document.getElementById('statWifi').innerText = data.wifi.rssi + ' dBm';
                const gwEnabled = data.sources?.goodwe?.enabled !== false;
                const azEnabled = data.sources?.azrouter?.enabled !== false;
                document.getElementById('statGoodwe').innerText =
                    gwEnabled ? (data.goodwe.available ? 'OK' : 'Offline') : 'Vypnuto';
                const azModeLabels = {
                    bearer: 'token',
                    cookie: 'cookie',
                    'login-no-session': 'login',
                    anonymous: 'bez loginu',
                    invalid: 'neúplné údaje',
                    'login-failed': 'login chyba',
                    disabled: ''
                };
                const azMode = azModeLabels[data.azrouter.authMode] || data.azrouter.authMode || '';
                document.getElementById('statAzrouter').innerText = azEnabled
                    ? ((data.azrouter.available ? 'OK' : 'Offline') + (azMode ? ' · ' + azMode : ''))
                    : 'Vypnuto';
                document.getElementById('statWeather').innerText = data.weather.available ? data.weather.provider : (data.weather.enabled ? 'Offline' : 'Vypnuto');
                document.getElementById('statGoodweUpdate').innerText = formatDataAge(data.goodwe.lastUpdateAgeSeconds);
                document.getElementById('statAzrouterUpdate').innerText = formatDataAge(data.azrouter.lastUpdateAgeSeconds);
                document.getElementById('statWeatherUpdate').innerText = formatDataAge(data.weather.lastUpdateAgeSeconds);
                document.getElementById('statHeap').innerText = Math.round(data.freeHeap / 1024) + ' KB';

                const display = data.display || {};
                document.getElementById('statDisplayState').innerText = displayStateText(display.state);
                displayBusy = !['idle', 'stopped', 'error'].includes(display.state);
                setDisplayActionsDisabled();

                if (!systemConfigLoaded && data.systemConfig) {
                    document.getElementById('systemHostname').value = data.systemConfig.hostname;
                    document.getElementById('systemNtp').value = data.systemConfig.ntpServer;
                    const timezoneSelect = document.getElementById('systemTimezone');
                    const timezoneValue = data.systemConfig.timezone;
                    if (timezoneSelect && timezoneValue) {
                        const knownTimezone = Array.from(timezoneSelect.options).some(option => option.value === timezoneValue);
                        if (!knownTimezone) {
                            const customOption = document.createElement('option');
                            customOption.value = timezoneValue;
                            customOption.textContent = 'Vlastní / původní nastavení — ' + timezoneValue;
                            timezoneSelect.appendChild(customOption);
                        }
                        timezoneSelect.value = timezoneValue;
                        syncTimezoneSearchLabel();
                    }
                    systemConfigLoaded = true;
                }

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
                    const azAuthEnabled = document.getElementById('azAuthEnabled');
                    const azUser = document.getElementById('azUser');
                    azAuthEnabled.checked = az.authEnabled === true;
                    azUser.value = az.username || '';
                    azUser.disabled = !azAuthEnabled.checked;
                    const azPassword = document.getElementById('azPassword');
                    azPassword.disabled = !azAuthEnabled.checked;
                    azPassword.value = '';
                    azPassword.placeholder = az.passwordConfigured
                        ? 'uloženo — prázdné = zachovat'
                        : 'heslo do AZRouter WebUI';
                    const azAuthHelp = document.getElementById('azAuthHelp');
                    if (azAuthHelp) {
                        azAuthHelp.textContent = az.authEnabled
                            ? (az.passwordConfigured
                                ? 'Auth je zapnutý. Přihlašovací údaje jsou uložené; heslo se do prohlížeče neposílá.'
                                : 'Auth je zapnutý. Vyplň uživatele a heslo.')
                            : 'Auth je vypnutý (doporučeno pro běžný AZRouter). Uložené údaje zůstávají zachované.';
                    }
                    sourceConfigLoaded = true;
                }

                if (data.sources && data.sources.pool) {
                    const pool = data.sources.pool;
                    setPoolScreenVisibility(pool.enabled !== false);
                    if (!poolConfigLoaded) {
                        const enabled = document.getElementById('poolEnabled');
                        if (enabled) enabled.checked = pool.enabled !== false;
                        poolConfigLoaded = true;
                    }
                }

                if (!weatherConfigLoaded && data.sources && data.sources.weather) {
                    const weather = data.sources.weather;
                    document.getElementById('weatherProvider').value = weather.provider;
                    document.getElementById('weatherLatitude').value = weather.latitude;
                    document.getElementById('weatherLongitude').value = weather.longitude;
                    document.getElementById('weatherInterval').value = weather.interval;
                    document.getElementById('weatherEnabled').checked = weather.enabled;
                    weatherConfigLoaded = true;
                }

                const s = data.uptime;
                const hrs = Math.floor(s / 3600);
                const mins = Math.floor((s % 3600) / 60);
                document.getElementById('statUptime').innerText = `${hrs}h ${mins}m ${s % 60}s`;

                if (data.weather.forecastDays) {
                    const select = document.getElementById('weatherForecastDay');
                    Array.from(select.options).forEach((option, index) => {
                        const date = data.weather.forecastDays[index];
                        option.textContent = date ? date.slice(8, 10) + '.' + date.slice(5, 7) + '. — detail' : (index + 1) + '. den';
                    });
                }
                if (data.screen !== lastScreenId && data.screen.startsWith('weather-hourly-')) {
                    document.getElementById('weatherForecastDay').value = data.screen.slice('weather-hourly-'.length);
                }
                lastScreenId = data.screen;
                ['home', 'solar', 'pool', 'weather', 'diagnostics'].forEach(id => {
                    const el = document.querySelector(`button[onclick="activate('${id}')"]`);
                    if (el) el.classList.toggle('active', id === data.screen || (id === 'weather' && data.screen.startsWith('weather-hourly-')));
                });
            } catch (e) {
                if (e.name !== 'AbortError') console.error('Chyba čtení statusu:', e);
            } finally {
                clearTimeout(timeout);
                statusRequestInFlight = false;
            }
        }
        function formatDataAge(seconds) {
            return seconds === null ? 'Nikdy' : 'před ' + seconds + ' s';
        }

        async function activate(screenId) {
            if (displayCommandInFlight || displayBusy) return;
            displayCommandInFlight = true;
            setDisplayActionsDisabled();
            try {
                showToast('Přepínám na: ' + screenId);
                const res = await fetch(`/api/screens/${screenId}/activate`, { method: 'POST' });
                if (!res.ok) throw new Error('Activate HTTP ' + res.status);
                displayBusy = true;
                showToast('Překreslení zařazeno');
            } catch (e) {
                showToast('Chyba spojení');
            } finally {
                displayCommandInFlight = false;
                setDisplayActionsDisabled();
            }
        }

        async function triggerRefresh(full) {
            if (displayCommandInFlight || displayBusy) return;
            displayCommandInFlight = true;
            setDisplayActionsDisabled();
            try {
                const endpoint = full ? '/api/display/full-refresh' : '/api/display/refresh';
                const response = await fetch(endpoint, { method: 'POST' });
                if (!response.ok) throw new Error('Refresh HTTP ' + response.status);
                displayBusy = true;
                showToast(full ? 'Plný refresh zařazen' : 'Částečný refresh zařazen');
            } catch (e) {
                showToast('Chyba refresh');
            } finally {
                displayCommandInFlight = false;
                setDisplayActionsDisabled();
            }
        }
        function confirmRestart() {
            if (confirm("Opravdu chcete restartovat zařízení ESP32?")) {
                fetch('/api/system/restart', { method: 'POST' });
                showToast('ESP32 se restartuje...');
            }
        }

        async function importConfiguration() {
            const file = document.getElementById('configImportFile').files[0];
            if (!file) { showToast('Nejprve vyberte YAML soubor'); return; }
            if (file.size > 8192) { showToast('Soubor konfigurace je příliš velký'); return; }
            if (!confirm('Import přepíše Wi-Fi, zdroje i počasí a restartuje zařízení. Pokračovat?')) return;
            try {
                const yaml = await file.text();
                const response = await fetch('/api/config/import', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/yaml'},
                    body: yaml
                });
                const result = await response.json();
                if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));
                showToast('Konfigurace importována, zařízení se restartuje...');
            } catch (error) {
                showToast('Import selhal: ' + error.message);
            }
        }

        async function confirmFactoryReset() {
            if (!confirm('Opravdu smazat veškeré nastavení včetně Wi-Fi, zdrojů a počasí?')) return;
            if (!confirm('Tuto akci nelze vrátit. Po restartu může být nutné připojit se k síti Dashboard-Setup. Pokračovat?')) return;

            try {
                const body = new URLSearchParams({confirmation: 'RESET'});
                const response = await fetch('/api/config/factory-reset', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body
                });
                if (!response.ok) throw new Error('HTTP ' + response.status);
                showToast('Nastavení bylo smazáno, zařízení se restartuje...');
            } catch (error) {
                showToast('Tovární nastavení se nepodařilo obnovit');
            }
        }

        function timezoneOptionLabel(option) {
            return option ? option.textContent.trim() : '';
        }

        function syncTimezoneSearchLabel() {
            const select = document.getElementById('systemTimezone');
            const input = document.getElementById('systemTimezoneSearch');
            if (!select || !input) return;
            const selected = select.options[select.selectedIndex];
            input.value = timezoneOptionLabel(selected);
        }

        function selectTimezone(value) {
            const select = document.getElementById('systemTimezone');
            const input = document.getElementById('systemTimezoneSearch');
            const results = document.getElementById('systemTimezoneResults');
            if (!select || !input || !results) return;
            select.value = value;
            syncTimezoneSearchLabel();
            results.hidden = true;
        }

        function filterTimezones(query, editing) {
            const select = document.getElementById('systemTimezone');
            const input = document.getElementById('systemTimezoneSearch');
            const results = document.getElementById('systemTimezoneResults');
            if (!select || !input || !results) return;

            if (editing) {
                const selected = select.options[select.selectedIndex];
                if (!selected || timezoneOptionLabel(selected) !== input.value.trim()) {
                    select.value = '';
                }
            }

            const needle = (query || '').trim().toLowerCase();
            const options = Array.from(select.options);
            const matches = options.filter(option => {
                const group = option.parentElement && option.parentElement.tagName === 'OPTGROUP'
                    ? option.parentElement.label : '';
                const haystack = (timezoneOptionLabel(option) + ' ' + group + ' ' + option.value).toLowerCase();
                return !needle || haystack.includes(needle);
            });

            results.replaceChildren();
            if (matches.length === 0) {
                const empty = document.createElement('div');
                empty.className = 'timezone-no-result';
                empty.textContent = 'Nenalezeno. Zkus město, zemi nebo např. UTC+05:45.';
                results.appendChild(empty);
            } else {
                matches.forEach(option => {
                    const button = document.createElement('button');
                    button.type = 'button';
                    button.className = 'timezone-result';
                    button.textContent = timezoneOptionLabel(option);
                    button.addEventListener('click', () => selectTimezone(option.value));
                    results.appendChild(button);
                });
            }
            results.hidden = false;
        }

        document.addEventListener('click', event => {
            const picker = event.target.closest('.timezone-picker');
            if (picker) return;
            ['systemTimezoneResults', 'weatherPlaceResults'].forEach(id => {
                const results = document.getElementById(id);
                if (results) results.hidden = true;
            });
        });

        async function saveSystem(event) {
            event.preventDefault();
            const timezoneValue = document.getElementById('systemTimezone').value.trim();
            if (!timezoneValue) {
                showToast('Vyber časové pásmo ze seznamu');
                document.getElementById('systemTimezoneSearch').focus();
                return;
            }
            const body = new URLSearchParams({
                hostname: document.getElementById('systemHostname').value.trim(),
                ntpServer: document.getElementById('systemNtp').value.trim(),
                timezone: timezoneValue
            });
            const response = await fetch('/api/config/system', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body
            });
            showToast(response.ok ? 'Systém uložen, zařízení se restartuje' : 'Systém se nepodařilo uložit');
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

        function toggleAzAuthFields() {
            const enabled = document.getElementById('azAuthEnabled').checked;
            document.getElementById('azUser').disabled = !enabled;
            document.getElementById('azPassword').disabled = !enabled;
            const help = document.getElementById('azAuthHelp');
            if (help && !enabled) {
                help.textContent = 'Auth je vypnutý (doporučeno pro běžný AZRouter). Uložené údaje zůstávají zachované.';
            }
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
                azEnabled: document.getElementById('azEnabled').checked ? '1' : '0',
                azAuthEnabled: document.getElementById('azAuthEnabled').checked ? '1' : '0',
                azUser: document.getElementById('azUser').value,
                azPassword: document.getElementById('azPassword').value
            });
            const response = await fetch('/api/config/sources', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body
            });
            if (response.ok) {
                setSolarScreenVisibility(
                    document.getElementById('gwEnabled').checked,
                    document.getElementById('azEnabled').checked
                );
            }
            showToast(response.ok ? 'Zdroje uloženy, zařízení se restartuje' : 'Zdroje se nepodařilo uložit');
        }

        function scheduleWeatherPlaceSearch() {
            const input = document.getElementById('weatherPlace');
            const results = document.getElementById('weatherPlaceResults');
            if (!input || !results) return;

            clearTimeout(weatherSearchTimer);
            if (weatherSearchController) {
                weatherSearchController.abort();
                weatherSearchController = null;
            }

            const query = input.value.trim();
            if (query.length < 2) {
                weatherSearchResults = [];
                results.replaceChildren();
                results.hidden = true;
                return;
            }

            results.replaceChildren();
            const loading = document.createElement('div');
            loading.className = 'timezone-no-result';
            loading.textContent = 'Hledám…';
            results.appendChild(loading);
            results.hidden = false;

            weatherSearchTimer = setTimeout(() => searchWeatherPlace(query), 350);
        }

        function showWeatherPlaceSuggestions() {
            const input = document.getElementById('weatherPlace');
            if (!input || input.value.trim().length < 2) return;
            if (weatherSearchResults.length) {
                renderWeatherPlaceSuggestions();
            } else {
                scheduleWeatherPlaceSearch();
            }
        }

        function renderWeatherPlaceSuggestions(message = '') {
            const results = document.getElementById('weatherPlaceResults');
            if (!results) return;
            results.replaceChildren();

            if (message) {
                const info = document.createElement('div');
                info.className = 'timezone-no-result';
                info.textContent = message;
                results.appendChild(info);
            } else if (!weatherSearchResults.length) {
                const empty = document.createElement('div');
                empty.className = 'timezone-no-result';
                empty.textContent = 'Místo nebylo nalezeno.';
                results.appendChild(empty);
            } else {
                weatherSearchResults.forEach((place, index) => {
                    const button = document.createElement('button');
                    button.type = 'button';
                    button.className = 'timezone-result';
                    button.textContent = [place.name, place.admin2, place.admin1, place.country]
                        .filter(Boolean).join(', ');
                    button.addEventListener('click', () => applyWeatherPlace(index));
                    results.appendChild(button);
                });
            }
            results.hidden = false;
        }

        async function searchWeatherPlace(query) {
            const input = document.getElementById('weatherPlace');
            if (!input || input.value.trim() !== query) return;

            const controller = new AbortController();
            weatherSearchController = controller;
            const timeout = setTimeout(() => controller.abort(), 5000);
            try {
                const url = 'https://geocoding-api.open-meteo.com/v1/search?' +
                    new URLSearchParams({name: query, count: '8', language: 'cs', format: 'json'});
                const response = await fetch(url, {signal: controller.signal});
                if (!response.ok) throw new Error('Geocoding HTTP ' + response.status);
                const data = await response.json();
                if (input.value.trim() !== query) return;
                weatherSearchResults = Array.isArray(data.results) ? data.results : [];
                renderWeatherPlaceSuggestions();
            } catch (error) {
                if (error.name !== 'AbortError') {
                    weatherSearchResults = [];
                    renderWeatherPlaceSuggestions('Vyhledávání se nepodařilo.');
                }
            } finally {
                clearTimeout(timeout);
                if (weatherSearchController === controller) weatherSearchController = null;
            }
        }

        function applyWeatherPlace(index) {
            const place = weatherSearchResults[Number(index)];
            if (!place) return;
            document.getElementById('weatherLatitude').value = place.latitude;
            document.getElementById('weatherLongitude').value = place.longitude;
            document.getElementById('weatherPlace').value =
                [place.name, place.admin2, place.admin1, place.country].filter(Boolean).join(', ');
            const results = document.getElementById('weatherPlaceResults');
            if (results) results.hidden = true;
            showToast('Souřadnice byly vyplněny');
        }

        async function savePool(event) {
            if (event) event.preventDefault();
            const enabledInput = document.getElementById('poolEnabled');
            if (!enabledInput) return;

            enabledInput.disabled = true;
            const enabled = enabledInput.checked;
            setPoolScreenVisibility(enabled);

            try {
                const body = new URLSearchParams({enabled: enabled ? '1' : '0'});
                const response = await fetch('/api/config/pool', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body
                });
                const result = await response.json().catch(() => ({}));
                if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));
                poolConfigLoaded = false;
                showToast(enabled ? 'Bazén zapnut' : 'Bazén vypnut');
                setTimeout(updateStatus, 100);
            } catch (error) {
                enabledInput.checked = !enabled;
                setPoolScreenVisibility(!enabled);
                showToast('Nastavení bazénu se nepodařilo uložit');
            } finally {
                enabledInput.disabled = false;
            }
        }

        async function saveWeather(event) {
            event.preventDefault();
            const body = new URLSearchParams({
                provider: document.getElementById('weatherProvider').value,
                latitude: document.getElementById('weatherLatitude').value,
                longitude: document.getElementById('weatherLongitude').value,
                interval: document.getElementById('weatherInterval').value,
                enabled: document.getElementById('weatherEnabled').checked ? '1' : '0'
            });
            const response = await fetch('/api/config/weather', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body
            });
            showToast(response.ok ? 'Počasí uloženo, zařízení se restartuje' : 'Počasí se nepodařilo uložit');
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

        function setGithubUpdateBusy(busy) {
            githubUpdateBusy = busy;
            const checkButton = document.getElementById('githubCheckButton');
            const installButton = document.getElementById('githubInstallButton');
            checkButton.disabled = busy;
            installButton.disabled = busy || !githubUpdateInfo;
        }

        function setGithubUpdateMessage(message, type = '') {
            const element = document.getElementById('githubUpdateMessage');
            element.textContent = message;
            element.className = 'update-message' + (type ? ' ' + type : '');
        }

        async function checkGithubUpdate() {
            githubUpdateInfo = null;
            const installButton = document.getElementById('githubInstallButton');
            installButton.hidden = true;
            setGithubUpdateBusy(true);
            setGithubUpdateMessage('Kontroluji poslední GitHub release...');
            try {
                const response = await fetch('/api/update/check', { cache: 'no-store' });
                const data = await response.json();
                if (!response.ok || data.status === 'error') {
                    throw new Error(data.message || 'Kontrola aktualizace selhala');
                }

                document.getElementById('githubCurrentVersion').textContent = 'v' + data.current_version;
                document.getElementById('githubLatestVersion').textContent = 'v' + data.latest_version;
                if (data.status === 'update_available') {
                    githubUpdateInfo = data;
                    installButton.hidden = false;
                    setGithubUpdateMessage('Je dostupná nová verze v' + data.latest_version + '.', 'ok');
                } else if (data.current_version === data.latest_version) {
                    setGithubUpdateMessage('Používáte nejnovější dostupnou verzi.', 'ok');
                } else {
                    setGithubUpdateMessage('Nainstalovaná verze je novější než poslední GitHub release.', 'ok');
                }
            } catch (error) {
                setGithubUpdateMessage('Kontrola se nezdařila: ' + error.message, 'error');
            } finally {
                setGithubUpdateBusy(false);
            }
        }

        async function installGithubUpdate() {
            if (!githubUpdateInfo) return;
            const current = githubUpdateInfo.current_version;
            const latest = githubUpdateInfo.latest_version;
            if (!confirm('Aktualizovat firmware z v' + current + ' na v' + latest + '? Zařízení se po instalaci restartuje.')) return;

            setGithubUpdateBusy(true);
            setGithubUpdateMessage('Stahuji a ověřuji firmware v' + latest + '. Nezavírejte stránku...');
            try {
                const response = await fetch('/api/update/github', { method: 'POST' });
                const data = await response.json();
                if (!response.ok) throw new Error(data.message || 'Instalace selhala');
                setGithubUpdateMessage('Firmware je ověřený. Zařízení se restartuje...', 'ok');
                showToast('Aktualizace byla nainstalována');
                await waitForDashboardRestart();
            } catch (error) {
                setGithubUpdateMessage('Instalace se nezdařila: ' + error.message, 'error');
                setGithubUpdateBusy(false);
            }
        }

        async function waitForDashboardRestart() {
            await new Promise(resolve => setTimeout(resolve, 3000));
            for (let attempt = 0; attempt < 30; attempt++) {
                const controller = new AbortController();
                const timeout = setTimeout(() => controller.abort(), 1500);
                try {
                    const response = await fetch('/api/status', { cache: 'no-store', signal: controller.signal });
                    if (response.ok) {
                        location.reload();
                        return;
                    }
                } catch (error) {
                    // Restart krátce přeruší dostupnost HTTP serveru.
                } finally {
                    clearTimeout(timeout);
                }
                await new Promise(resolve => setTimeout(resolve, 1500));
            }
            setGithubUpdateMessage('Zařízení se zatím neozvalo. Obnovte stránku ručně.', 'error');
            setGithubUpdateBusy(false);
        }

        window.dashboardUpdateStatus = updateStatus;
        const statusTimer = setInterval(updateStatus, 5000);
        document.addEventListener('visibilitychange', () => {
            if (!document.hidden) updateStatus();
        });
        updateStatus();
    </script>

<script>
(function () {
    const layoutMarker = 'responsive-shell-v1';

    function makeHeading(title, description) {
        const heading = document.createElement('div');
        heading.className = 'section-heading';
        const h2 = document.createElement('h2');
        h2.textContent = title;
        const p = document.createElement('p');
        p.textContent = description;
        heading.append(h2, p);
        return heading;
    }

    function makeView(name, title, description) {
        const section = document.createElement('section');
        section.className = 'app-view view-' + name;
        section.dataset.view = name;
        section.hidden = true;
        const grid = document.createElement('div');
        grid.className = 'section-grid';
        section.append(makeHeading(title, description), grid);
        return { section, grid };
    }

    function cardTitle(card) {
        const title = card.querySelector('.card-title');
        return title ? title.textContent.trim() : '';
    }

    function initResponsiveShell() {
        const container = document.querySelector('.container');
        if (!container || document.getElementById('appNavigation')) return;

        const header = Array.from(container.children).find(el => el.classList && el.classList.contains('header'));
        const cards = Array.from(container.children).filter(el => el.classList && el.classList.contains('card'));
        if (!header || !cards.length) return;

        const shell = document.createElement('div');
        shell.className = 'app-shell';
        shell.dataset.layout = layoutMarker;

        const nav = document.createElement('nav');
        nav.id = 'appNavigation';
        nav.className = 'app-nav';
        nav.setAttribute('aria-label', 'Hlavní navigace');

        const content = document.createElement('main');
        content.className = 'app-content';

        const views = {
            screens: makeView('screens', 'Obrazovky', 'Běžné ovládání e-paper displeje a rychlé přepínání zobrazení.'),
            settings: makeView('settings', 'Nastavení', 'Konfigurace sítě, systému a datových zdrojů dashboardu.'),
            system: makeView('system', 'Systém', 'Stav zařízení, firmware, záloha konfigurace a servisní operace.')
        };

        Object.values(views).forEach(view => content.appendChild(view.section));

        const systemCard = cards.find(card => cardTitle(card) === 'Systém');
        if (systemCard) {
            const systemForm = systemCard.querySelector('form[onsubmit^="saveSystem"]');
            if (systemForm) {
                const settingsCard = document.createElement('div');
                settingsCard.className = 'card';
                const title = document.createElement('div');
                title.className = 'card-title';
                title.textContent = 'Systémové nastavení';
                settingsCard.append(title, systemForm);
                views.settings.grid.appendChild(settingsCard);
            }
            const title = systemCard.querySelector('.card-title');
            if (title) title.textContent = 'Údržba zařízení';
        }

        cards.forEach(card => {
            const title = cardTitle(card);
            let target = views.settings.grid;

            if (title === 'Aktivní obrazovka displeje' || title === 'Ovládání displeje') {
                target = views.screens.grid;
            } else if (title === 'Systémový stav' || title === 'Údržba zařízení' ||
                       title.indexOf('Aktualizace z GitHubu') >= 0 || title.indexOf('Demonstrační data') >= 0 ||
                       title.toLowerCase().indexOf('firmware') >= 0) {
                target = views.system.grid;
            }

            if (title === 'Aktivní obrazovka displeje' || title === 'Datové zdroje' || title === 'Systémový stav') {
                card.classList.add('wide-card');
            }
            target.appendChild(card);
        });

        const navItems = [
            ['screens', '🖥', 'Obrazovky'],
            ['settings', '⚙', 'Nastavení'],
            ['system', '☰', 'Systém']
        ];

        function activateView(name, updateHash) {
            if (!views[name]) name = 'screens';
            Object.entries(views).forEach(([key, view]) => {
                view.section.hidden = key !== name;
            });
            nav.querySelectorAll('.app-nav-button').forEach(button => {
                const active = button.dataset.target === name;
                button.classList.toggle('active', active);
                button.setAttribute('aria-current', active ? 'page' : 'false');
            });
            try { localStorage.setItem('dashboardView', name); } catch (_) {}
            if (updateHash && location.hash !== '#' + name) history.replaceState(null, '', '#' + name);
            window.scrollTo({ top: 0, behavior: 'auto' });
        }

        navItems.forEach(([name, icon, label]) => {
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'app-nav-button';
            button.dataset.target = name;
            button.innerHTML = '<span class="app-nav-icon">' + icon + '</span><span>' + label + '</span>';
            button.addEventListener('click', () => activateView(name, true));
            nav.appendChild(button);
        });

        shell.append(nav, content);
        header.insertAdjacentElement('afterend', shell);

        let initial = location.hash.replace('#', '');
        if (!views[initial]) {
            try { initial = localStorage.getItem('dashboardView') || 'screens'; } catch (_) { initial = 'screens'; }
        }
        activateView(initial, false);

        window.addEventListener('hashchange', () => {
            const requested = location.hash.replace('#', '');
            if (views[requested]) activateView(requested, false);
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initResponsiveShell, { once: true });
    } else {
        initResponsiveShell();
    }
})();
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

    // Weather HTTPS temporarily owns this gate while its TLS buffers are live.
    // Do not accept/read local HTTP clients at the same time: on ESP32-WROOM
    // both paths compete for internal RAM used by WiFiClient receive buffers.
    if (_networkClientGate != nullptr &&
        xSemaphoreTake(_networkClientGate, 0) != pdTRUE) {
        return;
    }

    {
        Performance::Scope timing(Performance::WebService);

        const uint32_t startedMs = millis();
        _server.handleClient();
        const uint32_t elapsedMs = millis() - startedMs;

        if (elapsedMs >= 500UL) {
            WiFiClient client = _server.client();
            const IPAddress remote = client ? client.remoteIP() : IPAddress();
            Serial.printf(
                "[WEB][STALL] handleClient=%lu ms | uri=%s | remote=%s | connected=%d\n",
                static_cast<unsigned long>(elapsedMs),
                _server.uri().c_str(),
                remote.toString().c_str(),
                client ? client.connected() : 0);
        }
    }

    if (_networkClientGate != nullptr) {
        xSemaphoreGive(_networkClientGate);
    }
}

void DashboardWebServer::onScreenChange(ScreenChangeCallback callback) {
    _screenCallback = callback;
}

void DashboardWebServer::onRefresh(RefreshCallback callback) {
    _refreshCallback = callback;
}

void DashboardWebServer::onDisplayStatus(DisplayStatusCallback callback) {
    _displayStatusCallback = callback;
}

void DashboardWebServer::onSystemConfig(SystemConfigCallback callback) {
    _systemConfigCallback = callback;
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

void DashboardWebServer::onPoolConfig(PoolConfigCallback callback) {
    _poolConfigCallback = callback;
}

void DashboardWebServer::onWeatherConfig(WeatherConfigCallback callback) {
    _weatherConfigCallback = callback;
}

void DashboardWebServer::onFactoryReset(FactoryResetCallback callback) {
    _factoryResetCallback = callback;
}

void DashboardWebServer::onConfigImport(ConfigImportCallback callback) {
    _configImportCallback = callback;
}

void DashboardWebServer::setupRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    _server.on("/api/screens", HTTP_GET, [this]() { handleApiScreens(); });

    _server.on("/api/screens/home/activate", HTTP_POST, [this]() { handleApiActivateScreen("home"); });
    _server.on("/api/screens/solar/activate", HTTP_POST, [this]() { handleApiActivateScreen("solar"); });
    _server.on("/api/screens/pool/activate", HTTP_POST, [this]() { handleApiActivateScreen("pool"); });
    _server.on("/api/screens/weather/activate", HTTP_POST, [this]() { handleApiActivateScreen("weather"); });
    for (uint8_t day = 0; day < WeatherForecastDayCount; ++day) {
        const String id = "weather-hourly-" + String(day);
        _server.on("/api/screens/" + id + "/activate", HTTP_POST, [this, id]() {
            handleApiActivateScreen(id);
        });
    }
    _server.on("/api/screens/diagnostics/activate", HTTP_POST, [this]() { handleApiActivateScreen("diagnostics"); });

    _server.on("/api/display/refresh", HTTP_POST, [this]() { handleApiRefresh(false); });
    _server.on("/api/display/full-refresh", HTTP_POST, [this]() { handleApiRefresh(true); });
    _server.on("/api/system/restart", HTTP_POST, [this]() { handleApiRestart(); });
    _server.on("/api/config/factory-reset", HTTP_POST, [this]() { handleApiFactoryReset(); });
    _server.on("/api/config/export", HTTP_GET, [this]() { handleApiConfigExport(); });
    _server.on("/api/config/import", HTTP_POST, [this]() { handleApiConfigImport(); });
    _server.on("/api/config/system", HTTP_POST, [this]() { handleApiSystemConfig(); });
    _server.on("/api/wifi/config", HTTP_POST, [this]() { handleApiWifiConfig(); });
    _server.on("/api/wifi/scan", HTTP_GET, [this]() { handleApiWifiScan(); });
    _server.on("/api/config/sources", HTTP_POST, [this]() { handleApiSourceConfig(); });
    _server.on("/api/config/pool", HTTP_POST, [this]() { handleApiPoolConfig(); });
    _server.on("/api/config/weather", HTTP_POST, [this]() { handleApiWeatherConfig(); });
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

    const DisplayTaskStatus displayStatus = _displayStatusCallback
        ? _displayStatusCallback()
        : DisplayTaskStatus{};
    JsonObject displayObj = doc["display"].to<JsonObject>();
    displayObj["state"] = displayTaskStateName(displayStatus.state);
    displayObj["ready"] = displayStatus.ready;
    displayObj["pending"] = displayStatus.pending;
    displayObj["pendingFull"] = displayStatus.pendingFull;
    displayObj["completedCount"] = displayStatus.completedCount;
    displayObj["lastDurationMs"] = displayStatus.lastDurationMs;
    displayObj["lastCompletedMs"] = displayStatus.lastCompletedMs;
    displayObj["stackHighWaterWords"] = displayStatus.stackHighWaterWords;
    const bool includeDetails =
        _server.hasArg("details") && _server.arg("details") != "0";

    if (includeDetails) {
        JsonObject perf = doc["performance"].to<JsonObject>();
        perf["uptimeMs"] = millis();
        for (int i = 0; i < Performance::Count; ++i) {
            const auto metric = static_cast<Performance::Metric>(i);
            const auto value = Performance::stats(metric);
            JsonObject item = perf[Performance::name(metric)].to<JsonObject>();
            item["count"] = value.count;
            item["avgMs"] = value.count ? (double)value.totalMs / value.count : 0;
            item["maxMs"] = value.maxMs;
            item["lastMs"] = value.lastMs;
            item["lastEndMs"] = value.lastEndMs;
        }
    }

    JsonObject systemConfigObj = doc["systemConfig"].to<JsonObject>();
    systemConfigObj["hostname"] = _config.system.hostname;
    systemConfigObj["ntpServer"] = _config.system.ntpServer;
    systemConfigObj["timezone"] = _config.system.timezone;

    JsonObject wifiObj = doc["wifi"].to<JsonObject>();
    wifiObj["connected"] = _dataModel.system.wifiConnected;
    wifiObj["rssi"] = _dataModel.system.wifiRssi;
    wifiObj["ip"] = _dataModel.system.ipAddress;

    JsonObject gwObj = doc["goodwe"].to<JsonObject>();
    gwObj["available"] = _dataModel.solar.status.available;
    if (_dataModel.solar.status.lastSuccessMs == 0) {
        gwObj["lastUpdateAgeSeconds"] = nullptr;
    } else {
        gwObj["lastUpdateAgeSeconds"] = (millis() - _dataModel.solar.status.lastSuccessMs) / 1000;
    }

    JsonObject azObj = doc["azrouter"].to<JsonObject>();
    azObj["available"] = _dataModel.azrouter.status.available;
    azObj["authenticated"] = _dataModel.azrouter.authenticated;
    azObj["authMode"] = _dataModel.azrouter.authMode;

    if (_dataModel.azrouter.hasRoutedPower) azObj["routedPowerW"] = _dataModel.azrouter.routedPowerW;
    else azObj["routedPowerW"] = nullptr;
    if (_dataModel.azrouter.hasGridPower) azObj["gridPowerW"] = _dataModel.azrouter.gridPowerW;
    else azObj["gridPowerW"] = nullptr;
    if (_dataModel.azrouter.hasSystemTemp) azObj["systemTempC"] = _dataModel.azrouter.systemTempC;
    else azObj["systemTempC"] = nullptr;

    if (_dataModel.azrouter.hasSystemStatus) azObj["systemStatusCode"] = _dataModel.azrouter.systemStatusCode;
    else azObj["systemStatusCode"] = nullptr;
    if (_dataModel.azrouter.hasHdo) azObj["hdo"] = _dataModel.azrouter.hdoOn;
    else azObj["hdo"] = nullptr;
    if (_dataModel.azrouter.hasMode) azObj["modeCode"] = _dataModel.azrouter.modeCode;
    else azObj["modeCode"] = nullptr;
    if (_dataModel.azrouter.hasMasterBoost) azObj["masterBoost"] = _dataModel.azrouter.masterBoost;
    else azObj["masterBoost"] = nullptr;

    if (includeDetails) {
        JsonArray gridPhases = azObj["gridPhases"].to<JsonArray>();
        JsonArray routedPhases = azObj["routedPhases"].to<JsonArray>();
        for (uint8_t phase = 0; phase < 3; ++phase) {
            JsonObject grid = gridPhases.add<JsonObject>();
            grid["phase"] = phase + 1;
            if (_dataModel.azrouter.hasGridPhasePower[phase]) grid["powerW"] = _dataModel.azrouter.gridPhasePowerW[phase];
            else grid["powerW"] = nullptr;
            if (_dataModel.azrouter.hasGridPhaseCurrent[phase]) grid["currentA"] = _dataModel.azrouter.gridPhaseCurrentA[phase];
            else grid["currentA"] = nullptr;
            if (_dataModel.azrouter.hasGridPhaseVoltage[phase]) grid["voltageV"] = _dataModel.azrouter.gridPhaseVoltageV[phase];
            else grid["voltageV"] = nullptr;
            if (_dataModel.azrouter.hasGridPhaseStatus[phase]) grid["connected"] = _dataModel.azrouter.gridPhaseConnected[phase];
            else grid["connected"] = nullptr;

            JsonObject routed = routedPhases.add<JsonObject>();
            routed["phase"] = phase + 1;
            if (_dataModel.azrouter.hasRoutedPhasePower[phase]) routed["powerW"] = _dataModel.azrouter.routedPhasePowerW[phase];
            else routed["powerW"] = nullptr;
        }

        JsonObject energy = azObj["energy"].to<JsonObject>();
        if (_dataModel.azrouter.hasRoutedEnergyTotal) energy["totalKWh"] = _dataModel.azrouter.routedEnergyTotalKWh;
        else energy["totalKWh"] = nullptr;
        if (_dataModel.azrouter.hasRoutedEnergyYear) energy["yearKWh"] = _dataModel.azrouter.routedEnergyYearKWh;
        else energy["yearKWh"] = nullptr;
        if (_dataModel.azrouter.hasRoutedEnergyMonth) energy["monthKWh"] = _dataModel.azrouter.routedEnergyMonthKWh;
        else energy["monthKWh"] = nullptr;
        if (_dataModel.azrouter.hasRoutedEnergyWeek) energy["weekKWh"] = _dataModel.azrouter.routedEnergyWeekKWh;
        else energy["weekKWh"] = nullptr;
        if (_dataModel.azrouter.hasRoutedEnergyToday) energy["todayKWh"] = _dataModel.azrouter.routedEnergyTodayKWh;
        else energy["todayKWh"] = nullptr;

    }

    // Device-level telemetry remains available internally, but is not exposed
    // as dashboard master telemetry because it depends on the attached load.

    if (_dataModel.azrouter.status.lastSuccessMs == 0) {
        azObj["lastUpdateAgeSeconds"] = nullptr;
    } else {
        azObj["lastUpdateAgeSeconds"] = (millis() - _dataModel.azrouter.status.lastSuccessMs) / 1000;
    }

    JsonObject poolObj = doc["pool"].to<JsonObject>();
    poolObj["enabled"] = _config.pool.enabled;

    JsonObject bme280Obj = doc["bme280"].to<JsonObject>();
    bme280Obj["available"] = _dataModel.inside.status.available;
    bme280Obj["temperatureC"] = _dataModel.inside.temperatureC;
    bme280Obj["humidityPercent"] = _dataModel.inside.humidityPercent;
    bme280Obj["pressureHpa"] = _dataModel.inside.pressureHpa;
    bme280Obj["lastError"] = _dataModel.inside.status.lastError;
    if (_dataModel.inside.status.lastSuccessMs == 0) {
        bme280Obj["lastUpdateAgeSeconds"] = nullptr;
    } else {
        bme280Obj["lastUpdateAgeSeconds"] =
            (millis() - _dataModel.inside.status.lastSuccessMs) / 1000;
    }

    JsonObject weatherObj = doc["weather"].to<JsonObject>();
    weatherObj["enabled"] = _config.weather.enabled;
    weatherObj["available"] = _dataModel.weather.status.available;
    weatherObj["provider"] = _dataModel.weather.provider;
    weatherObj["locationName"] = _dataModel.weather.locationName;
    weatherObj["lastError"] = _dataModel.weather.status.lastError;
    JsonArray forecastDays = weatherObj["forecastDays"].to<JsonArray>();
    for (uint8_t i = 0; i < _dataModel.weather.dailyCount; ++i) {
        forecastDays.add(_dataModel.weather.daily[i].date);
    }
    weatherObj["hourlyCount"] = _dataModel.weather.hourlyCount;
    if (_dataModel.weather.status.lastSuccessMs == 0) {
        weatherObj["lastUpdateAgeSeconds"] = nullptr;
    } else {
        weatherObj["lastUpdateAgeSeconds"] =
            (millis() - _dataModel.weather.status.lastSuccessMs) / 1000;
    }
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
    sourceAz["authEnabled"] = _config.azrouter.authEnabled;
    sourceAz["username"] = _config.azrouter.username;
    sourceAz["passwordConfigured"] = !_config.azrouter.password.isEmpty();
    JsonObject sourcePool = sourcesObj["pool"].to<JsonObject>();
    sourcePool["enabled"] = _config.pool.enabled;
    JsonObject sourceWeather = sourcesObj["weather"].to<JsonObject>();
    sourceWeather["enabled"] = _config.weather.enabled;
    sourceWeather["provider"] = _config.weather.provider;
    sourceWeather["latitude"] = _config.weather.latitude;
    sourceWeather["longitude"] = _config.weather.longitude;
    sourceWeather["interval"] = _config.weather.pollIntervalSeconds;
    sourceWeather["activeLocationId"] = _config.weather.activeLocationId;
    const WeatherLocation* activeWeatherLocation = _config.weather.activeLocation();
    sourceWeather["activeLocationName"] = activeWeatherLocation ? activeWeatherLocation->name : "";
    JsonArray weatherLocations = sourceWeather["locations"].to<JsonArray>();
    for (uint8_t i = 0; i < _config.weather.locationCount && i < MaxWeatherLocations; ++i) {
        const auto& location = _config.weather.locations[i];
        JsonObject item = weatherLocations.add<JsonObject>();
        item["id"] = location.id;
        item["name"] = location.name;
        item["country"] = location.country;
        item["latitude"] = location.latitude;
        item["longitude"] = location.longitude;
    }

    String response;
    serializeJson(doc, response);
    _server.sendHeader("Cache-Control", "no-store");
    _server.sendHeader("Connection", "close");
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

void DashboardWebServer::handleApiFactoryReset() {
    if (!_server.hasArg("confirmation") || _server.arg("confirmation") != "RESET") {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Factory reset confirmation required\"}");
        return;
    }
    if (!_factoryResetCallback) {
        _server.send(503, "application/json",
                     "{\"status\":\"error\",\"message\":\"Factory reset unavailable\"}");
        return;
    }
    if (!_factoryResetCallback()) {
        _server.send(500, "application/json",
                     "{\"status\":\"error\",\"message\":\"Failed to clear configuration\"}");
        return;
    }

    _server.send(200, "application/json",
                 "{\"status\":\"ok\",\"message\":\"Factory defaults restored; restarting\"}");
    delay(750);
    ESP.restart();
}

void DashboardWebServer::handleApiConfigExport() {
    const String yaml = exportConfigurationYaml(_config);
    _server.sendHeader("Content-Disposition", "attachment; filename=\"pvdashboard-config.yaml\"");
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/yaml; charset=utf-8", yaml);
}

void DashboardWebServer::handleApiConfigImport() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"YAML body is required\"}");
        return;
    }
    AppConfig imported;
    String error;
    if (!importConfigurationYaml(_server.arg("plain"), imported, error)) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"" + error + "\"}");
        return;
    }
    if (!_configImportCallback || !_configImportCallback(imported)) {
        _server.send(500, "application/json",
                     "{\"status\":\"error\",\"message\":\"Failed to save configuration\"}");
        return;
    }
    _server.send(200, "application/json",
                 "{\"status\":\"ok\",\"message\":\"Configuration imported; restarting\"}");
    delay(750);
    ESP.restart();
}

void DashboardWebServer::handleApiSystemConfig() {
    const char* required[] = {"hostname", "ntpServer", "timezone"};
    for (const char* name : required) {
        if (!_server.hasArg(name) || _server.arg(name).isEmpty()) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing system setting\"}");
            return;
        }
    }
    SystemConfig system;
    system.hostname = _server.arg("hostname");
    system.ntpServer = _server.arg("ntpServer");
    system.timezone = _server.arg("timezone");
    const auto hasControl = [](const String& value) {
        for (size_t i = 0; i < value.length(); ++i) if (static_cast<uint8_t>(value[i]) < 0x20) return true;
        return false;
    };
    bool hostnameValid = system.hostname.length() <= 32 && system.hostname[0] != '-' && !system.hostname.endsWith("-");
    for (size_t i = 0; hostnameValid && i < system.hostname.length(); ++i) {
        const char c = system.hostname[i];
        hostnameValid = isAlphaNumeric(c) || c == '-';
    }
    if (!hostnameValid || system.ntpServer.length() > 253 || system.timezone.length() > 127 ||
        hasControl(system.ntpServer) || hasControl(system.timezone)) {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid system setting\"}");
        return;
    }
    if (!_systemConfigCallback) {
        _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"System configuration unavailable\"}");
        return;
    }
    _server.send(200, "application/json", "{\"status\":\"saved\"}");
    _systemConfigCallback(system);
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
    const char* required[] = {"gwPort", "gwInterval", "azPort", "azInterval"};
    for (const char* name : required) {
        if (!_server.hasArg(name) || _server.arg(name).isEmpty()) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing source setting\"}");
            return;
        }
    }

    const bool gwEnabled = _server.arg("gwEnabled") == "1";
    const bool azEnabled = _server.arg("azEnabled") == "1";
    const bool azAuthEnabled = _server.arg("azAuthEnabled") == "1";
    const String gwHost = _server.arg("gwHost");
    const String azHost = _server.arg("azHost");
    String azUser = _server.hasArg("azUser") ? _server.arg("azUser") : _config.azrouter.username;
    String azPassword = _server.hasArg("azPassword") ? _server.arg("azPassword") : "";
    azUser.trim();
    long gwPort = _server.arg("gwPort").toInt();
    long azPort = _server.arg("azPort").toInt();
    long gwInterval = _server.arg("gwInterval").toInt();
    long azInterval = _server.arg("azInterval").toInt();
    auto invalidCredentialText = [](const String& value, size_t maxLength) {
        if (value.length() > maxLength) return true;
        for (size_t i = 0; i < value.length(); ++i) {
            if (static_cast<uint8_t>(value[i]) < 0x20) return true;
        }
        return false;
    };

    if ((gwEnabled && gwHost.isEmpty()) || (azEnabled && azHost.isEmpty()) ||
        gwPort < 1 || gwPort > 65535 || azPort < 1 || azPort > 65535 ||
        gwInterval < 1 || gwInterval > 3600 || azInterval < 1 || azInterval > 3600 ||
        invalidCredentialText(azUser, 64) || invalidCredentialText(azPassword, 128)) {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid source setting\"}");
        return;
    }

    if (_sourceConfigCallback) {
        GoodWeConfig goodwe;
        goodwe.enabled = gwEnabled;
        goodwe.host = gwHost;
        goodwe.port = static_cast<uint16_t>(gwPort);
        goodwe.pollIntervalSeconds = static_cast<uint32_t>(gwInterval);

        AZRouterConfig azrouter = _config.azrouter;
        azrouter.enabled = azEnabled;
        azrouter.host = azHost;
        azrouter.port = static_cast<uint16_t>(azPort);
        azrouter.pollIntervalSeconds = static_cast<uint32_t>(azInterval);
        azrouter.authEnabled = azAuthEnabled;
        azrouter.username = azUser;

        // Never send the stored password back to the browser. An empty field
        // means "keep current password". Clearing the username switches back
        // to anonymous mode and clears the stored password as well.
        if (azUser.isEmpty()) {
            azrouter.password = "";
        } else if (!azPassword.isEmpty()) {
            azrouter.password = azPassword;
        }

        _server.send(200, "application/json", "{\"status\":\"saved\"}");
        _sourceConfigCallback(goodwe, azrouter);
        return;
    }

    _server.send(503, "application/json", "{\"status\":\"error\",\"message\":\"Source configuration unavailable\"}");
}

void DashboardWebServer::handleApiCheckForUpdate() {
    _githubUpdateVersion = "";
    _githubUpdateUrl = "";
    _githubUpdateSha256 = "";

    const String manifestUrl = String(GITHUB_LATEST_DOWNLOAD_BASE) + GITHUB_MANIFEST_ASSET_NAME;
    Serial.printf("[OTA] Checking GitHub manifest: %s (free heap: %u)\n",
                  manifestUrl.c_str(), static_cast<unsigned int>(ESP.getFreeHeap()));

    String payload;
    if (!httpGetString(manifestUrl, payload)) {
        Serial.println("[OTA] GitHub manifest download failed");
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"GitHub manifest download failed\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[OTA] Invalid manifest JSON: %s\n", err.c_str());
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"Invalid GitHub manifest\"}");
        return;
    }

    const char* version = doc["firmware"]["version"] | "";
    const char* manifestSha256 = doc["firmware"]["sha256"] | "";
    if (version[0] == '\0' || manifestSha256[0] == '\0') {
        Serial.println("[OTA] Manifest is missing firmware version or SHA-256");
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"GitHub manifest is incomplete\"}");
        return;
    }

    String sha256;
    if (!normalizeSha256(String(manifestSha256), sha256)) {
        Serial.println("[OTA] Manifest contains invalid SHA-256");
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"GitHub manifest has invalid SHA-256\"}");
        return;
    }

    const String latestVersion(version);
    const String downloadUrl = String(GITHUB_LATEST_DOWNLOAD_BASE) + GITHUB_FIRMWARE_ASSET_NAME;
    const bool updateAvailable = compareVersions(FIRMWARE_VERSION, latestVersion);

    Serial.printf("[OTA] Version check: current=%s latest=%s update=%s (free heap: %u)\n",
                  FIRMWARE_VERSION, latestVersion.c_str(), updateAvailable ? "yes" : "no",
                  static_cast<unsigned int>(ESP.getFreeHeap()));

    if (updateAvailable) {
        _githubUpdateVersion = latestVersion;
        _githubUpdateUrl = downloadUrl;
        _githubUpdateSha256 = sha256;
    }
    _server.send(200, "application/json", buildGithubReleaseCheckJson(updateAvailable, latestVersion, downloadUrl, sha256));
}

void DashboardWebServer::handleApiGithubUpdate() {
    if (_githubUpdateUrl.isEmpty() || _githubUpdateSha256.isEmpty() ||
        !compareVersions(FIRMWARE_VERSION, _githubUpdateVersion)) {
        _server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Check for a newer GitHub release first\"}");
        return;
    }

    if (!downloadFirmwareBinary(_githubUpdateUrl, _githubUpdateSha256)) {
        Serial.println("[OTA] GitHub binary download failed");
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Firmware download or validation failed\"}");
        return;
    }

    Serial.printf("[OTA] GitHub firmware %s installed, restarting...\n", _githubUpdateVersion.c_str());
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"GitHub firmware updated; restarting\"}");
    delay(500);
    ESP.restart();
}
void DashboardWebServer::handleApiUpdateUpload() {
    _otaManager.handleUpload(_server.upload());
}

void DashboardWebServer::handleApiUpdateComplete() {
    if (!_otaManager.finish(_server.arg("sha256"))) {
        Serial.printf("[OTA] Upload failed: %s\n", _otaManager.error().c_str());
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"" + _otaManager.error() + "\"}");
        return;
    }

    Serial.println("[OTA] Firmware upload succeeded, restarting...");
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Firmware updated; restarting\"}");
    delay(500);
    ESP.restart();
}

void DashboardWebServer::handleApiPoolConfig() {
    if (!_server.hasArg("enabled")) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Missing pool setting\"}");
        return;
    }

    const String enabled = _server.arg("enabled");
    if (enabled != "0" && enabled != "1") {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Invalid pool setting\"}");
        return;
    }

    if (_poolConfigCallback) {
        PoolConfig pool;
        pool.enabled = enabled == "1";
        _server.send(200, "application/json", "{\"status\":\"saved\"}");
        _poolConfigCallback(pool);
        return;
    }

    _server.send(503, "application/json",
                 "{\"status\":\"error\",\"message\":\"Pool configuration unavailable\"}");
}

void DashboardWebServer::handleApiWeatherConfig() {
    const char* required[] = {"provider", "interval"};
    for (const char* name : required) {
        if (!_server.hasArg(name) || _server.arg(name).isEmpty()) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Missing weather setting\"}");
            return;
        }
    }

    const String provider = _server.arg("provider");
    const long interval = _server.arg("interval").toInt();
    if ((provider != "open-meteo" && provider != "met-no") ||
        interval < 900 || interval > 21600) {
        _server.send(400, "application/json",
                     "{\"status\":\"error\",\"message\":\"Invalid weather setting\"}");
        return;
    }

    WeatherConfig weather;
    weather.enabled = _server.arg("enabled") == "1";
    weather.provider = provider;
    weather.pollIntervalSeconds = static_cast<uint32_t>(interval);

    auto invalidText = [](const String& value, size_t maxLength, bool allowEmpty) {
        if ((!allowEmpty && value.isEmpty()) || value.length() > maxLength) return true;
        for (size_t i = 0; i < value.length(); ++i) {
            if (static_cast<uint8_t>(value[i]) < 0x20) return true;
        }
        return false;
    };

    if (_server.hasArg("locations")) {
        JsonDocument locationsDoc;
        if (deserializeJson(locationsDoc, _server.arg("locations"))) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Invalid weather locations\"}");
            return;
        }
        JsonArray locations = locationsDoc.as<JsonArray>();
        if (locations.isNull() || locations.size() == 0 || locations.size() > MaxWeatherLocations ||
            !_server.hasArg("activeLocationId")) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Invalid weather locations\"}");
            return;
        }

        weather.locationCount = 0;
        for (JsonObject item : locations) {
            const String id = item["id"] | "";
            const String name = item["name"] | "";
            const String country = item["country"] | "";
            const double latitude = item["latitude"] | 999.0;
            const double longitude = item["longitude"] | 999.0;
            if (invalidText(id, 32, false) || invalidText(name, 80, false) ||
                invalidText(country, 64, true) ||
                latitude < -90.0 || latitude > 90.0 ||
                longitude < -180.0 || longitude > 180.0 ||
                (latitude == 0.0 && longitude == 0.0)) {
                _server.send(400, "application/json",
                             "{\"status\":\"error\",\"message\":\"Invalid weather location\"}");
                return;
            }
            for (uint8_t i = 0; i < weather.locationCount; ++i) {
                if (weather.locations[i].id == id) {
                    _server.send(400, "application/json",
                                 "{\"status\":\"error\",\"message\":\"Duplicate weather location\"}");
                    return;
                }
            }
            auto& location = weather.locations[weather.locationCount++];
            location.id = id;
            location.name = name;
            location.country = country;
            location.latitude = latitude;
            location.longitude = longitude;
        }

        weather.activeLocationId = _server.arg("activeLocationId");
        bool activeFound = false;
        for (uint8_t i = 0; i < weather.locationCount; ++i) {
            if (weather.locations[i].id == weather.activeLocationId) {
                activeFound = true;
                break;
            }
        }
        if (!activeFound) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Active weather location not found\"}");
            return;
        }
        weather.syncActiveCoordinates();
    } else {
        // Kompatibilita se starším WebUI / API s jedinou lokalitou.
        if (!_server.hasArg("latitude") || !_server.hasArg("longitude") ||
            _server.arg("latitude").isEmpty() || _server.arg("longitude").isEmpty()) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Missing weather location\"}");
            return;
        }
        const double latitude = _server.arg("latitude").toDouble();
        const double longitude = _server.arg("longitude").toDouble();
        if (latitude < -90.0 || latitude > 90.0 ||
            longitude < -180.0 || longitude > 180.0 ||
            (latitude == 0.0 && longitude == 0.0)) {
            _server.send(400, "application/json",
                         "{\"status\":\"error\",\"message\":\"Invalid weather location\"}");
            return;
        }
        weather.latitude = latitude;
        weather.longitude = longitude;
        weather.locationCount = 1;
        weather.locations[0].id = "legacy";
        weather.locations[0].name = "Uložené místo";
        weather.locations[0].latitude = latitude;
        weather.locations[0].longitude = longitude;
        weather.activeLocationId = weather.locations[0].id;
    }

    if (_weatherConfigCallback) {
        _server.send(200, "application/json", "{\"status\":\"saved\"}");
        _weatherConfigCallback(weather);
        return;
    }

    _server.send(503, "application/json",
                 "{\"status\":\"error\",\"message\":\"Weather configuration unavailable\"}");
}