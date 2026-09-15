#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <mbedtls/sha256.h>
#include "../../include/FirmwareLimits.h"

class OtaManager {
public:
    void handleUpload(HTTPUpload& upload);
    bool finish(const String& expectedSha256);
    void abort();
    bool succeeded() const;
    const String& error() const;

private:
    void releaseHashContext();

    mbedtls_sha256_context _hashContext{};
    bool _hashActive = false;
    bool _uploadComplete = false;
    String _actualSha256;
    bool _succeeded = false;
    size_t _received = 0;
    String _error;
};
