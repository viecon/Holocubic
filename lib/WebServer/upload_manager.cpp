#include "upload_manager.h"

UploadManager uploadManager;

bool isUploadActive() { return uploadManager.isUploading(); }

bool UploadManager::consumeError()
{
    if (_uploadError)
    {
        _uploadError = false;
        return true;
    }
    return false;
}

bool UploadManager::openFile(const char *path)
{
    _file = SD.open(path, FILE_WRITE);
    if (!_file)
    {
        Serial.printf("[Upload] Cannot create: %s\n", path);
        _uploadError = true;
        return false;
    }
    strncpy(_path, path, sizeof(_path) - 1);
    _path[sizeof(_path) - 1] = '\0';
    _fileOpen = true;
    return true;
}

bool UploadManager::writeChunk(const uint8_t *data, size_t len)
{
    if (!_fileOpen || _uploadError)
        return false;
    _lastUploadMs = millis();
    size_t written = _file.write(data, len);
    if (written != len)
    {
        Serial.printf("[Upload] Write error: wrote %u/%u\n", written, len);
        _uploadError = true;
        return false;
    }
    return true;
}

void UploadManager::closeFile()
{
    if (_fileOpen)
    {
        _file.close();
        _fileOpen = false;
    }
}

bool UploadManager::openOriginal(const char *path)
{
    _originalFile = SD.open(path, FILE_WRITE);
    if (!_originalFile)
    {
        Serial.printf("[Upload] Cannot create original: %s\n", path);
        _uploadError = true;
        return false;
    }
    _origFileOpen = true;
    return true;
}

bool UploadManager::writeOriginalChunk(const uint8_t *data, size_t len)
{
    if (!_origFileOpen || _uploadError)
        return false;
    _lastUploadMs = millis();
    size_t written = _originalFile.write(data, len);
    if (written != len)
    {
        Serial.printf("[Upload] Original write error: wrote %u/%u\n", written, len);
        _uploadError = true;
        return false;
    }
    return true;
}

void UploadManager::closeOriginal()
{
    if (_origFileOpen)
    {
        _originalFile.close();
        _origFileOpen = false;
    }
}

void UploadManager::setUploading(bool uploading)
{
    _isUploading = uploading;
}

void UploadManager::setError(bool error)
{
    _uploadError = error;
}

void UploadManager::touchTimestamp()
{
    _lastUploadMs = millis();
}

void UploadManager::abort()
{
    closeFile();
    closeOriginal();
    _isUploading = false;
    _uploadError = false;
    Serial.printf("[Upload] Aborted. Free heap: %u\n", ESP.getFreeHeap());
}

void UploadManager::checkTimeout()
{
    if (_isUploading && _lastUploadMs > 0 &&
        (millis() - _lastUploadMs) > UPLOAD_TIMEOUT_MS)
    {
        Serial.printf("[Upload] Timeout (%us idle), auto-recovering\n",
                      UPLOAD_TIMEOUT_MS / 1000);
        _isUploading = false;
        _lastUploadMs = 0;
    }
}
