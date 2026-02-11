#include "web_server.h"
#include "gif_manager.h"
#include <WiFi.h>
#include <SD.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <vector>

HoloWebServer webServer;

HoloWebServer::HoloWebServer(uint16_t port)
    : _server(port), _onGifChange(nullptr), _uploadFrameIndex(-1)
{
}

void HoloWebServer::begin()
{
    setupRoutes();
    _server.begin();
    Serial.println("[WebServer] Started on port 80");
}

void HoloWebServer::setOnGifChange(void (*callback)())
{
    _onGifChange = callback;
}

String HoloWebServer::getLocalIP()
{
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
    {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

void HoloWebServer::setupRoutes()
{
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); });

    _server.on("/api/gifs", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetGifs(request); });

    _server.on("^\\/api\\/gif\\/([^\\/]+)$", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetGifInfo(request); });

    _server.on("^\\/api\\/gif\\/([^\\/]+)$", HTTP_DELETE, [this](AsyncWebServerRequest *request)
               { handleDeleteGif(request); });

    _server.on("^\\/api\\/gif\\/([^\\/]+)\\/frame\\/([0-9]+)$", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetFrame(request); });

    _server.on(
        "^\\/api\\/gif\\/([^\\/]+)\\/frame\\/([0-9]+)$",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            request->send(200, "application/json", "{\"success\":true}");
        },
        [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleUploadFrame(request, filename, index, data, len, final);
        });

    // Original GIF routes must be before /api/gif JSON handler
    _server.on("^\\/api\\/gif\\/([^\\/]+)\\/original$", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetOriginal(request); });

    _server.on(
        "^\\/api\\/gif\\/([^\\/]+)\\/original$",
        HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            request->send(200, "application/json", "{\"success\":true}");
        },
        [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleUploadOriginal(request, filename, index, data, len, final);
        });

    AsyncCallbackJsonWebHandler *createHandler = new AsyncCallbackJsonWebHandler(
        "/api/gif",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            handleCreateGif(request, json);
        });
    _server.addHandler(createHandler);

    AsyncCallbackJsonWebHandler *reorderHandler = new AsyncCallbackJsonWebHandler(
        "/api/reorder",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            handleReorder(request, json);
        });
    _server.addHandler(reorderHandler);

    _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleWifiPage(request); });

    _server.on("/api/abort-upload", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
                   abortUpload();
                   request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetWifi(request); });

    AsyncCallbackJsonWebHandler *wifiHandler = new AsyncCallbackJsonWebHandler(
        "/api/wifi",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            handleSaveWifi(request, json);
        });
    _server.addHandler(wifiHandler);

    _server.onNotFound([](AsyncWebServerRequest *request)
                       { request->send(404, "text/plain", "Not Found"); });
}

