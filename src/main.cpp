#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "config.h"
#include "display.h"
#include "mpu.h"
#include "gif_manager.h"
#include "web_server.h"
#include "wifi_manager.h"
#include "app.h"
#include "gif_app.h"
#include "now_playing_app.h"
#include "fish_tank_app.h"
#include "dice_app.h"
#include "racing_app.h"

App *apps[] = {&gifApp, &nowPlayingApp, &fishTankApp, &diceApp, &racingApp};
extern const int APP_COUNT = sizeof(apps) / sizeof(apps[0]);
int currentAppIndex = 0;

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
  wifiManager.begin();

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

  display.showIP(webServer.getLocalIP());
  Serial.printf("[Main] Web UI: http://%s\n", webServer.getLocalIP());
  delay(3000);

  display.clear();

  currentAppIndex = 0;
  apps[currentAppIndex]->onEnter();
  apps[currentAppIndex]->triggerOverlay();

  Serial.printf("[Main] Ready! Running %s app. %d GIFs found.\n",
                apps[currentAppIndex]->name(), gifManager.getGifCount());
}

void loop()
{
  static unsigned long lastMpuCheck = 0;
  int tiltDir = 0;
  if (millis() - lastMpuCheck >= 50)
  {
    lastMpuCheck = millis();
    tiltDir = mpu.checkTiltChange();
  }

  if (tiltDir != 0)
  {
    if (apps[currentAppIndex]->onTilt(tiltDir))
      apps[currentAppIndex]->triggerOverlay();
  }

  apps[currentAppIndex]->updateOverlay();
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
  apps[currentAppIndex]->triggerOverlay();
  Serial.printf("[Main] Switched to %s app\n", apps[currentAppIndex]->name());
}
