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
#include "app.h"
#include "gif_app.h"
#include "now_playing_app.h"

App *apps[] = {&gifApp, &nowPlayingApp};
extern const int APP_COUNT = sizeof(apps) / sizeof(apps[0]);
int currentAppIndex = 0;

unsigned long overlayTriggerTime = 0;
bool overlayVisible = false;

void connectWiFi();
void switchApp(int newIndex);

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("[Main] Starting");

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

  webServer.setOnGifChange([]()
                           { gifApp.notifyGifChange(); });
  webServer.setOnModeChange([](int index)
                            {
                              if (index >= 0 && index < APP_COUNT && index != currentAppIndex)
                                switchApp(index); });
  webServer.setAppInfo(apps, &APP_COUNT, &currentAppIndex);
  webServer.begin();

  display.showIP(webServer.getLocalIP().c_str());
  Serial.printf("[Main] Web UI: http://%s\n", webServer.getLocalIP().c_str());
  delay(3000);

  display.clear();

  currentAppIndex = 0;
  apps[currentAppIndex]->onEnter();
  overlayVisible = true;
  overlayTriggerTime = millis();

  Serial.printf("[Main] Ready! Running %s app. %d GIFs found.\n",
                apps[currentAppIndex]->name(), gifManager.getGifCount());
}

void loop()
{
  static unsigned long lastMpuCheck = 0;
  int tiltDir = 0;
  int pitchDir = 0;
  if (millis() - lastMpuCheck >= 50)
  {
    lastMpuCheck = millis();
    tiltDir = mpu.checkTiltChange();
    pitchDir = mpu.checkPitchChange();
  }

  if (pitchDir != 0)
  {
    int newIdx = currentAppIndex + pitchDir;
    if (newIdx >= APP_COUNT)
      newIdx = 0;
    if (newIdx < 0)
      newIdx = APP_COUNT - 1;
    switchApp(newIdx);
  }

  if (tiltDir != 0)
  {
    if (apps[currentAppIndex]->onTilt(tiltDir))
    {
      overlayVisible = true;
      overlayTriggerTime = millis();
    }
  }

  if (overlayVisible && (millis() - overlayTriggerTime >= OVERLAY_SHOW_MS))
  {
    overlayVisible = false;
  }

  webServer.checkUploadTimeout();
  apps[currentAppIndex]->loop();
}

void switchApp(int newIndex)
{
  if (newIndex < 0 || newIndex >= APP_COUNT || newIndex == currentAppIndex)
    return;

  apps[currentAppIndex]->onExit();
  currentAppIndex = newIndex;
  apps[currentAppIndex]->onEnter();

  overlayVisible = true;
  overlayTriggerTime = millis();
  Serial.printf("[Main] Switched to %s app\n", apps[currentAppIndex]->name());
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
