#include "gif_manager.h"
#include <SD.h>

GifManager gifManager;

GifManager::GifManager()
{
}

bool GifManager::begin()
{
    if (!SD.begin(SD_CS, SPI, SD_SPI_FREQUENCY))
    {
        Serial.println("[GifManager] SD card init failed!");
        return false;
    }

    Serial.printf("[GifManager] SD card initialized @ %d MHz\n", SD_SPI_FREQUENCY / 1000000);

    // Ensure gifs directory exists
    ensureDirectory(GIFS_ROOT); // GIFS_ROOT is a const char*

    // Load existing GIFs
    return refresh();
}

bool GifManager::ensureDirectory(const char *path)
{
    if (SD.exists(path))
    {
        return true;
    }
    return SD.mkdir(path);
}

bool GifManager::refresh()
{
    _gifNames.clear();

    // First try to load from order.json
    if (loadOrder())
    {
        Serial.printf("[GifManager] Loaded %d GIFs from order\n", _gifNames.size());
        return true;
    }

    // Otherwise scan directory
    File root = SD.open(GIFS_ROOT);
    if (!root || !root.isDirectory())
    {
        Serial.println("[GifManager] Cannot open gifs directory");
        return false;
    }

    File entry;
    while ((entry = root.openNextFile()))
    {
        if (entry.isDirectory())
        {
            String fullPath = entry.name();
            Serial.printf("[GifManager] Found entry: %s\n", fullPath.c_str());

            // Extract just the folder name from path
            int lastSlash = fullPath.lastIndexOf('/');
            String name = (lastSlash >= 0) ? fullPath.substring(lastSlash + 1) : fullPath;

            // Skip hidden directories
            if (name.length() > 0 && name[0] != '.')
            {
                Serial.printf("[GifManager] Found GIF folder: %s\n", name.c_str());
                _gifNames.push_back(name);
            }
        }
        entry.close();
    }
    root.close();

    // Save the discovered order
    if (!_gifNames.empty())
    {
        saveOrder();
    }

    Serial.printf("[GifManager] Found %d GIFs\n", _gifNames.size());
    return true;
}

