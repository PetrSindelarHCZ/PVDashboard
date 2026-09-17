#pragma once
#include <Arduino.h>

struct NetworkProbeResult {
    bool resolved = false;
    String resolvedIp;
    bool pingOk = false;
    uint32_t pingMs = 0;
    bool portOpen = false;
    uint32_t portConnectMs = 0;
    String error;
};

class NetworkDiagnostics {
public:
    static NetworkProbeResult probe(const String& host,
                                    uint16_t port,
                                    uint32_t pingTimeoutMs = 450,
                                    uint32_t tcpTimeoutMs = 650);
};
