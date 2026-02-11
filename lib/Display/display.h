#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <FS.h>
#include "config.h"

class Display
{
public:
    Display();

    void begin();
    void clear();
    void showMessage(const String &msg, int x = 10, int y = 70);
    void showIP(const String &ip);

    void swapAndRender();
    uint16_t *getBackBuffer();
    void clearBackBuffer();

    bool decodeBmpToCanvas(const char *filename);
    void drawOverlay(bool wifiConnected, const char *timeStr,
                     const String &gifName, int current, int total);

private:
    Adafruit_ST7735 _tft;
    GFXcanvas16 *_canvas[2];
    int _frontIdx;
    uint8_t _rowBuf[MAX_ROW_BUFFER];

    void renderCanvas();
};

extern Display display;

#endif // DISPLAY_H