void HoloWebServer::handleRoot(AsyncWebServerRequest *request)
{
    const size_t len = strlen(INDEX_HTML);
    auto *response = request->beginResponse("text/html", len,
                                            [len](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                            {
                                                if (index >= len)
                                                    return 0;
                                                size_t remaining = len - index;
                                                size_t toWrite = (remaining < maxLen) ? remaining : maxLen;
                                                memcpy(buffer, INDEX_HTML + index, toWrite);
                                                return toWrite;
                                            });
    request->send(response);
}

void HoloWebServer::handleGetGifs(AsyncWebServerRequest *request)
{
    gifManager.refresh();

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    int count = gifManager.getGifCount();
    Serial.printf("[WebServer] handleGetGifs: found %d GIFs\n", count);

    for (int i = 0; i < count; i++)
    {
        GifInfo info;
        if (gifManager.getGifInfoByIndex(i, info))
        {
            Serial.printf("[WebServer] GIF %d: %s (%d frames)\n", i, info.name, info.frameCount);
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

void HoloWebServer::handleGetGifInfo(AsyncWebServerRequest *request)
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

void HoloWebServer::handleDeleteGif(AsyncWebServerRequest *request)
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

void HoloWebServer::handleCreateGif(AsyncWebServerRequest *request, JsonVariant &json)
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
        Serial.printf("[WebServer] Created GIF: %s. Free heap: %u\n", name, ESP.getFreeHeap());
        request->send(200, "application/json", "{\"success\":true}");
    }
    else
    {
        request->send(500, "application/json", "{\"error\":\"Failed to create GIF\"}");
    }
}

void HoloWebServer::handleUploadFrame(AsyncWebServerRequest *request, const String &filename,
                                      size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        _isUploading = true;

        snprintf(_uploadPath, sizeof(_uploadPath), "%s/%s/%s.bmp",
                 GIFS_ROOT, request->pathArg(0).c_str(), request->pathArg(1).c_str());

        _uploadFile = SD.open(_uploadPath, FILE_WRITE);
        if (!_uploadFile)
        {
            Serial.printf("[WebServer] Cannot create file: %s\n", _uploadPath);
            return;
        }
    }

    if (_uploadFile)
    {
        size_t written = _uploadFile.write(data, len);
        if (written != len)
        {
            Serial.printf("[WebServer] SD write error: wrote %u/%u\n", written, len);
            _uploadFile.close();
        }
    }

    if (final)
    {
        if (_uploadFile)
        {
            _uploadFile.close();
        }
        _uploadFrameIndex = -1;
    }
}

void HoloWebServer::abortUpload()
{
    if (_uploadFile)
    {
        _uploadFile.close();
    }
    if (_uploadOriginalFile)
    {
        _uploadOriginalFile.close();
    }
    _isUploading = false;
    _uploadFrameIndex = -1;
    Serial.printf("[WebServer] Upload aborted. Free heap: %u\n", ESP.getFreeHeap());
}

void HoloWebServer::handleGetFrame(AsyncWebServerRequest *request)
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

void HoloWebServer::handleUploadOriginal(AsyncWebServerRequest *request, const String &filename,
                                         size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        snprintf(_uploadPath, sizeof(_uploadPath), "%s/%s/original.gif",
                 GIFS_ROOT, request->pathArg(0).c_str());

        _uploadOriginalFile = SD.open(_uploadPath, FILE_WRITE);
        if (!_uploadOriginalFile)
        {
            Serial.printf("[WebServer] Cannot create original: %s\n", _uploadPath);
            return;
        }
    }

    if (_uploadOriginalFile)
    {
        _uploadOriginalFile.write(data, len);
    }

    if (final)
    {
        if (_uploadOriginalFile)
        {
            _uploadOriginalFile.close();
            Serial.printf("[WebServer] Uploaded original done. Free heap: %u\n", ESP.getFreeHeap());
        }

        _isUploading = false;

        if (_onGifChange)
            _onGifChange();
    }
}

void HoloWebServer::handleGetOriginal(AsyncWebServerRequest *request)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/%s/original.gif",
             GIFS_ROOT, request->pathArg(0).c_str());

    if (!SD.exists(path))
    {
        request->send(404, "text/plain", "Original not found");
        return;
    }

    request->send(SD, path, "image/gif");
}

void HoloWebServer::handleReorder(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject obj = json.as<JsonObject>();
    JsonArray order = obj["order"].as<JsonArray>();

    if (order.isNull() || order.size() == 0)
    {
        request->send(400, "application/json", "{\"error\":\"Invalid order\"}");
        return;
    }

    // Convert to vector
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

void HoloWebServer::handleGetWifi(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    doc["mode"] = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? "AP" : "STA";
    doc["ip"] = getLocalIP();

    if (SD.exists(WIFI_CONFIG_FILE))
    {
        File f = SD.open(WIFI_CONFIG_FILE, FILE_READ);
        if (f)
        {
            JsonDocument cfg;
            if (!deserializeJson(cfg, f))
            {
                doc["ssid"] = cfg["ssid"].as<String>();
            }
            f.close();
        }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void HoloWebServer::handleSaveWifi(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject obj = json.as<JsonObject>();
    const char *ssid = obj["ssid"] | "";
    const char *password = obj["password"] | "";

    if (strlen(ssid) == 0)
    {
        request->send(400, "application/json", "{\"error\":\"SSID is required\"}");
        return;
    }

    // Write to SD
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    File f = SD.open(WIFI_CONFIG_FILE, FILE_WRITE);
    if (!f)
    {
        request->send(500, "application/json", "{\"error\":\"Failed to write wifi.json\"}");
        return;
    }

    serializeJson(doc, f);
    f.close();

    Serial.printf("[WiFi] Saved config: SSID=\"%s\"\n", ssid);
    request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi config saved. Rebooting...\"}");

    // Reboot after short delay to let response be sent
    delay(500);
    ESP.restart();
}

void HoloWebServer::handleWifiPage(AsyncWebServerRequest *request)
{
    const size_t len = strlen(WIFI_HTML);
    auto *response = request->beginResponse("text/html", len,
                                            [len](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                            {
                                                if (index >= len)
                                                    return 0;
                                                size_t remaining = len - index;
                                                size_t toWrite = (remaining < maxLen) ? remaining : maxLen;
                                                memcpy(buffer, WIFI_HTML + index, toWrite);
                                                return toWrite;
                                            });
    request->send(response);
}
