#include "gif_app.h"
#include "display.h"
#include "frame_loader.h"
#include "web_server.h"
#include <WiFi.h>

GifApp gifApp;

GifApp::GifApp()
    : _currentIndex(0), _currentFrame(0), _lastFrameTime(0),
      _needRefresh(false)
{
    _currentGif.valid = false;
    _nextFramePath[0] = '\0';
}

void GifApp::onEnter()
{
    Serial.println("[GifApp] Enter");
    frameLoader.begin();
    loadGif();
}

void GifApp::onExit()
{
    Serial.println("[GifApp] Exit");
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
            display.showIP(webServer.getLocalIP());
        }
        else
        {
            if (_currentIndex >= count)
                _currentIndex = 0;
            loadGif();
        }
    }

    if (_currentGif.valid && !webServer.isUploading())
        playFrame();
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
    Serial.printf("[GifApp] Switched to GIF %d: %s\n", _currentIndex, _currentGif.name);
    return true;
}

void GifApp::notifyGifChange()
{
    _needRefresh = true;
}

void GifApp::loadGif()
{
    Serial.printf("[GifApp] Loading GIF index %d...\n", _currentIndex);

    frameLoader.waitIdle();

    if (!gifManager.getGifInfoByIndex(_currentIndex, _currentGif))
    {
        Serial.println("[GifApp] Failed to get GIF info!");
        _currentGif.valid = false;
        return;
    }

    _currentFrame = 0;
    _lastFrameTime = millis();

    snprintf(_nextFramePath, sizeof(_nextFramePath), "/gifs/%s/%d.bmp",
             _currentGif.name, _currentFrame);
    frameLoader.requestLoad(_nextFramePath);

    Serial.printf("[GifApp] Loaded: %s (%d frames, %dx%d, %dms)\n",
                  _currentGif.name, _currentGif.frameCount,
                  _currentGif.width, _currentGif.height, _currentGif.defaultDelay);
}

void GifApp::playFrame()
{
    unsigned long now = millis();
    if (now - _lastFrameTime < _currentGif.defaultDelay || !frameLoader.isLoaded())
        return;

    _lastFrameTime = now;
    frameLoader.consumeLoaded();

    showOverlay();
    display.swapAndRender();

    _currentFrame++;
    if (_currentFrame >= _currentGif.frameCount)
        _currentFrame = 0;

    snprintf(_nextFramePath, sizeof(_nextFramePath), "/gifs/%s/%d.bmp",
             _currentGif.name, _currentFrame);
    frameLoader.requestLoad(_nextFramePath);
}

void GifApp::showOverlay()
{
    if (!isOverlayVisible())
        return;
    bool wifi = (WiFi.status() == WL_CONNECTED);
    display.drawOverlay(wifi, display.getTimeString(),
                        _currentGif.name, _currentIndex + 1, gifManager.getGifCount());
}
