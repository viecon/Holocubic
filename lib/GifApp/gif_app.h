#ifndef GIF_APP_H
#define GIF_APP_H

#include "app.h"
#include "gif_manager.h"

class GifApp : public App
{
public:
    GifApp();

    void onEnter() override;
    void onExit() override;
    void loop() override;
    bool onTilt(int direction) override;
    const char *name() const override { return "GIF"; }

    void notifyGifChange();

private:
    int _currentIndex;
    int _currentFrame;
    unsigned long _lastFrameTime;
    GifInfo _currentGif;
    bool _needRefresh;

    static TaskHandle_t _loaderTask;
    static volatile bool _frameLoaded;
    static volatile bool _loaderBusy;
    static char _nextFramePath[64];

    static void frameLoaderTask(void *param);
    void loadGif();
    void playFrame();
    void showOverlay();
};

extern GifApp gifApp;

#endif // GIF_APP_H
