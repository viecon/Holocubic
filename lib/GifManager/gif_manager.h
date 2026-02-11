#ifndef GIF_MANAGER_H
#define GIF_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"

struct GifInfo
{
    String name;
    int frameCount;
    int width;
    int height;
    uint16_t defaultDelay;
    bool valid;
};

class GifManager
{
public:
    GifManager();

    bool begin();

    // GIF list management
    int getGifCount();
    String getGifName(int index);
    bool getGifInfo(const String &name, GifInfo &info);
    bool getGifInfoByIndex(int index, GifInfo &info);

    // File operations
    bool createGif(const String &name, int frameCount, int width, int height, uint16_t defaultDelay);
    bool deleteGif(const String &name);
    bool saveFrame(const String &gifName, int frameIndex, const uint8_t *data, size_t len);
    String getFramePath(const String &gifName, int frameIndex);

    // Order management
    bool reorderGifs(const std::vector<String> &names);
    bool moveGif(int fromIndex, int toIndex);

    // Refresh from SD card
    bool refresh();

private:
    std::vector<String> _gifNames;
    char _pathBuf[64];

    bool ensureDirectory(const char *path);
    bool loadOrder();
    bool saveOrder();
    bool loadGifConfig(const String &name, GifInfo &info);
    bool saveGifConfig(const String &name, const GifInfo &info);
    bool deleteDirectory(const String &path);
};

extern GifManager gifManager;

#endif // GIF_MANAGER_H
