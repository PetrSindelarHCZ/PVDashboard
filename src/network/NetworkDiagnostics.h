#pragma once
#include <Arduino.h>

enum class NetworkProbeTransport : uint8_t {
    Tcp,
    GoodWeUdp
};

struct NetworkProbeResult {
    bool resolved = false;
    String resolvedIp;
    bool pingOk = false;
    uint32_t pingMs = 0;
    bool portOpen = false;
    uint32_t portConnectMs = 0;
    String portProtocol;
    String error;
};

class NetworkDiagnostics {
public:
    static NetworkProbeResult probe(const String& host,
                                    uint16_t port,
                                    NetworkProbeTransport transport,
                                    uint32_t pingTimeoutMs = 450,
                                    uint32_t portTimeoutMs = 650);
};
