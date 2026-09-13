#include "OtaManager.h"
#include <Update.h>

void OtaManager::handleUpload(HTTPUpload& upload) {
    switch (upload.status) {
        case UPLOAD_FILE_START:
            _succeeded = false;
            _error = "";
            {
                const uint32_t totalSize = (upload.totalSize > 0) ? upload.totalSize : UPDATE_SIZE_UNKNOWN;
                if (!Update.begin(totalSize)) {
                    _error = Update.errorString();
                }
            }
            break;

        case UPLOAD_FILE_WRITE:
            if (_error.isEmpty() && Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                _error = Update.errorString();
            }
            break;

        case UPLOAD_FILE_END:
            if (_error.isEmpty()) {
                _succeeded = Update.end();
                if (!_succeeded) {
                    _error = Update.errorString();
                }
            }
            break;

        case UPLOAD_FILE_ABORTED:
            abort();
            _error = "Upload aborted";
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