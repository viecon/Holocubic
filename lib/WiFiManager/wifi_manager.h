#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

class WiFiManager
{
public:
    void begin();
    bool isConnected() const;

private:
    bool loadConfig(String &ssid, String &password);
    bool connectStation(const String &ssid, const String &password);
    void startAccessPoint();
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
