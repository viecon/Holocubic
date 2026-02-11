#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <SD.h>
#include <ArduinoJson.h>

#include "config.h"
#include "display.h"
#include "mpu.h"
#include "gif_manager.h"
#include "web_server.h"

// State
int currentGifIndex = 0;
int currentFrameIndex = 0;
unsigned long lastFrameTime = 0;
unsigned long overlayTriggerTime = 0;
bool overlayVisible = false;
GifInfo currentGif;
bool needRefreshGifList = false;

// Dual-core pipeline
static TaskHandle_t loaderTaskHandle = NULL;
static volatile bool frameLoaded = false;
static volatile bool loaderBusy = false;
static char nextFramePath[64];

// Function declarations
void connectWiFi();
void loadCurrentGif();
void onGifChange();
void playFrame();
const char *getTimeString();
void frameLoaderTask(void *pvParameters);

void frameLoaderTask(void *pvParameters)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (webServer.isUploading())
      continue;

    loaderBusy = true;
    if (display.decodeBmpToCanvas(nextFramePath))
    {
      frameLoaded = true;
    }
    loaderBusy = false;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Holocubic Starting ===");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
  SPI.setFrequency(SPI_FREQUENCY);

  display.begin();
  display.clear();
  display.showMessage("Initializing...");
  Serial.println("[Main] Display initialized");

  if (!gifManager.begin())
  {
    display.showMessage("SD Card Error!");
    Serial.println("[Main] SD card failed!");
    while (1)
      delay(100);
  }
  Serial.println("[Main] SD card initialized");

  display.clear();
  display.showMessage("Connecting WiFi...");
  connectWiFi();

  mpu.begin();
  delay(50);
  display.clear();
  display.showMessage("Calibrating...");
  mpu.calibrate(300);
  Serial.println("[Main] MPU calibrated");

  webServer.setOnGifChange(onGifChange);
  webServer.begin();

  display.showIP(webServer.getLocalIP().c_str());
  Serial.printf("[Main] Web UI: http://%s\n", webServer.getLocalIP().c_str());
  delay(3000);

  display.clear();
  loadCurrentGif();
  overlayVisible = true;
  overlayTriggerTime = millis();

  xTaskCreatePinnedToCore(
      frameLoaderTask,
      "FrameLoader",
      4096,
      NULL,
      1,
      &loaderTaskHandle,
      0);
  Serial.println("[Main] Dual-core frame loader started on Core 0");

  Serial.println("[Main] Ready! Tilt left/right to switch GIFs");
  Serial.printf("[Main] Found %d GIFs\n", gifManager.getGifCount());
}

void loop()
{
  // Check if GIF list needs refresh
  if (needRefreshGifList)
  {
    needRefreshGifList = false;
    gifManager.refresh();

    int count = gifManager.getGifCount();
    if (count == 0)
    {
      currentGif.valid = false;
      currentGifIndex = 0;
      display.clear();
      display.showMessage("No GIFs");
      display.showIP(webServer.getLocalIP().c_str());
    }
    else
    {
      if (currentGifIndex >= count)
      {
        currentGifIndex = 0;
      }
      loadCurrentGif();
    }
  }

  // Check tilt for GIF switching
  static unsigned long lastMpuCheck = 0;
  int tiltDir = 0;
  if (millis() - lastMpuCheck >= 50)
  {
    lastMpuCheck = millis();
    tiltDir = mpu.checkTiltChange();
  }
  if (tiltDir != 0)
  {
    int count = gifManager.getGifCount();
    if (count > 0)
    {
      currentGifIndex += tiltDir;
      if (currentGifIndex >= count)
        currentGifIndex = 0;
      if (currentGifIndex < 0)
        currentGifIndex = count - 1;

      loadCurrentGif();
      overlayVisible = true;
      overlayTriggerTime = millis();
      Serial.printf("[Main] Switched to GIF %d: %s\n", currentGifIndex, currentGif.name.c_str());
    }
  }

  // Auto-hide overlay
  if (overlayVisible && (millis() - overlayTriggerTime >= OVERLAY_SHOW_MS))
  {
    overlayVisible = false;
  }

  // Play current frame (skip if uploading)
  if (currentGif.valid && !webServer.isUploading())
  {
    playFrame();
  }
}

