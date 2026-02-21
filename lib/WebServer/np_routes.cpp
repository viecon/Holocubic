#include "np_routes.h"
#include "upload_manager.h"
#include "now_playing_app.h"
#include "config.h"
#include <SD.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

static void handleNowPlaying(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject obj = json.as<JsonObject>();
    const char *title = obj["title"] | "";
    const char *artist = obj["artist"] | "";
    int frameCount = obj["frameCount"] | 0;

    if (strlen(title) == 0 || frameCount <= 0)
    {
        request->send(400, "application/json", "{\"error\":\"title and frameCount required\"}");
        return;
    }

    nowPlayingApp.updateTrack(title, artist, frameCount);

    Serial.printf("[NpRoutes] Track: \"%s\" (%d frames). Free heap: %u\n",
                  title, frameCount, ESP.getFreeHeap());
    request->send(200, "application/json", "{\"success\":true}");
}

static void handleGetNowPlaying(AsyncWebServerRequest *request)
{
    const NowPlayingInfo &info = nowPlayingApp.getInfo();

    JsonDocument doc;
    doc["title"] = info.title;
    doc["artist"] = info.artist;
    doc["frameCount"] = info.frameCount;
    doc["framesReady"] = info.framesReady;
    doc["active"] = (info.lastUpdate > 0);

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

static void handleUploadNpFrame(AsyncWebServerRequest *request, const String &filename,
                                size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        if (uploadManager.isFileOpen())
            uploadManager.closeFile();

        uploadManager.setError(false);
        uploadManager.touchTimestamp();

        if (request->pathArg(0) == "0")
        {
            uploadManager.setUploading(true);

            File dir = SD.open(NP_DIR);
            if (dir && dir.isDirectory())
            {
                File entry = dir.openNextFile();
                while (entry)
                {
                    char fullPath[64];
                    strlcpy(fullPath, entry.name(), sizeof(fullPath));
                    entry.close();
                    SD.remove(fullPath);
                    entry = dir.openNextFile();
                }
                dir.close();
            }
            else
            {
                if (!SD.mkdir(NP_DIR))
                {
                    Serial.printf("[NpRoutes] Failed to create %s\n", NP_DIR);
                }
            }
            Serial.printf("[NpRoutes] NP upload start. Free heap: %u\n", ESP.getFreeHeap());
        }

        char path[64];
        snprintf(path, sizeof(path), "%s/%s.bmp",
                 NP_DIR, request->pathArg(0).c_str());

        if (!uploadManager.openFile(path))
            return;
    }

    uploadManager.writeChunk(data, len);

    if (final)
    {
        uploadManager.closeFile();
    }
}

static void handleNpReady(AsyncWebServerRequest *request)
{
    uploadManager.setUploading(false);
    nowPlayingApp.setFramesReady();
    Serial.printf("[NpRoutes] NP frames ready. Free heap: %u\n", ESP.getFreeHeap());
    request->send(200, "application/json", "{\"success\":true}");
}

void NpRoutes::registerRoutes(AsyncWebServer &server)
{
    server.on("/api/now-playing", HTTP_GET, handleGetNowPlaying);

    auto *npHandler = new AsyncCallbackJsonWebHandler("/api/now-playing", handleNowPlaying);
    server.addHandler(npHandler);

    server.on(
        "^\\/api\\/np\\/frame\\/([0-9]+)$",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            if (uploadManager.consumeError())
            {
                request->send(500, "application/json", "{\"error\":\"SD write failed\"}");
            }
            else
            {
                request->send(200, "application/json", "{\"success\":true}");
            }
        },
        handleUploadNpFrame);

    server.on("/api/np/ready", HTTP_POST, handleNpReady);
}
