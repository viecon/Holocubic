#ifndef FRAME_LOADER_H
#define FRAME_LOADER_H

#include <Arduino.h>

class FrameLoader
{
public:
    void begin();
    void requestLoad(const char *path);
    bool isLoaded() const { return _frameLoaded; }
    bool isBusy() const { return _loaderBusy; }
    void consumeLoaded() { _frameLoaded = false; }
    void waitIdle();

private:
    static TaskHandle_t _task;
    static volatile bool _frameLoaded;
    static volatile bool _loaderBusy;
    static char _path[64];

    static void loaderTask(void *param);
};

extern FrameLoader frameLoader;

#endif // FRAME_LOADER_H
