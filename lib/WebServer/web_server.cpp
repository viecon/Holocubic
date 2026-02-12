#include "web_server.h"
#include "web_html.h"
#include "upload_manager.h"
#include "gif_routes.h"
#include "np_routes.h"
#include "app.h"
#include <WiFi.h>
#include <SD.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

HoloWebServer webServer;

HoloWebServer::HoloWebServer(uint16_t port)
    : _server(port), _onModeChange(nullptr),
      _apps(nullptr), _appCount(nullptr), _currentIndex(nullptr)
{
    _ipBuf[0] = '\0';
}

void HoloWebServer::begin()
{
    setupRoutes();
    _server.begin();
    Serial.println("[WebServer] Started on port 80");
}

void HoloWebServer::setOnGifChange(void (*callback)())
{
    GifRoutes::setOnGifChange(callback);
}

void HoloWebServer::setOnModeChange(void (*callback)(int))
{
    _onModeChange = callback;
}

void HoloWebServer::setAppInfo(App **apps, const int *appCount, int *currentIndex)
{
    _apps = apps;
    _appCount = appCount;
    _currentIndex = currentIndex;
}

const char *HoloWebServer::getLocalIP()
{
    IPAddress ip = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
                       ? WiFi.softAPIP()
                       : WiFi.localIP();
    snprintf(_ipBuf, sizeof(_ipBuf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return _ipBuf;
}

bool HoloWebServer::isUploading() const
{
    return uploadManager.isUploading();
}

void HoloWebServer::checkUploadTimeout()
{
    uploadManager.checkTimeout();
}

void HoloWebServer::setupRoutes()
{
    // HTML pages
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                   const size_t len = strlen(INDEX_HTML);
                   auto *response = request->beginResponse("text/html", len,
                       [len](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                           if (index >= len) return 0;
                           size_t remaining = len - index;
                           size_t toWrite = (remaining < maxLen) ? remaining : maxLen;
                           memcpy(buffer, INDEX_HTML + index, toWrite);
                           return toWrite;
                       });
                   request->send(response); });

    _server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                   const size_t len = strlen(WIFI_HTML);
                   auto *response = request->beginResponse("text/html", len,
                       [len](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                           if (index >= len) return 0;
                           size_t remaining = len - index;
                           size_t toWrite = (remaining < maxLen) ? remaining : maxLen;
                           memcpy(buffer, WIFI_HTML + index, toWrite);
                           return toWrite;
                       });
                   request->send(response); });

    // GIF routes
    GifRoutes::registerRoutes(_server);

    // NowPlaying routes
    NpRoutes::registerRoutes(_server);

    // Upload abort
    _server.on("/api/abort-upload", HTTP_POST, [](AsyncWebServerRequest *request)
               {
                   uploadManager.abort();
                   request->send(200, "application/json", "{\"success\":true}"); });

    // WiFi routes
    _server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest *request)
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
                   request->send(200, "application/json", response); });

    auto *wifiHandler = new AsyncCallbackJsonWebHandler(
        "/api/wifi",
        [](AsyncWebServerRequest *request, JsonVariant &json)
        {
            JsonObject obj = json.as<JsonObject>();
            const char *ssid = obj["ssid"] | "";
            const char *password = obj["password"] | "";

            if (strlen(ssid) == 0)
            {
                request->send(400, "application/json", "{\"error\":\"SSID is required\"}");
                return;
            }

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

            delay(500);
            ESP.restart();
        });
    _server.addHandler(wifiHandler);

    // Mode routes
    _server.on("/api/mode", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
                   if (!_apps || !_appCount || !_currentIndex)
                   {
                       request->send(500, "application/json", "{\"error\":\"App info not set\"}");
                       return;
                   }

                   JsonDocument doc;
                   doc["current"] = *_currentIndex;
                   JsonArray arr = doc["apps"].to<JsonArray>();
                   for (int i = 0; i < *_appCount; i++)
                   {
                       arr.add(_apps[i]->name());
                   }

                   String response;
                   serializeJson(doc, response);
                   request->send(200, "application/json", response); });

    auto *modeHandler = new AsyncCallbackJsonWebHandler(
        "/api/mode",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
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
        });
    _server.addHandler(modeHandler);

    _server.onNotFound([](AsyncWebServerRequest *request)
                       { request->send(404, "text/plain", "Not Found"); });
}