#include "OtaManager.h"
#include <Update.h>

void OtaManager::handleUpload(HTTPUpload& upload) {
    switch (upload.status) {
        case UPLOAD_FILE_START:
            _succeeded = false;
            _error = "";
            {
                const uint32_t totalSize = (upload.totalSize > 0) ? upload.totalSize : UPDATE_SIZE_UNKNOWN;
                Serial.printf("[OTA] Upload start: totalSize=%u, currentSize=%u, filename=%s\n",
                              (unsigned int)upload.totalSize,
                              (unsigned int)upload.currentSize,
                              upload.filename.c_str());
                if (!Update.begin(totalSize)) {
                    _error = Update.errorString();
                    Serial.printf("[OTA] Update.begin failed: %s\n", _error.c_str());
                }
            }
            break;

        case UPLOAD_FILE_WRITE:
            if (_error.isEmpty()) {
                const size_t written = Update.write(upload.buf, upload.currentSize);
                Serial.printf("[OTA] Write chunk: currentSize=%u, written=%u, expected=%u\n",
                              (unsigned int)upload.currentSize,
                              (unsigned int)written,
                              (unsigned int)upload.currentSize);
                if (written != upload.currentSize) {
                    _error = Update.errorString();
                    Serial.printf("[OTA] Write mismatch: %s\n", _error.c_str());
                }
            }
            break;

        case UPLOAD_FILE_END:
            if (_error.isEmpty()) {
                Serial.printf("[OTA] Upload end: finalSize=%u\n", (unsigned int)upload.currentSize);
                _succeeded = Update.end();
                if (!_succeeded) {
                    _error = Update.errorString();
                    Serial.printf("[OTA] Update.end failed: %s\n", _error.c_str());
                } else {
                    Serial.println("[OTA] Update.end succeeded");
                }
            }
            break;

        case UPLOAD_FILE_ABORTED:
            abort();
            _error = "Upload aborted";
            Serial.println("[OTA] Upload aborted by client");
            break;

        default:
            break;
    }
}

bool OtaManager::finish() {
    if (!_succeeded && _error.isEmpty()) {
        _error = "Incomplete firmware upload";
    }
    return _succeeded;
}

void OtaManager::abort() {
    if (Update.isRunning()) Update.abort();
    _succeeded = false;
}

bool OtaManager::succeeded() const {
    return _succeeded;
}

const String& OtaManager::error() const {
    return _error;
}