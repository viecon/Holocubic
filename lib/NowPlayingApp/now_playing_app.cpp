#include "now_playing_app.h"
#include "display.h"
#include "frame_loader.h"
#include "config.h"

NowPlayingApp nowPlayingApp;

static const uint16_t COLOR_GRAY = 0x7BEF;

NowPlayingApp::NowPlayingApp()
    : _currentFrame(0), _nextFrame(1), _lastFrameTime(0), _needRedraw(true), _frameRequested(false)
{
    memset(&_info, 0, sizeof(_info));
    _nextFramePath[0] = '\0';
}

void NowPlayingApp::onEnter()
{
    Serial.println("[NowPlaying] Enter");
    frameLoader.begin();
    _currentFrame = 0;
    _lastFrameTime = millis();
    _needRedraw = true;
    _frameRequested = false;
}

void NowPlayingApp::onExit()
{
    Serial.println("[NowPlaying] Exit");
}

void NowPlayingApp::loop()
{
    if (!_info.framesReady || _info.frameCount <= 0)
    {
        if (_needRedraw)
        {
            _needRedraw = false;
            renderIdle();
            display.swapAndRender();
        }
        return;
    }

    unsigned long now = millis();
    unsigned long frameDelay = NP_FRAME_DELAY_MS;
    if (_currentFrame == 0 || _currentFrame == _info.frameCount - 1)
        frameDelay *= 5;

    if (!_frameRequested)
    {
        snprintf(_nextFramePath, sizeof(_nextFramePath), "%s/%d.bmp", NP_DIR, _nextFrame);
        frameLoader.requestLoad(_nextFramePath);
        _frameRequested = true;
    }

    if (frameLoader.isLoaded() && now - _lastFrameTime >= frameDelay)
    {
        _lastFrameTime = now;
        frameLoader.consumeLoaded();
        display.swapAndRender();
        _currentFrame = _nextFrame;
        _nextFrame = (_nextFrame + 1) % _info.frameCount;
        _frameRequested = false;
    }
}

void NowPlayingApp::updateTrack(const char *title, const char *artist, int frameCount)
{
    strncpy(_info.title, title, sizeof(_info.title) - 1);
    _info.title[sizeof(_info.title) - 1] = '\0';
    strncpy(_info.artist, artist, sizeof(_info.artist) - 1);
    _info.artist[sizeof(_info.artist) - 1] = '\0';
    _info.frameCount = frameCount;
    _info.framesReady = false;
    _info.lastUpdate = millis();

    _currentFrame = 0;
    _needRedraw = true;

    Serial.printf("[NowPlaying] Track: %s - %s (%d frames)\n",
                  _info.artist, _info.title, frameCount);
}

void NowPlayingApp::setFramesReady()
{
    _info.framesReady = true;
    _currentFrame = 0;
    _lastFrameTime = millis();
    _frameRequested = false;
    Serial.printf("[NowPlaying] Frames ready (%d)\n", _info.frameCount);
}

void NowPlayingApp::renderIdle()
{
    GFXcanvas16 *canvas = display.getBackCanvas();
    if (!canvas)
        return;

    display.clearBackBuffer();

    canvas->setTextSize(1);
    canvas->setTextColor(COLOR_GRAY);

    const char *header = "Now Playing";
    int hw = strlen(header) * 6;
    canvas->setCursor((CANVAS_WIDTH - hw) / 2, 35);
    canvas->print(header);

    const char *msg = "Waiting for music...";
    int mw = strlen(msg) * 6;
    canvas->setCursor((CANVAS_WIDTH - mw) / 2, 55);
    canvas->print(msg);
}
