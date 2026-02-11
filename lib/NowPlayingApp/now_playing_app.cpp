#include "now_playing_app.h"
#include "display.h"
#include <WiFi.h>

NowPlayingApp nowPlayingApp;

NowPlayingApp::NowPlayingApp()
    : _needRedraw(true)
{
    memset(&_info, 0, sizeof(_info));
}

void NowPlayingApp::onEnter()
{
    Serial.println("[NowPlaying] Enter");
    _needRedraw = true;
}

void NowPlayingApp::onExit()
{
    Serial.println("[NowPlaying] Exit");
}

void NowPlayingApp::loop()
{
    // Auto-fallback if no data for 30 seconds
    if (_info.lastUpdate > 0 && millis() - _info.lastUpdate > 30000)
    {
        _info.lastUpdate = 0;
        _needRedraw = true;
    }

    if (_needRedraw)
    {
        _needRedraw = false;
        render();
        display.swapAndRender();
    }
}

void NowPlayingApp::updateTrack(const char *title, const char *artist)
{
    strncpy(_info.title, title, sizeof(_info.title) - 1);
    _info.title[sizeof(_info.title) - 1] = '\0';
    strncpy(_info.artist, artist, sizeof(_info.artist) - 1);
    _info.artist[sizeof(_info.artist) - 1] = '\0';
    _info.lastUpdate = millis();
    _needRedraw = true;
    Serial.printf("[NowPlaying] Track: %s - %s\n", _info.artist, _info.title);
}

void NowPlayingApp::updateArt(const uint8_t *bmpData, size_t len)
{
    // TODO: decode BMP from memory buffer to back buffer
    _info.hasArt = true;
    _info.lastUpdate = millis();
    _needRedraw = true;
}

void NowPlayingApp::render()
{
    display.clearBackBuffer();

    // TODO: draw album art to back buffer if hasArt

    // Placeholder: show track info as text
    GFXcanvas16 *canvas = nullptr;
    // For now, use simple display message approach
    // Will be replaced with proper canvas drawing

    if (_info.lastUpdate == 0)
    {
        // No data — show waiting message
        // Will render via overlay-style drawing later
    }
}
