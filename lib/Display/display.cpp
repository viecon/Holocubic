#include "display.h"
#include <SD.h>

Display display;

Display::Display()
    : _tft(TFT_CS, TFT_DC, TFT_RST), _frontIdx(0)
{
    _canvas[0] = nullptr;
    _canvas[1] = nullptr;
}

void Display::begin()
{
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, LOW);
    delay(20);
    digitalWrite(TFT_RST, HIGH);
    delay(50);

    _tft.initR(INITR_BLACKTAB);
    _tft.setRotation(0);
    _tft.fillScreen(ST77XX_BLACK);

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    _canvas[0] = new GFXcanvas16(CANVAS_WIDTH, CANVAS_HEIGHT);
    _canvas[1] = new GFXcanvas16(CANVAS_WIDTH, CANVAS_HEIGHT);
    if (!_canvas[0] || !_canvas[1])
    {
        Serial.println("[Display] Canvas allocation failed!");
    }
    _frontIdx = 0;
}

void Display::clear()
{
    _tft.fillScreen(ST77XX_BLACK);
}

void Display::showMessage(const String &msg, int x, int y)
{
    _tft.setCursor(x, y);
    _tft.setTextColor(ST77XX_WHITE);
    _tft.setTextSize(1);
    _tft.print(msg);
}

void Display::showIP(const String &ip)
{
    clear();
    _tft.setCursor(10, 60);
    _tft.setTextColor(ST77XX_WHITE);
    _tft.setTextSize(1);
    _tft.print("Web UI:");
    _tft.setCursor(10, 80);
    _tft.print("http://");
    _tft.print(ip);
}

void Display::clearBackBuffer()
{
    int backIdx = 1 - _frontIdx;
    if (_canvas[backIdx])
    {
        _canvas[backIdx]->fillScreen(ST77XX_BLACK);
    }
}

uint16_t *Display::getBackBuffer()
{
    int backIdx = 1 - _frontIdx;
    return _canvas[backIdx] ? _canvas[backIdx]->getBuffer() : nullptr;
}

void Display::renderCanvas()
{
    // Render front buffer
    if (!_canvas[_frontIdx])
        return;

    _tft.startWrite();
    _tft.setAddrWindow(CANVAS_X, CANVAS_Y, CANVAS_WIDTH, CANVAS_HEIGHT);
    _tft.writePixels(_canvas[_frontIdx]->getBuffer(), CANVAS_WIDTH * CANVAS_HEIGHT);
    _tft.endWrite();
}

void Display::swapAndRender()
{
    _frontIdx = 1 - _frontIdx;
    renderCanvas();
}

