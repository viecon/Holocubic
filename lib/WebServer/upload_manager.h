#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include "config.h"

class UploadManager
{
public:
    bool isUploading() const { return _isUploading; }
    bool isFileOpen() const { return _fileOpen; }

    // Returns current error state and clears it (for response lambdas)
    bool consumeError();

    // Primary file upload
    bool openFile(const char *path);
    bool writeChunk(const uint8_t *data, size_t len);
    void closeFile();

    // Secondary file upload (GIF original)
    bool openOriginal(const char *path);
    bool writeOriginalChunk(const uint8_t *data, size_t len);
    void closeOriginal();

    void setUploading(bool uploading);
    void setError(bool error);
    void touchTimestamp();

    // Safe abort — only call from same task context as upload handler
    void abort();

    // Call from main loop — only sets flags, never closes files
    void checkTimeout();

private:
    File _file;
    File _originalFile;
    char _path[64];
    volatile bool _isUploading = false;
    volatile bool _uploadError = false;
    volatile unsigned long _lastUploadMs = 0;
    volatile bool _fileOpen = false;
    volatile bool _origFileOpen = false;
};

extern UploadManager uploadManager;

#endif // UPLOAD_MANAGER_H
