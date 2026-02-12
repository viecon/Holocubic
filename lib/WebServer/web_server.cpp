#include "web_server.h"
#include "gif_manager.h"
#include "now_playing_app.h"
#include "app.h"
#include <WiFi.h>
#include <SD.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <vector>

HoloWebServer webServer;

HoloWebServer::HoloWebServer(uint16_t port)
    : _server(port), _onGifChange(nullptr), _onModeChange(nullptr), _uploadFrameIndex(-1)
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

void HoloWebServer::setOnModeChange(void (*callback)(int))
{
    _onModeChange = callback;
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
        [this](AsyncWebServerRequest *request)
        {
            if (_uploadError)
            {
                _uploadError = false;
                request->send(500, "application/json", "{\"error\":\"SD write failed\"}");
            }
            else
            {
                request->send(200, "application/json", "{\"success\":true}");
            }
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
        [this](AsyncWebServerRequest *request)
        {
            if (_uploadError)
            {
                _uploadError = false;
                request->send(500, "application/json", "{\"error\":\"SD write failed\"}");
            }
            else
            {
                request->send(200, "application/json", "{\"success\":true}");
            }
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

    // Now Playing routes
    _server.on("/api/now-playing", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetNowPlaying(request); });

    AsyncCallbackJsonWebHandler *npHandler = new AsyncCallbackJsonWebHandler(
        "/api/now-playing",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            handleNowPlaying(request, json);
        });
    _server.addHandler(npHandler);

    _server.on(
        "^\\/api\\/np\\/frame\\/([0-9]+)$",
        HTTP_POST,
        [this](AsyncWebServerRequest *request)
        {
            if (_uploadError)
            {
                _uploadError = false;
                request->send(500, "application/json", "{\"error\":\"SD write failed\"}");
            }
            else
            {
                request->send(200, "application/json", "{\"success\":true}");
            }
        },
        [this](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleUploadNpFrame(request, filename, index, data, len, final);
        });

    _server.on("/api/np/ready", HTTP_POST, [this](AsyncWebServerRequest *request)
               { handleNpReady(request); });

    _server.on("/api/mode", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetMode(request); });

    AsyncCallbackJsonWebHandler *modeHandler = new AsyncCallbackJsonWebHandler(
        "/api/mode",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        {
            handleSetMode(request, json);
        });
    _server.addHandler(modeHandler);

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
        _uploadError = false;
        _lastUploadMs = millis();

        snprintf(_uploadPath, sizeof(_uploadPath), "%s/%s/%s.bmp",
                 GIFS_ROOT, request->pathArg(0).c_str(), request->pathArg(1).c_str());

        _uploadFile = SD.open(_uploadPath, FILE_WRITE);
        if (!_uploadFile)
        {
            Serial.printf("[WebServer] Cannot create file: %s\n", _uploadPath);
            _uploadError = true;
            return;
        }
        _fileOpen = true;
    }

    if (_fileOpen && !_uploadError)
    {
        _lastUploadMs = millis();
        size_t written = _uploadFile.write(data, len);
        if (written != len)
        {
            Serial.printf("[WebServer] SD write error: wrote %u/%u\n", written, len);
            _uploadError = true;
        }
    }

    if (final)
    {
        if (_fileOpen)
        {
            _uploadFile.close();
            _fileOpen = false;
        }
        _uploadFrameIndex = -1;
    }
}

void HoloWebServer::abortUpload()
{
    if (_fileOpen)
    {
        _uploadFile.close();
        _fileOpen = false;
    }
    if (_origFileOpen)
    {
        _uploadOriginalFile.close();
        _origFileOpen = false;
    }
    _isUploading = false;
    _uploadError = false;
    _uploadFrameIndex = -1;
    Serial.printf("[WebServer] Upload aborted. Free heap: %u\n", ESP.getFreeHeap());
}

