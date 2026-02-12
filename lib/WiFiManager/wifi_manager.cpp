#include "wifi_manager.h"
#include "display.h"
#include "config.h"
#include <WiFi.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <time.h>

WiFiManager wifiManager;

bool WiFiManager::loadConfig(String &ssid, String &password)
{
    if (!SD.exists(WIFI_CONFIG_FILE))
    {
        Serial.println("[WiFi] No wifi.json found");
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
        Serial.println("[WiFi] SSID is empty");
        return false;
    }

    Serial.printf("[WiFi] Config: SSID=\"%s\"\n", ssid.c_str());
    return true;
}

bool WiFiManager::connectStation(const String &ssid, const String &password)
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
        Serial.printf("\n[WiFi] Connected: %s\n", WiFi.localIP().toString().c_str());
        configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SERVER);
        return true;
    }

    Serial.printf("\n[WiFi] Failed to connect to \"%s\"\n", ssid.c_str());
    WiFi.disconnect(true);
    return false;
}

void WiFiManager::startAccessPoint()
{
    Serial.println("[WiFi] Starting AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("[WiFi] AP: %s / %s\n", AP_SSID, AP_PASSWORD);
    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

    display.clear();
    display.showMessage("WiFi Setup", 10, 30);
    display.showMessage(String("AP: ") + AP_SSID, 10, 50);
    display.showMessage(WiFi.softAPIP().toString(), 10, 70);
}

void WiFiManager::begin()
{
    String ssid, password;
    if (loadConfig(ssid, password) && connectStation(ssid, password))
        return;
    startAccessPoint();
}

bool WiFiManager::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}
