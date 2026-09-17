#!/usr/bin/env python3
"""Apply the OTA updater migration directly to WebServer.cpp.

This one-shot repository maintenance script exists so the source change can be
applied reproducibly by GitHub Actions. It is safe to run repeatedly: once the
new code is present it exits without changing the file.
"""

from pathlib import Path

PATH = Path("src/network/WebServer.cpp")

OLD_HEADER = r'''    static const char* GITHUB_RELEASE_API_URL = "https://api.github.com/repos/PetrSindelarHCZ/PVDashboard/releases/latest";
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
'''

NEW_HEADER = r'''    static const char* GITHUB_LATEST_DOWNLOAD_BASE = "https://github.com/PetrSindelarHCZ/PVDashboard/releases/latest/download/";
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
'''

OLD_HANDLER = r'''void DashboardWebServer::handleApiCheckForUpdate() {
    _githubUpdateVersion = "";
    _githubUpdateUrl = "";
    _githubUpdateSha256 = "";

    String payload;
    if (!httpGetString(String(GITHUB_RELEASE_API_URL), payload)) {
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"GitHub release check failed\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"Invalid GitHub release response\"}");
        return;
    }

    const char* tagName = doc["tag_name"] | "";
    if (tagName[0] == '\0') {
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"GitHub release has no version\"}");
        return;
    }

    String latestVersion = String(tagName);
    if (latestVersion.startsWith("v")) latestVersion.remove(0, 1);

    JsonArray assets = doc["assets"].as<JsonArray>();
    String downloadUrl = findGitHubAssetDownloadUrl(assets, String(GITHUB_FIRMWARE_ASSET_NAME));
    String sha256 = findGitHubAssetSha256(assets, String(GITHUB_FIRMWARE_ASSET_NAME));
    if (downloadUrl.isEmpty() || sha256.isEmpty()) {
        _server.send(502, "application/json", "{\"status\":\"error\",\"message\":\"GitHub release assets are incomplete\"}");
        return;
    }

    const bool updateAvailable = compareVersions(FIRMWARE_VERSION, latestVersion);
    if (updateAvailable) {
        _githubUpdateVersion = latestVersion;
        _githubUpdateUrl = downloadUrl;
        _githubUpdateSha256 = sha256;
    }
    _server.send(200, "application/json", buildGithubReleaseCheckJson(updateAvailable, latestVersion, downloadUrl, sha256));
}
'''

NEW_HANDLER = r'''void DashboardWebServer::handleApiCheckForUpdate() {
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
'''


def replace_once(text: str, old: str, new: str, label: str) -> tuple[str, bool]:
    if new in text:
        return text, False
    if old not in text:
        raise RuntimeError(f"Could not locate expected {label} block")
    return text.replace(old, new, 1), True


def main() -> int:
    text = PATH.read_text(encoding="utf-8")
    changed = False
    text, did_change = replace_once(text, OLD_HEADER, NEW_HEADER, "version comparison")
    changed |= did_change
    text, did_change = replace_once(text, OLD_HANDLER, NEW_HANDLER, "GitHub update check")
    changed |= did_change

    if changed:
        PATH.write_text(text, encoding="utf-8", newline="\n")
        print(f"Updated {PATH}")
    else:
        print(f"{PATH} already contains the OTA migration")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
