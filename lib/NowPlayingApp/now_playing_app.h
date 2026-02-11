#ifndef NOW_PLAYING_APP_H
#define NOW_PLAYING_APP_H

#include "app.h"

struct NowPlayingInfo
{
    char title[64];
    char artist[48];
    int frameCount;
    bool framesReady;
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

    // Called from web server
    void updateTrack(const char *title, const char *artist, int frameCount);
    void setFramesReady();
    const NowPlayingInfo &getInfo() const { return _info; }

private:
    NowPlayingInfo _info;

    int _currentFrame;
    unsigned long _lastFrameTime;
    bool _needRedraw;

    void playFrame();
    void renderIdle();
};

extern NowPlayingApp nowPlayingApp;

#endif // NOW_PLAYING_APP_H
