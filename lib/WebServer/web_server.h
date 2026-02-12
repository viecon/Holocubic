#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
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

    const char *getLocalIP();
    bool isUploading() const;
    void checkUploadTimeout();

private:
    AsyncWebServer _server;
    void (*_onModeChange)(int);

    App **_apps;
    const int *_appCount;
    int *_currentIndex;
    char _ipBuf[16];

    void setupRoutes();
};

extern HoloWebServer webServer;

#endif // WEB_SERVER_H