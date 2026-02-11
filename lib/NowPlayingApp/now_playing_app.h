#ifndef NOW_PLAYING_APP_H
#define NOW_PLAYING_APP_H

#include "app.h"

struct NowPlayingInfo
{
    char title[64];
    char artist[48];
    bool hasArt;
    unsigned long lastUpdate;
};

class NowPlayingApp : public App
{
public:
    NowPlayingApp();

    void onEnter() override;
    void onExit() override;
    void loop() override;
    const char *name() const override { return "NowPlaying"; }

    // Called from web server when new track data arrives
    void updateTrack(const char *title, const char *artist);
    void updateArt(const uint8_t *bmpData, size_t len);
    const NowPlayingInfo &getInfo() const { return _info; }

private:
    NowPlayingInfo _info;
    bool _needRedraw;

    void render();
};

extern NowPlayingApp nowPlayingApp;

#endif // NOW_PLAYING_APP_H
