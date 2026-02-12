#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "config.h"

class App;

class HoloWebServer
{
public:
    HoloWebServer(uint16_t port = 80);

    void begin();
    void setOnGifChange(void (*callback)());
    void setOnModeChange(void (*callback)(int));
    void setAppInfo(App **apps, const int *appCount, int *currentIndex);

    String getLocalIP();
    bool isUploading() const { return _isUploading; }
    void checkUploadTimeout();

private:
    AsyncWebServer _server;
    void (*_onGifChange)();
    void (*_onModeChange)(int);

    App **_apps;
    const int *_appCount;
    int *_currentIndex;

    void setupRoutes();

    void handleRoot(AsyncWebServerRequest *request);
    void handleGetGifs(AsyncWebServerRequest *request);
    void handleGetGifInfo(AsyncWebServerRequest *request);
    void handleDeleteGif(AsyncWebServerRequest *request);
    void handleReorder(AsyncWebServerRequest *request, JsonVariant &json);
    void handleCreateGif(AsyncWebServerRequest *request, JsonVariant &json);
    void handleUploadFrame(AsyncWebServerRequest *request, const String &filename,
                           size_t index, uint8_t *data, size_t len, bool final);
    void handleGetFrame(AsyncWebServerRequest *request);
    void handleUploadOriginal(AsyncWebServerRequest *request, const String &filename,
                              size_t index, uint8_t *data, size_t len, bool final);
    void handleGetOriginal(AsyncWebServerRequest *request);
    void handleWifiPage(AsyncWebServerRequest *request);
    void handleGetWifi(AsyncWebServerRequest *request);
    void handleSaveWifi(AsyncWebServerRequest *request, JsonVariant &json);
    void handleNowPlaying(AsyncWebServerRequest *request, JsonVariant &json);
    void handleGetNowPlaying(AsyncWebServerRequest *request);
    void handleUploadNpFrame(AsyncWebServerRequest *request, const String &filename,
                             size_t index, uint8_t *data, size_t len, bool final);
    void handleNpReady(AsyncWebServerRequest *request);
    void handleSetMode(AsyncWebServerRequest *request, JsonVariant &json);
    void handleGetMode(AsyncWebServerRequest *request);
    void abortUpload();

    char _uploadPath[64];
    int _uploadFrameIndex;
    File _uploadFile;
    File _uploadOriginalFile;
    volatile bool _isUploading = false;
    volatile bool _uploadError = false;
    volatile unsigned long _lastUploadMs = 0;
    volatile bool _fileOpen = false;
    volatile bool _origFileOpen = false;
};

extern HoloWebServer webServer;

#endif // WEB_SERVER_H