void HoloWebServer::checkUploadTimeout()
{
    if (_isUploading && _lastUploadMs > 0 &&
        (millis() - _lastUploadMs) > UPLOAD_TIMEOUT_MS)
    {
        Serial.printf("[WebServer] Upload timeout (%us idle), auto-recovering\n",
                      UPLOAD_TIMEOUT_MS / 1000);
        // Only clear _isUploading so frame loader can resume.
        // Do NOT set _uploadError — it belongs to the upload handler
        // context (async TCP task) and setting it here races with
        // the response lambda, causing spurious 500 responses.
        _isUploading = false;
        _lastUploadMs = 0;
    }
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
        _uploadError = false;
        _lastUploadMs = millis();

        snprintf(_uploadPath, sizeof(_uploadPath), "%s/%s/original.gif",
                 GIFS_ROOT, request->pathArg(0).c_str());

        _uploadOriginalFile = SD.open(_uploadPath, FILE_WRITE);
        if (!_uploadOriginalFile)
        {
            Serial.printf("[WebServer] Cannot create original: %s\n", _uploadPath);
            _uploadError = true;
            return;
        }
        _origFileOpen = true;
    }

    if (_origFileOpen && !_uploadError)
    {
        _lastUploadMs = millis();
        size_t written = _uploadOriginalFile.write(data, len);
        if (written != len)
        {
            Serial.printf("[WebServer] Original write error: wrote %u/%u\n", written, len);
            _uploadError = true;
        }
    }

    if (final)
    {
        if (_origFileOpen)
        {
            _uploadOriginalFile.close();
            _origFileOpen = false;
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

void HoloWebServer::handleNowPlaying(AsyncWebServerRequest *request, JsonVariant &json)
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

    // Only store metadata — frames will be uploaded later
    // when the companion detects NowPlaying app is active
    nowPlayingApp.updateTrack(title, artist, frameCount);

    Serial.printf("[WebServer] NP track: \"%s\" (%d frames). Free heap: %u\n",
                  title, frameCount, ESP.getFreeHeap());
    request->send(200, "application/json", "{\"success\":true}");
}

void HoloWebServer::handleGetNowPlaying(AsyncWebServerRequest *request)
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

void HoloWebServer::handleUploadNpFrame(AsyncWebServerRequest *request, const String &filename,
                                        size_t index, uint8_t *data, size_t len, bool final)
{
    if (index == 0)
    {
        // Close previously leaked file handle if any
        if (_fileOpen)
        {
            _uploadFile.close();
            _fileOpen = false;
        }

        _uploadError = false;
        _lastUploadMs = millis();

        // On first chunk of first frame, clean old files and set uploading
        if (request->pathArg(0) == "0")
        {
            _isUploading = true;

            // Clean old NP frames
            File dir = SD.open(NP_DIR);
            if (dir && dir.isDirectory())
            {
                File entry = dir.openNextFile();
                while (entry)
                {
                    // entry.name() returns full path on ESP32
                    const char *fullPath = entry.name();
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
                    Serial.printf("[WebServer] Failed to create %s\n", NP_DIR);
                }
            }
            Serial.printf("[WebServer] NP upload start. Free heap: %u\n", ESP.getFreeHeap());
        }

        snprintf(_uploadPath, sizeof(_uploadPath), "%s/%s.bmp",
                 NP_DIR, request->pathArg(0).c_str());

        _uploadFile = SD.open(_uploadPath, FILE_WRITE);
        if (!_uploadFile)
        {
            Serial.printf("[WebServer] Cannot create: %s\n", _uploadPath);
            _uploadError = true;
            return;
        }
        _fileOpen = true;
    }

    if (_fileOpen && !_uploadError)
    {
        _lastUploadMs = millis();
        size_t written = _uploadFile.write(data, len);
        if (written != len)
        {
            Serial.printf("[WebServer] NP write error: wrote %u/%u\n", written, len);
            _uploadError = true;
        }
    }

    if (final)
    {
        if (_fileOpen)
        {
            _uploadFile.close();
            _fileOpen = false;
        }
    }
}

void HoloWebServer::handleNpReady(AsyncWebServerRequest *request)
{
    _isUploading = false;
    nowPlayingApp.setFramesReady();
    Serial.printf("[WebServer] NP frames ready. Free heap: %u\n", ESP.getFreeHeap());
    request->send(200, "application/json", "{\"success\":true}");
}

void HoloWebServer::handleSetMode(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject obj = json.as<JsonObject>();
    int app = obj["app"] | -1;

    if (app < 0)
    {
        request->send(400, "application/json", "{\"error\":\"Invalid app index\"}");
        return;
    }

    if (_onModeChange)
        _onModeChange(app);

    request->send(200, "application/json", "{\"success\":true}");
}

void HoloWebServer::handleGetMode(AsyncWebServerRequest *request)
{
    extern App *apps[];
    extern const int APP_COUNT;
    extern int currentAppIndex;

    JsonDocument doc;
    doc["current"] = currentAppIndex;
    JsonArray arr = doc["apps"].to<JsonArray>();
    for (int i = 0; i < APP_COUNT; i++)
    {
        arr.add(apps[i]->name());
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}
