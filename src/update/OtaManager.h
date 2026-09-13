#pragma once

#include <Arduino.h>
#include <WebServer.h>

class OtaManager {
public:
    void handleUpload(HTTPUpload& upload);
    bool finish();
    void abort();
    bool succeeded() const;
    const String& error() const;

private:
    bool _succeeded = false;
    String _error;
};