bool loadWiFiConfig(String &ssid, String &password)
{
  if (!SD.exists(WIFI_CONFIG_FILE))
  {
    Serial.println("[WiFi] No wifi.json found on SD card");
    return false;
  }

  File f = SD.open(WIFI_CONFIG_FILE, FILE_READ);
  if (!f)
  {
    Serial.println("[WiFi] Failed to open wifi.json");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err)
  {
    Serial.printf("[WiFi] wifi.json parse error: %s\n", err.c_str());
    return false;
  }

  ssid = doc["ssid"].as<String>();
  password = doc["password"].as<String>();

  if (ssid.length() == 0)
  {
    Serial.println("[WiFi] SSID is empty in wifi.json");
    return false;
  }

  Serial.printf("[WiFi] Loaded config: SSID=\"%s\"\n", ssid.c_str());
  return true;
}

void connectWiFi()
{
  String ssid, password;
  bool hasConfig = loadWiFiConfig(ssid, password);

  if (hasConfig)
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    int maxAttempts = WIFI_CONNECT_TIMEOUT * 2;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts)
    {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

      configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SERVER);
      Serial.println("[NTP] Time sync started");
      return;
    }

    Serial.printf("\n[WiFi] Failed to connect to \"%s\"\n", ssid.c_str());
    WiFi.disconnect(true);
  }

  // Fallback: AP mode
  Serial.println("[WiFi] Starting AP mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[WiFi] AP SSID: %s  Password: %s\n", AP_SSID, AP_PASSWORD);
  Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());

  display.clear();
  display.showMessage("WiFi Setup", 10, 30);
  display.showMessage(String("AP: ") + AP_SSID, 10, 50);
  display.showMessage(WiFi.softAPIP().toString(), 10, 70);
}

void loadCurrentGif()
{
  Serial.printf("[Main] Loading GIF index %d...\n", currentGifIndex);

  while (loaderBusy)
  {
    delay(1);
  }

  if (!gifManager.getGifInfoByIndex(currentGifIndex, currentGif))
  {
    Serial.println("[Main] Failed to get GIF info!");
    currentGif.valid = false;
    return;
  }

  currentFrameIndex = 0;
  lastFrameTime = millis();
  frameLoaded = false;

  snprintf(nextFramePath, sizeof(nextFramePath), "/gifs/%s/%d.bmp",
           currentGif.name.c_str(), currentFrameIndex);

  if (loaderTaskHandle != NULL)
  {
    // Background loader available
    xTaskNotifyGive(loaderTaskHandle);
  }
  else
  {
    // Initial setup before task is created
    if (display.decodeBmpToCanvas(nextFramePath))
    {
      frameLoaded = true;
    }
  }

  Serial.printf("[Main] Loaded GIF: %s (%d frames, %dx%d, delay=%dms)\n",
                currentGif.name.c_str(), currentGif.frameCount, currentGif.width, currentGif.height, currentGif.defaultDelay);
}

void onGifChange()
{
  needRefreshGifList = true;
}

void playFrame()
{
  unsigned long now = millis();
  uint16_t delay_ms = currentGif.defaultDelay;

  // Wait for both frame delay AND background loader to finish
  if (now - lastFrameTime >= delay_ms && frameLoaded)
  {
    lastFrameTime = now;
    frameLoaded = false;

    // Draw overlay on back buffer (which has the pre-loaded frame)
    if (overlayVisible)
    {
      bool wifi = (WiFi.status() == WL_CONNECTED);
      display.drawOverlay(wifi, getTimeString(),
                          currentGif.name, currentGifIndex + 1, gifManager.getGifCount());
    }

    // Swap back→front and render to TFT
    display.swapAndRender();

    // Advance frame index
    currentFrameIndex++;
    if (currentFrameIndex >= currentGif.frameCount)
    {
      currentFrameIndex = 0;
    }

    // Signal Core 0 to pre-load next frame into back buffer
    snprintf(nextFramePath, sizeof(nextFramePath), "/gifs/%s/%d.bmp",
             currentGif.name.c_str(), currentFrameIndex);
    xTaskNotifyGive(loaderTaskHandle);
  }
}

char _cachedTimeStr[6] = "--:--";
unsigned long _lastTimeUpdate = 0;
bool _timeSynced = false;

const char *getTimeString()
{
  unsigned long now = millis();
  // Before first sync: try every 1s; after sync: update every 10s
  unsigned long interval = _timeSynced ? 10000 : 1000;
  if (now - _lastTimeUpdate >= interval)
  {
    _lastTimeUpdate = now;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10))
    {
      strftime(_cachedTimeStr, sizeof(_cachedTimeStr), "%H:%M", &timeinfo);
      _timeSynced = true;
    }
  }
  return _cachedTimeStr;
}
