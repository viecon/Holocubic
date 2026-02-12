#include "frame_loader.h"
#include "display.h"
#include "upload_manager.h"

FrameLoader frameLoader;

TaskHandle_t FrameLoader::_task = NULL;
volatile bool FrameLoader::_frameLoaded = false;
volatile bool FrameLoader::_loaderBusy = false;
char FrameLoader::_path[64] = {0};

void FrameLoader::loaderTask(void *param)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (uploadManager.isUploading())
            continue;

        _loaderBusy = true;
        if (display.decodeBmpToCanvas(_path))
            _frameLoaded = true;
        _loaderBusy = false;
    }
}

void FrameLoader::begin()
{
    if (_task != NULL)
        return;

    xTaskCreatePinnedToCore(
        loaderTask,
        "FrameLoader",
        4096,
        NULL,
        1,
        &_task,
        0);
    Serial.println("[FrameLoader] Started on Core 0");
}

void FrameLoader::requestLoad(const char *path)
{
    strlcpy(_path, path, sizeof(_path));
    _frameLoaded = false;
    if (_task != NULL)
        xTaskNotifyGive(_task);
}

void FrameLoader::waitIdle()
{
    while (_loaderBusy)
        delay(1);
}
