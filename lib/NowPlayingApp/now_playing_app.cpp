#include "now_playing_app.h"
#include "display.h"
#include "config.h"
#include <SD.h>

NowPlayingApp nowPlayingApp;

// Color constants (RGB565)
static const uint16_t COLOR_GRAY = 0x7BEF;

NowPlayingApp::NowPlayingApp()
    : _currentFrame(0), _lastFrameTime(0), _needRedraw(true)
{
    memset(&_info, 0, sizeof(_info));
}

void NowPlayingApp::onEnter()
{
    Serial.println("[NowPlaying] Enter");
    _currentFrame = 0;
    _lastFrameTime = millis();
    _needRedraw = true;
}

void NowPlayingApp::onExit()
{
    Serial.println("[NowPlaying] Exit");
}

void NowPlayingApp::loop()
{
    if (!_info.framesReady || _info.frameCount <= 0)
    {
        // No frames — show idle screen once
        if (_needRedraw)
        {
            _needRedraw = false;
            renderIdle();
            display.swapAndRender();
        }
        return;
    }

    // Frame animation
    unsigned long now = millis();
    int delay = NP_FRAME_DELAY_MS;
    if (_currentFrame == 0 || _currentFrame == _info.frameCount - 1)
        delay *= 5;

    if (now - _lastFrameTime >= delay)
    {
        _lastFrameTime = now;
        playFrame();
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
    Serial.printf("[NowPlaying] Frames ready (%d)\n", _info.frameCount);
}

void NowPlayingApp::playFrame()
{
    char path[32];
    snprintf(path, sizeof(path), "%s/%d.bmp", NP_DIR, _currentFrame);

    if (display.decodeBmpToCanvas(path))
    {
        display.swapAndRender();
    }

    _currentFrame++;
    if (_currentFrame >= _info.frameCount)
        _currentFrame = 0;
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
