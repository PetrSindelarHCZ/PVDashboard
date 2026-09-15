#include "OtaManager.h"
#include <Update.h>

void OtaManager::releaseHashContext() {
    if (_hashActive) {
        mbedtls_sha256_free(&_hashContext);
        _hashActive = false;
    }
}

void OtaManager::handleUpload(HTTPUpload& upload) {
    switch (upload.status) {
        case UPLOAD_FILE_START:
            abort();
            _error = "";
            _received = 0;
            _actualSha256 = "";
            Serial.printf("[OTA] Upload start: filename=%s\n", upload.filename.c_str());
            mbedtls_sha256_init(&_hashContext);
            _hashActive = true;
            if (mbedtls_sha256_starts_ret(&_hashContext, 0) != 0) {
                _error = "SHA-256 initialization failed";
                abort();
            } else if (!Update.begin(FirmwareLimits::MaxImageBytes)) {
                _error = Update.errorString();
                abort();
            }
            break;

        case UPLOAD_FILE_WRITE:
            if (!_error.isEmpty()) break;
            if (_received + upload.currentSize > FirmwareLimits::MaxImageBytes) {
                _error = "Firmware exceeds OTA partition";
                abort();
                break;
            }
            if (mbedtls_sha256_update_ret(&_hashContext, upload.buf, upload.currentSize) != 0) {
                _error = "SHA-256 calculation failed";
                abort();
                break;
            }
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                _error = Update.errorString();
                abort();
            } else {
                _received += upload.currentSize;
            }
            break;

        case UPLOAD_FILE_END:
            if (_error.isEmpty()) {
                uint8_t digest[32];
                if (mbedtls_sha256_finish_ret(&_hashContext, digest) != 0) {
                    _error = "SHA-256 calculation failed";
                    abort();
                    break;
                }
                releaseHashContext();
                static const char hex[] = "0123456789abcdef";
                char hash[65];
                for (size_t i = 0; i < sizeof(digest); ++i) {
                    hash[2 * i] = hex[digest[i] >> 4];
                    hash[2 * i + 1] = hex[digest[i] & 0x0f];
                }
                hash[64] = '\0';
                _actualSha256 = hash;
                _uploadComplete = true;
                Serial.printf("[OTA] Upload received: %u bytes; awaiting SHA-256 validation\n",
                              (unsigned int)_received);
            }
            break;

        case UPLOAD_FILE_ABORTED:
            _error = "Upload aborted";
            abort();
            break;

        default:
            break;
    }
}

bool OtaManager::finish(const String& expectedSha256) {
    if (_succeeded) return true;
    if (!_error.isEmpty()) return false;
    if (!_uploadComplete || _received == 0) {
        _error = "Incomplete firmware upload";
        abort();
        return false;
    }

    String expected = expectedSha256;
    expected.trim();
    expected.toLowerCase();
    bool validHash = expected.length() == 64;
    for (size_t i = 0; validHash && i < expected.length(); ++i) {
        const char c = expected[i];
        validHash = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    }
    if (!validHash || expected != _actualSha256) {
        _error = validHash ? "SHA-256 mismatch" : "Missing or invalid SHA-256";
        abort();
        return false;
    }

    _succeeded = Update.end(true);
    if (!_succeeded) {
        _error = Update.errorString();
        abort();
    }
    return _succeeded;
}

void OtaManager::abort() {
    if (Update.isRunning()) Update.abort();
    releaseHashContext();
    _uploadComplete = false;
    _succeeded = false;
}

bool OtaManager::succeeded() const {
    return _succeeded;
}

const String& OtaManager::error() const {
    return _error;
}
