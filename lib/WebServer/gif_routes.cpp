#include "gif_routes.h"
#include "upload_manager.h"
#include "gif_manager.h"
#include "config.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <vector>

static void (*_onGifChange)() = nullptr;

void GifRoutes::setOnGifChange(void (*callback)())
{
    _onGifChange = callback;
}

static void uploadResponseHandler(AsyncWebServerRequest *request)
{
    if (uploadManager.consumeError())
    {
        request->send(500, "application/json", "{\"error\":\"SD write failed\"}");
    }
    else
    {
        request->send(200, "application/json", "{\"success\":true}");
    }
}

static void handleGetGifs(AsyncWebServerRequest *request)
{
    gifManager.refresh();

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    int count = gifManager.getGifCount();

    for (int i = 0; i < count; i++)
    {
        GifInfo info;
        if (gifManager.getGifInfoByIndex(i, info))
        {
            JsonObject obj = arr.add<JsonObject>();
            obj["name"] = info.name;
            obj["frameCount"] = info.frameCount;
            obj["width"] = info.width;
            obj["height"] = info.height;
            obj["defaultDelay"] = info.defaultDelay;
        }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

static void handleGetGifInfo(AsyncWebServerRequest *request)
{
    const String &name = request->pathArg(0);

    GifInfo info;
    if (!gifManager.getGifInfo(name.c_str(), info))
    {
        request->send(404, "application/json", "{\"error\":\"GIF not found\"}");
        return;
    }

    JsonDocument doc;
    doc["name"] = info.name;
    doc["frameCount"] = info.frameCount;
    doc["width"] = info.width;
    doc["height"] = info.height;
    doc["defaultDelay"] = info.defaultDelay;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

static void handleDeleteGif(AsyncWebServerRequest *request)
{
    const String &name = request->pathArg(0);

    if (gifManager.deleteGif(name.c_str()))
    {
        if (_onGifChange)
            _onGifChange();
        request->send(200, "application/json", "{\"success\":true}");
    }
    else
    {
        request->send(500, "application/json", "{\"error\":\"Failed to delete\"}");
    }
}

static void handleCreateGif(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject obj = json.as<JsonObject>();

    const char *name = obj["name"] | "";
    int frameCount = obj["frameCount"] | 0;
    int width = obj["width"] | CANVAS_WIDTH;
    int height = obj["height"] | CANVAS_HEIGHT;
    uint16_t defaultDelay = obj["defaultDelay"] | 100;

    if (strlen(name) == 0 || strlen(name) > MAX_GIF_NAME_LEN || frameCount == 0)
    {
        request->send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
        return;
    }

    if (gifManager.createGif(name, frameCount, width, height, defaultDelay))
    {
        if (_onGifChange)
            _onGifChange();
        Serial.printf("[GifRoutes] Created GIF: %s. Free heap: %u\n", name, ESP.getFreeHeap());
        request->send(200, "application/json", "{\"success\":true}");
    }
    else
    {
        request->send(500, "application/json", "{\"error\":\"Failed to create GIF\"}");
    }
}

static void handleUploadFrame(AsyncWebServerRequest *request, const String &filename,
                              size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        uploadManager.setUploading(true);
        uploadManager.setError(false);
        uploadManager.touchTimestamp();

        char path[64];
        snprintf(path, sizeof(path), "%s/%s/%s.bmp",
                 GIFS_ROOT, request->pathArg(0).c_str(), request->pathArg(1).c_str());

        if (!uploadManager.openFile(path))
            return;
    }

    uploadManager.writeChunk(data, len);

    if (final)
    {
        uploadManager.closeFile();
    }
}

static void handleGetFrame(AsyncWebServerRequest *request)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/%s/%s.bmp",
             GIFS_ROOT, request->pathArg(0).c_str(), request->pathArg(1).c_str());

    if (!SD.exists(path))
    {
        request->send(404, "text/plain", "Frame not found");
        return;
    }

    request->send(SD, path, "image/bmp");
}

static void handleUploadOriginal(AsyncWebServerRequest *request, const String &filename,
                                 size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        uploadManager.setError(false);
        uploadManager.touchTimestamp();

        char path[64];
        snprintf(path, sizeof(path), "%s/%s/original.gif",
                 GIFS_ROOT, request->pathArg(0).c_str());

        if (!uploadManager.openOriginal(path))
            return;
    }

    uploadManager.writeOriginalChunk(data, len);

    if (final)
    {
        uploadManager.closeOriginal();
        Serial.printf("[GifRoutes] Original uploaded. Free heap: %u\n", ESP.getFreeHeap());

        uploadManager.setUploading(false);

        if (_onGifChange)
            _onGifChange();
    }
}

static void handleGetOriginal(AsyncWebServerRequest *request)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/%s/original.gif",
             GIFS_ROOT, request->pathArg(0).c_str());

    if (!SD.exists(path))
    {
        request->send(404, "text/plain", "Original not found");
        return;
    }

    AsyncWebServerResponse *response = request->beginResponse(SD, path, "image/gif");
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
}

static void handleReorder(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject obj = json.as<JsonObject>();
    JsonArray order = obj["order"].as<JsonArray>();

    if (order.isNull() || order.size() == 0)
    {
        request->send(400, "application/json", "{\"error\":\"Invalid order\"}");
        return;
    }

    std::vector<String> names;
    names.reserve(order.size());
    for (JsonVariant v : order)
    {
        names.push_back(v.as<String>());
    }

    if (gifManager.reorderGifs(names))
    {
        if (_onGifChange)
            _onGifChange();
        request->send(200, "application/json", "{\"success\":true}");
    }
    else
    {
        request->send(500, "application/json", "{\"error\":\"Failed to reorder\"}");
    }
}

void GifRoutes::registerRoutes(AsyncWebServer &server)
{
    server.on("/api/gifs", HTTP_GET, handleGetGifs);

    server.on("^\\/api\\/gif\\/([^\\/]+)$", HTTP_GET, handleGetGifInfo);

    server.on("^\\/api\\/gif\\/([^\\/]+)$", HTTP_DELETE, handleDeleteGif);

    server.on("^\\/api\\/gif\\/([^\\/]+)\\/frame\\/([0-9]+)$", HTTP_GET, handleGetFrame);

    server.on(
        "^\\/api\\/gif\\/([^\\/]+)\\/frame\\/([0-9]+)$",
        HTTP_POST,
        uploadResponseHandler,
        handleUploadFrame);

    server.on("^\\/api\\/gif\\/([^\\/]+)\\/original$", HTTP_GET, handleGetOriginal);

    server.on(
        "^\\/api\\/gif\\/([^\\/]+)\\/original$",
        HTTP_POST,
        uploadResponseHandler,
        handleUploadOriginal);

    auto *createHandler = new AsyncCallbackJsonWebHandler("/api/gif", handleCreateGif);
    server.addHandler(createHandler);

    auto *reorderHandler = new AsyncCallbackJsonWebHandler("/api/reorder", handleReorder);
    server.addHandler(reorderHandler);
}