bool GifManager::loadOrder()
{
    File f = SD.open(ORDER_FILE, FILE_READ);
    if (!f)
    {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        Serial.printf("[GifManager] Order JSON error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc["order"].as<JsonArray>();
    _gifNames.clear();

    char verifyBuf[64];
    for (JsonVariant v : arr)
    {
        const char *name = v.as<const char *>();
        if (name && name[0] != '\0')
        {
            // Verify GIF directory exists
            snprintf(verifyBuf, sizeof(verifyBuf), "%s/%s", GIFS_ROOT, name);
            if (SD.exists(verifyBuf))
            {
                _gifNames.push_back(name);
            }
        }
    }

    return !_gifNames.empty();
}

bool GifManager::saveOrder()
{
    File f = SD.open(ORDER_FILE, FILE_WRITE);
    if (!f)
    {
        Serial.println("[GifManager] Cannot write order file");
        return false;
    }

    JsonDocument doc;
    JsonArray arr = doc["order"].to<JsonArray>();

    for (const auto &name : _gifNames)
    {
        arr.add(name);
    }

    serializeJson(doc, f);
    f.close();
    return true;
}

int GifManager::getGifCount()
{
    return _gifNames.size();
}

String GifManager::getGifName(int index)
{
    if (index < 0 || index >= (int)_gifNames.size())
    {
        return "";
    }
    return _gifNames[index];
}

bool GifManager::loadGifConfig(const String &name, GifInfo &info)
{
    snprintf(_pathBuf, sizeof(_pathBuf), "%s/%s/%s", GIFS_ROOT, name.c_str(), GIF_CONFIG_FILE);

    Serial.printf("[GifManager] Loading config: %s\n", _pathBuf);

    File f = SD.open(_pathBuf, FILE_READ);
    if (!f)
    {
        Serial.printf("[GifManager] Config not found: %s\n", _pathBuf);
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        Serial.printf("[GifManager] Config JSON error: %s\n", err.c_str());
        return false;
    }

    info.name = name;
    info.frameCount = doc["frameCount"] | 0;
    info.width = doc["width"] | CANVAS_WIDTH;
    info.height = doc["height"] | CANVAS_HEIGHT;
    info.defaultDelay = doc["defaultDelay"] | 100;
    info.valid = true;

    Serial.printf("[GifManager] Config loaded: frames=%d, size=%dx%d, delay=%d\n",
                  info.frameCount, info.width, info.height, info.defaultDelay);

    return true;
}

bool GifManager::saveGifConfig(const String &name, const GifInfo &info)
{
    snprintf(_pathBuf, sizeof(_pathBuf), "%s/%s/%s", GIFS_ROOT, name.c_str(), GIF_CONFIG_FILE);

    File f = SD.open(_pathBuf, FILE_WRITE);
    if (!f)
    {
        return false;
    }

    JsonDocument doc;
    doc["name"] = name;
    doc["frameCount"] = info.frameCount;
    doc["width"] = info.width;
    doc["height"] = info.height;
    doc["defaultDelay"] = info.defaultDelay;

    serializeJson(doc, f);
    f.close();
    return true;
}

bool GifManager::getGifInfo(const String &name, GifInfo &info)
{
    return loadGifConfig(name, info);
}

bool GifManager::getGifInfoByIndex(int index, GifInfo &info)
{
    if (index < 0 || index >= (int)_gifNames.size())
    {
        info.valid = false;
        return false;
    }
    return loadGifConfig(_gifNames[index], info);
}

bool GifManager::createGif(const String &name, int frameCount, int width, int height, uint16_t defaultDelay)
{
    // Create directory
    snprintf(_pathBuf, sizeof(_pathBuf), "%s/%s", GIFS_ROOT, name.c_str());

    if (!ensureDirectory(_pathBuf))
    {
        Serial.printf("[GifManager] Cannot create directory: %s\n", _pathBuf);
        return false;
    }

    // Create config
    GifInfo info;
    info.name = name;
    info.frameCount = frameCount;
    info.width = width;
    info.height = height;
    info.defaultDelay = defaultDelay;
    info.valid = true;

    if (!saveGifConfig(name, info))
    {
        return false;
    }

    // Add to list if not already present
    bool found = false;
    for (const auto &n : _gifNames)
    {
        if (n == name)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        _gifNames.push_back(name);
        saveOrder();
    }

    return true;
}

bool GifManager::saveFrame(const String &gifName, int frameIndex, const uint8_t *data, size_t len)
{
    snprintf(_pathBuf, sizeof(_pathBuf), "%s/%s/%d.bmp", GIFS_ROOT, gifName.c_str(), frameIndex);

    File f = SD.open(_pathBuf, FILE_WRITE);
    if (!f)
    {
        Serial.printf("[GifManager] Cannot write frame: %s\n", _pathBuf);
        return false;
    }

    size_t written = f.write(data, len);
    f.close();

    return written == len;
}

String GifManager::getFramePath(const String &gifName, int frameIndex)
{
    snprintf(_pathBuf, sizeof(_pathBuf), "%s/%s/%d.bmp", GIFS_ROOT, gifName.c_str(), frameIndex);
    return String(_pathBuf);
}

bool GifManager::deleteDirectory(const String &path)
{
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory())
    {
        return false;
    }

    File entry;
    while ((entry = dir.openNextFile()))
    {
        // entry.name() returns full path on ESP32
        const char *entryPath = entry.name();

        if (entry.isDirectory())
        {
            entry.close();
            deleteDirectory(entryPath);
        }
        else
        {
            entry.close();
            SD.remove(entryPath);
        }
    }
    dir.close();

    return SD.rmdir(path);
}

bool GifManager::deleteGif(const String &name)
{
    snprintf(_pathBuf, sizeof(_pathBuf), "%s/%s", GIFS_ROOT, name.c_str());

    if (!deleteDirectory(_pathBuf))
    {
        Serial.printf("[GifManager] Cannot delete: %s\n", _pathBuf);
        return false;
    }

    // Remove from list
    for (auto it = _gifNames.begin(); it != _gifNames.end(); ++it)
    {
        if (*it == name)
        {
            _gifNames.erase(it);
            saveOrder();
            break;
        }
    }

    return true;
}

bool GifManager::reorderGifs(const std::vector<String> &names)
{
    // Verify all names exist
    for (const auto &name : names)
    {
        bool found = false;
        for (const auto &n : _gifNames)
        {
            if (n == name)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }

    _gifNames = names;
    return saveOrder();
}

bool GifManager::moveGif(int fromIndex, int toIndex)
{
    int size = _gifNames.size();
    if (fromIndex < 0 || fromIndex >= size ||
        toIndex < 0 || toIndex >= size ||
        fromIndex == toIndex)
    {
        return false;
    }

    String temp = _gifNames[fromIndex];
    _gifNames.erase(_gifNames.begin() + fromIndex);
    _gifNames.insert(_gifNames.begin() + toIndex, temp);

    return saveOrder();
}
