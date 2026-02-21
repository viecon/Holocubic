#include "fish_tank_app.h"
#include "display.h"
#include <Arduino.h>

FishTankApp fishTankApp;

// Fish colors
static const uint16_t FISH_COLORS[] = {
    0xFD20, // Orange
    0xF81F, // Magenta
    0x07FF, // Cyan
    0xFFE0, // Yellow
};

// Plant X positions (within 128px canvas)
static const int PLANT_X[] = {20, 64, 108};

FishTankApp::FishTankApp() : _lastUpdate(0)
{
}

void FishTankApp::onEnter()
{
    Serial.println("[FishTankApp] Enter");
    initFish();
    initBubbles();
    _lastUpdate = millis();
}

void FishTankApp::onExit()
{
    Serial.println("[FishTankApp] Exit");
}

void FishTankApp::initFish()
{
    for (int i = 0; i < NUM_FISH; i++)
    {
        _fish[i].x = random(10, 118);
        _fish[i].y = random(10, 90);
        float speed = 0.15f + (random(70) / 100.0f); // 0.15 to 0.85 px/frame
        _fish[i].vx = (random(2) == 0 ? 1 : -1) * speed;
        _fish[i].vy = (random(2) == 0 ? 1 : -1) * (speed * 0.4f);
        _fish[i].color = FISH_COLORS[i % 4];
        _fish[i].facingRight = (_fish[i].vx > 0);
    }
}

void FishTankApp::initBubbles()
{
    for (int i = 0; i < NUM_BUBBLES; i++)
    {
        _bubbles[i].x = random(4, 124);
        _bubbles[i].y = random(20, 128);
        _bubbles[i].speed = 0.3f + (random(70) / 100.0f);
        _bubbles[i].size = 1 + random(3);
    }
}

void FishTankApp::updateFish(float dt)
{
    for (int i = 0; i < NUM_FISH; i++)
    {
        _fish[i].x += _fish[i].vx * dt * 30.0f;
        _fish[i].y += _fish[i].vy * dt * 30.0f;

        // Bounce off horizontal walls
        if (_fish[i].x < 6 || _fish[i].x > 122)
        {
            _fish[i].vx = -_fish[i].vx;
            _fish[i].facingRight = (_fish[i].vx > 0);
        }
        // Bounce off vertical walls (keep away from plant area at bottom)
        if (_fish[i].y < 6 || _fish[i].y > 95)
        {
            _fish[i].vy = -_fish[i].vy;
        }

        // Clamp positions
        if (_fish[i].x < 6) _fish[i].x = 6;
        if (_fish[i].x > 122) _fish[i].x = 122;
        if (_fish[i].y < 6) _fish[i].y = 6;
        if (_fish[i].y > 95) _fish[i].y = 95;
    }
}

void FishTankApp::updateBubbles(float dt)
{
    for (int i = 0; i < NUM_BUBBLES; i++)
    {
        _bubbles[i].y -= _bubbles[i].speed * dt * 30.0f;

        // Slight horizontal drift
        _bubbles[i].x += sinf(_bubbles[i].y * 0.1f) * 0.3f;

        // Reset bubble when it reaches top
        if (_bubbles[i].y < 2)
        {
            _bubbles[i].y = 127; // Respawn at bottom (canvas valid range: 0-127)
            _bubbles[i].x = random(4, 124);
            _bubbles[i].speed = 0.3f + (random(70) / 100.0f);
        }
    }
}

void FishTankApp::drawFish(GFXcanvas16 *canvas, const Fish &fish)
{
    int x = (int)fish.x;
    int y = (int)fish.y;

    if (fish.facingRight)
    {
        // Body
        canvas->fillRect(x - 5, y - 2, 8, 4, fish.color);
        // Tail
        canvas->fillTriangle(x - 5, y, x - 9, y - 3, x - 9, y + 3, fish.color);
        // Eye
        canvas->drawPixel(x + 1, y - 1, 0x0000);
    }
    else
    {
        // Body
        canvas->fillRect(x - 3, y - 2, 8, 4, fish.color);
        // Tail
        canvas->fillTriangle(x + 3, y, x + 7, y - 3, x + 7, y + 3, fish.color);
        // Eye
        canvas->drawPixel(x - 1, y - 1, 0x0000);
    }
}

void FishTankApp::drawBubbles(GFXcanvas16 *canvas)
{
    for (int i = 0; i < NUM_BUBBLES; i++)
    {
        int x = (int)_bubbles[i].x;
        int y = (int)_bubbles[i].y;
        int r = _bubbles[i].size;
        // Draw hollow circle for bubble
        canvas->drawCircle(x, y, r, 0xBDF7); // Light blue/white
    }
}

void FishTankApp::drawPlants(GFXcanvas16 *canvas)
{
    for (int i = 0; i < NUM_PLANTS; i++)
    {
        int px = PLANT_X[i];
        int baseY = 127; // Bottom of canvas (valid range: 0-127)
        int height = 18 + (i * 4);

        // Draw stem
        for (int seg = 0; seg < 3; seg++)
        {
            int sy = baseY - seg * (height / 3);
            int ey = baseY - (seg + 1) * (height / 3);
            // Gentle sway based on segment
            int sway = (seg == 2) ? 2 : (seg == 1 ? 1 : 0);
            canvas->drawLine(px, sy, px + sway, ey, 0x07E0); // Green
        }

        // Draw leaf clusters
        int topY = baseY - height;
        int topX = px + 2;
        canvas->fillCircle(topX, topY, 4, 0x0400);      // Dark green leaf
        canvas->fillCircle(topX - 3, topY + 3, 3, 0x0400);
        canvas->fillCircle(topX + 3, topY + 3, 3, 0x0400);
    }
}

void FishTankApp::loop()
{
    unsigned long now = millis();
    float dt = (now - _lastUpdate) / 1000.0f;
    if (dt < 0.05f) return; // Cap at ~20 FPS
    _lastUpdate = now;

    updateFish(dt);
    updateBubbles(dt);

    GFXcanvas16 *canvas = display.getBackCanvas();
    if (!canvas) return;

    // Draw water background gradient (dark blue at top, lighter below)
    for (int y = 0; y < 128; y += 8)
    {
        uint16_t col = (y < 64) ? 0x0011 : 0x0019;
        canvas->fillRect(0, y, 128, 8, col);
    }

    drawPlants(canvas);
    drawBubbles(canvas);

    for (int i = 0; i < NUM_FISH; i++)
        drawFish(canvas, _fish[i]);

    updateOverlay();
    if (isOverlayVisible())
        display.drawOverlay(false, display.getTimeString(), name(), 0, 0);

    display.swapAndRender();
}
