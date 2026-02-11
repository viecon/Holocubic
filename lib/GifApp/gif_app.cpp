#include "gif_app.h"
#include "display.h"
#include "web_server.h"
#include "mpu.h"
#include <WiFi.h>

GifApp gifApp;

// Static members
TaskHandle_t GifApp::_loaderTask = NULL;
volatile bool GifApp::_frameLoaded = false;
volatile bool GifApp::_loaderBusy = false;
char GifApp::_nextFramePath[64] = {0};

// Time string cache (shared utility)
static char _cachedTimeStr[6] = "--:--";
static unsigned long _lastTimeUpdate = 0;
static bool _timeSynced = false;

static const char *getTimeString()
{
    unsigned long now = millis();
    unsigned long interval = _timeSynced ? 10000 : 1000;
    if (now - _lastTimeUpdate >= interval)
    {
        _lastTimeUpdate = now;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 10))
        {
            strftime(_cachedTimeStr, sizeof(_cachedTimeStr), "%H:%M", &timeinfo);
            _timeSynced = true;
        }
    }
    return _cachedTimeStr;
}

GifApp::GifApp()
    : _currentIndex(0), _currentFrame(0), _lastFrameTime(0),
      _needRefresh(false)
{
    _currentGif.valid = false;
}

void GifApp::frameLoaderTask(void *param)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (webServer.isUploading())
            continue;

        _loaderBusy = true;
        if (display.decodeBmpToCanvas(_nextFramePath))
        {
            _frameLoaded = true;
        }
        _loaderBusy = false;
    }
}

void GifApp::onEnter()
{
    Serial.println("[GifApp] Enter");

    // Start frame loader task if not already running
    if (_loaderTask == NULL)
    {
        xTaskCreatePinnedToCore(
            frameLoaderTask,
            "FrameLoader",
            4096,
            NULL,
            1,
            &_loaderTask,
            0);
        Serial.println("[GifApp] Frame loader started on Core 0");
    }

    loadGif();
}

void GifApp::onExit()
{
    Serial.println("[GifApp] Exit");
    // Loader task stays alive but idle (waiting on notify)
}

void GifApp::loop()
{
    if (_needRefresh)
    {
        _needRefresh = false;
        gifManager.refresh();

        int count = gifManager.getGifCount();
        if (count == 0)
        {
            _currentGif.valid = false;
            _currentIndex = 0;
            display.clear();
            display.showMessage("No GIFs");
            display.showIP(webServer.getLocalIP().c_str());
        }
        else
        {
            if (_currentIndex >= count)
                _currentIndex = 0;
            loadGif();
        }
    }

    if (_currentGif.valid && !webServer.isUploading())
    {
        playFrame();
    }
}

bool GifApp::onTilt(int direction)
{
    int count = gifManager.getGifCount();
    if (count <= 0)
        return false;

    _currentIndex += direction;
    if (_currentIndex >= count)
        _currentIndex = 0;
    if (_currentIndex < 0)
        _currentIndex = count - 1;

    loadGif();
    Serial.printf("[GifApp] Switched to GIF %d: %s\n", _currentIndex, _currentGif.name.c_str());
    return true;
}

void GifApp::notifyGifChange()
{
    _needRefresh = true;
}

void GifApp::loadGif()
{
    Serial.printf("[GifApp] Loading GIF index %d...\n", _currentIndex);

    while (_loaderBusy)
        delay(1);

    if (!gifManager.getGifInfoByIndex(_currentIndex, _currentGif))
    {
        Serial.println("[GifApp] Failed to get GIF info!");
        _currentGif.valid = false;
        return;
    }

    _currentFrame = 0;
    _lastFrameTime = millis();
    _frameLoaded = false;

    snprintf(_nextFramePath, sizeof(_nextFramePath), "/gifs/%s/%d.bmp",
             _currentGif.name.c_str(), _currentFrame);

    if (_loaderTask != NULL)
    {
        xTaskNotifyGive(_loaderTask);
    }
    else
    {
        if (display.decodeBmpToCanvas(_nextFramePath))
            _frameLoaded = true;
    }

    Serial.printf("[GifApp] Loaded: %s (%d frames, %dx%d, %dms)\n",
                  _currentGif.name.c_str(), _currentGif.frameCount,
                  _currentGif.width, _currentGif.height, _currentGif.defaultDelay);
}

void GifApp::playFrame()
{
    unsigned long now = millis();
    if (now - _lastFrameTime < _currentGif.defaultDelay || !_frameLoaded)
        return;

    _lastFrameTime = now;
    _frameLoaded = false;

    showOverlay();
    display.swapAndRender();

    _currentFrame++;
    if (_currentFrame >= _currentGif.frameCount)
        _currentFrame = 0;

    snprintf(_nextFramePath, sizeof(_nextFramePath), "/gifs/%s/%d.bmp",
             _currentGif.name.c_str(), _currentFrame);
    xTaskNotifyGive(_loaderTask);
}

// Declared in main.cpp
extern bool overlayVisible;

void GifApp::showOverlay()
{
    if (!overlayVisible)
        return;
    bool wifi = (WiFi.status() == WL_CONNECTED);
    display.drawOverlay(wifi, getTimeString(),
                        _currentGif.name, _currentIndex + 1, gifManager.getGifCount());
}