bool Display::decodeBmpToCanvas(const char *filename)
{
    File bmp = SD.open(filename);
    if (!bmp)
    {
        Serial.printf("[Display] Missing: %s\n", filename);
        clearBackBuffer();
        return false;
    }

    uint8_t header[54];
    if (bmp.read(header, 54) != 54 || header[0] != 'B' || header[1] != 'M')
    {
        Serial.println("[Display] Invalid BMP");
        bmp.close();
        return false;
    }

    uint32_t dataOffset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);
    int32_t w = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    int32_t h = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
    uint16_t bits = header[28] | (header[29] << 8);
    uint32_t comp = header[30] | (header[31] << 8) | (header[32] << 16) | (header[33] << 24);
    bool is16bit = (bits == 16 && comp == 3);
    bool is24bit = (bits == 24 && comp == 0);
    if (!is16bit && !is24bit)
    {
        Serial.printf("[Display] Unsupported BMP: %d-bit, comp=%d\n", bits, comp);
        bmp.close();
        return false;
    }

    bool flip = (h > 0);
    if (h < 0)
        h = -h;

    int bytesPerPixel = is16bit ? 2 : 3;
    uint32_t rowSize = ((w * bytesPerPixel + 3) & ~3);
    if (rowSize > MAX_ROW_BUFFER)
    {
        Serial.println("[Display] BMP too wide");
        bmp.close();
        return false;
    }

    if (w < CANVAS_WIDTH || h < CANVAS_HEIGHT)
    {
        clearBackBuffer();
    }

    int offsetX = (CANVAS_WIDTH - w) >> 1;
    int offsetY = (CANVAS_HEIGHT - h) >> 1;
    if (offsetX < 0)
        offsetX = 0;
    if (offsetY < 0)
        offsetY = 0;

    int copyW = w;
    if (offsetX + copyW > CANVAS_WIDTH)
        copyW = CANVAS_WIDTH - offsetX;

    int backIdx = 1 - _frontIdx;
    uint16_t *fb = _canvas[backIdx]->getBuffer();

    bmp.seek(dataOffset);

    for (int row = 0; row < h; row++)
    {
        int canvasRow = offsetY + (flip ? (h - 1 - row) : row);
        if (canvasRow < 0 || canvasRow >= CANVAS_HEIGHT)
        {
            bmp.seek(bmp.position() + rowSize);
            continue;
        }

        bmp.read(_rowBuf, rowSize);

        uint16_t *dst = fb + canvasRow * CANVAS_WIDTH + offsetX;
        uint8_t *p = _rowBuf;

        if (is16bit)
        {
            memcpy(dst, p, copyW * 2);
        }
        else
        {
            int i = 0;
            int fastEnd = copyW - 3;
            for (; i <= fastEnd; i += 4)
            {
                dst[i] = ((p[2] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[0] >> 3);
                dst[i + 1] = ((p[5] & 0xF8) << 8) | ((p[4] & 0xFC) << 3) | (p[3] >> 3);
                dst[i + 2] = ((p[8] & 0xF8) << 8) | ((p[7] & 0xFC) << 3) | (p[6] >> 3);
                dst[i + 3] = ((p[11] & 0xF8) << 8) | ((p[10] & 0xFC) << 3) | (p[9] >> 3);
                p += 12;
            }
            for (; i < copyW; i++)
            {
                dst[i] = ((p[2] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[0] >> 3);
                p += 3;
            }
        }
    }

    bmp.close();
    return true;
}

static void dimCanvasRegion(uint16_t *fb, int y, int h, int w)
{
    for (int row = y; row < y + h && row < CANVAS_HEIGHT; row++)
    {
        uint16_t *p = fb + row * CANVAS_WIDTH;
        for (int x = 0; x < w; x++)
        {
            p[x] = ((p[x] >> 1) & 0x7BEF);
        }
    }
}

void Display::drawOverlay(bool wifiConnected, const char *timeStr,
                          const String &gifName, int current, int total)
{
    int backIdx = 1 - _frontIdx;
    if (!_canvas[backIdx])
        return;

    uint16_t *fb = _canvas[backIdx]->getBuffer();

    // --- Top overlay bar (first OVERLAY_HEIGHT rows) ---
    dimCanvasRegion(fb, 0, OVERLAY_HEIGHT, CANVAS_WIDTH);

    _canvas[backIdx]->setTextSize(1);

    // WiFi indicator (left)
    _canvas[backIdx]->setCursor(2, 4);
    if (wifiConnected)
    {
        _canvas[backIdx]->setTextColor(ST77XX_GREEN);
        _canvas[backIdx]->print("WiFi");
    }
    else
    {
        _canvas[backIdx]->setTextColor(ST77XX_RED);
        _canvas[backIdx]->print("----");
    }

    // Time (right)
    if (timeStr && timeStr[0] != '\0')
    {
        _canvas[backIdx]->setTextColor(ST77XX_WHITE);
        int tw = strlen(timeStr) * 6;
        _canvas[backIdx]->setCursor(CANVAS_WIDTH - tw - 2, 4);
        _canvas[backIdx]->print(timeStr);
    }

    // --- Bottom overlay bar (last OVERLAY_HEIGHT rows) ---
    int bottomY = CANVAS_HEIGHT - OVERLAY_HEIGHT;
    dimCanvasRegion(fb, bottomY, OVERLAY_HEIGHT, CANVAS_WIDTH);

    // GIF name (left, truncate)
    _canvas[backIdx]->setCursor(2, bottomY + 4);
    _canvas[backIdx]->setTextColor(ST77XX_CYAN);
    char displayName[16];
    size_t nameLen = gifName.length();
    if (nameLen > 14)
    {
        memcpy(displayName, gifName.c_str(), 12);
        displayName[12] = '.';
        displayName[13] = '.';
        displayName[14] = '\0';
    }
    else
    {
        memcpy(displayName, gifName.c_str(), nameLen + 1);
    }
    _canvas[backIdx]->print(displayName);

    // Position "3/10" (right)
    _canvas[backIdx]->setTextColor(ST77XX_YELLOW);
    char posStr[12];
    int posLen = snprintf(posStr, sizeof(posStr), "%d/%d", current, total);
    int pw = posLen * 6;
    _canvas[backIdx]->setCursor(CANVAS_WIDTH - pw - 2, bottomY + 4);
    _canvas[backIdx]->print(posStr);
}
