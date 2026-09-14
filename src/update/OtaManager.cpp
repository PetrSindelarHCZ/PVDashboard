#include "OtaManager.h"
#include <Update.h>

void OtaManager::handleUpload(HTTPUpload& upload) {
    switch (upload.status) {
        case UPLOAD_FILE_START:
            _succeeded = false;
            _error = "";
            _received = 0;
            Serial.printf("[OTA] Upload start: reportedTotal=%u, currentSize=%u, filename=%s\n",
                          (unsigned int)upload.totalSize,
                          (unsigned int)upload.currentSize,
                          upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                _error = Update.errorString();
                Serial.printf("[OTA] Update.begin failed: %s\n", _error.c_str());
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
                } else {
                    _received += written;
                }
            }
            break;

        case UPLOAD_FILE_END:
            if (_error.isEmpty()) {
                Serial.printf("[OTA] Upload end: finalChunk=%u, received=%u\n",
                              (unsigned int)upload.currentSize,
                              (unsigned int)_received);
                _succeeded = Update.end(true);
                if (!_succeeded) {
                    _error = Update.errorString();
                    Serial.printf("[OTA] Update.end failed: %s\n", _error.c_str());
                } else {
                    Serial.println("[OTA] Update.end succeeded");
                }
            }
            break;

        case UPLOAD_FILE_ABORTED:
            {
                const String updateError = Update.errorString();
                const size_t received = _received;
                abort();
                _error = updateError.isEmpty() ? "Upload aborted" : updateError;
                Serial.printf("[OTA] Upload aborted by client: received=%u, updateError=%s\n",
                              (unsigned int)received,
                              _error.c_str());
            }
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
    if (Update.isRunning()) {
        Update.abort();
        if (_error.isEmpty()) {
            _error = Update.errorString();
        }
    }
    _succeeded = false;
}

bool OtaManager::succeeded() const {
    return _succeeded;
}

const String& OtaManager::error() const {
    return _error;
}