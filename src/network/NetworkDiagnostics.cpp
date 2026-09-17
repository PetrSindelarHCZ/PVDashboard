#include "NetworkDiagnostics.h"
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <cstring>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <ping/ping_sock.h>

namespace {
struct PingContext {
    volatile bool finished = false;
    volatile bool success = false;
    volatile uint32_t elapsedMs = 0;
};

PingContext gPingContext;

void onPingSuccess(esp_ping_handle_t handle, void* args) {
    auto* context = static_cast<PingContext*>(args);
    if (!context) return;
    uint32_t elapsed = 0;
    esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed, sizeof(elapsed));
    context->elapsedMs = elapsed;
    context->success = true;
}

void onPingTimeout(esp_ping_handle_t, void* args) {
    auto* context = static_cast<PingContext*>(args);
    if (!context) return;
    context->success = false;
}

void onPingEnd(esp_ping_handle_t, void* args) {
    auto* context = static_cast<PingContext*>(args);
    if (!context) return;
    context->finished = true;
}

bool resolveIpv4(const String& host, ip_addr_t& target, String& resolvedIp) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0 || !result) return false;

    const auto* addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
    const struct in_addr addr4 = addr->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target), &addr4);

    char buffer[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr4, buffer, sizeof(buffer));
    resolvedIp = buffer;
    freeaddrinfo(result);
    return true;
}

bool pingTarget(const ip_addr_t& target, uint32_t timeoutMs, uint32_t& elapsedMs) {
    gPingContext.finished = false;
    gPingContext.success = false;
    gPingContext.elapsedMs = 0;

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target;
    config.count = 1;
    config.interval_ms = 100;
    config.timeout_ms = timeoutMs;

    esp_ping_callbacks_t callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.on_ping_success = onPingSuccess;
    callbacks.on_ping_timeout = onPingTimeout;
    callbacks.on_ping_end = onPingEnd;
    callbacks.cb_args = &gPingContext;

    esp_ping_handle_t handle = nullptr;
    if (esp_ping_new_session(&config, &callbacks, &handle) != ESP_OK || !handle) return false;
    if (esp_ping_start(handle) != ESP_OK) {
        esp_ping_delete_session(handle);
        return false;
    }

    const uint32_t started = millis();
    const uint32_t guardMs = timeoutMs + 350;
    while (!gPingContext.finished && millis() - started < guardMs) delay(5);

    if (!gPingContext.finished) {
        esp_ping_stop(handle);
        const uint32_t stopStarted = millis();
        while (!gPingContext.finished && millis() - stopStarted < 100) delay(2);
    }

    const bool success = gPingContext.success;
    elapsedMs = gPingContext.elapsedMs;
    esp_ping_delete_session(handle);
    delay(2);
    return success;
}

bool testTcpPort(const String& host, uint16_t port, uint32_t timeoutMs, uint32_t& elapsedMs) {
    WiFiClient client;
    const uint32_t started = millis();
    const bool connected = client.connect(host.c_str(), port, static_cast<int32_t>(timeoutMs));
    elapsedMs = millis() - started;
    client.stop();
    return connected;
}

bool testGoodWeUdpPort(const String& resolvedIp, uint16_t port, uint32_t timeoutMs, uint32_t& elapsedMs) {
    IPAddress remote;
    if (!remote.fromString(resolvedIp)) return false;

    // Stejný bezpečný read-only Modbus dotaz, který používá GoodWeClient.
    const uint8_t request[8] = {0xF7, 0x03, 0x89, 0x1C, 0x00, 0x7D, 0x7A, 0xE7};
    WiFiUDP udp;
    if (!udp.begin(0)) return false;

    const uint32_t started = millis();
    if (!udp.beginPacket(remote, port)) {
        udp.stop();
        return false;
    }
    udp.write(request, sizeof(request));
    if (!udp.endPacket()) {
        udp.stop();
        return false;
    }

    bool response = false;
    while (millis() - started < timeoutMs) {
        const int packetSize = udp.parsePacket();
        if (packetSize > 0) {
            response = udp.remoteIP() == remote && udp.remotePort() == port;
            udp.flush();
            if (response) break;
        }
        delay(5);
    }
    elapsedMs = millis() - started;
    udp.stop();
    return response;
}
}

NetworkProbeResult NetworkDiagnostics::probe(const String& host,
                                             uint16_t port,
                                             NetworkProbeTransport transport,
                                             uint32_t pingTimeoutMs,
                                             uint32_t portTimeoutMs) {
    NetworkProbeResult result;
    if (host.isEmpty() || port == 0) {
        result.error = "Invalid host or port";
        return result;
    }

    ip_addr_t target;
    memset(&target, 0, sizeof(target));
    if (!resolveIpv4(host, target, result.resolvedIp)) {
        result.error = "DNS/host resolution failed";
        return result;
    }
    result.resolved = true;
    result.pingOk = pingTarget(target, pingTimeoutMs, result.pingMs);

    if (transport == NetworkProbeTransport::GoodWeUdp) {
        result.portProtocol = "UDP";
        result.portOpen = testGoodWeUdpPort(result.resolvedIp, port, portTimeoutMs, result.portConnectMs);
    } else {
        result.portProtocol = "TCP";
        result.portOpen = testTcpPort(host, port, portTimeoutMs, result.portConnectMs);
    }

    if (!result.pingOk && !result.portOpen) {
        result.error = "No ICMP reply and configured port is unreachable";
    } else if (!result.pingOk && result.portOpen) {
        result.error = "ICMP reply missing; configured port is reachable";
    } else if (result.pingOk && !result.portOpen) {
        result.error = "Host replies to ICMP, but configured port is unreachable";
    }
    return result;
}
