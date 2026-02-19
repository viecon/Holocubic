#ifndef FISH_TANK_APP_H
#define FISH_TANK_APP_H

#include "app.h"
#include "display.h"

struct Fish
{
    float x, y;
    float vx, vy;
    uint16_t color;
    bool facingRight;
};

struct Bubble
{
    float x, y;
    float speed;
    uint8_t size;
};

class FishTankApp : public App
{
public:
    FishTankApp();

    void onEnter() override;
    void onExit() override;
    void loop() override;
    const char *name() const override { return "FishTank"; }

private:
    static const int NUM_FISH = 4;
    static const int NUM_BUBBLES = 8;
    static const int NUM_PLANTS = 3;

    Fish _fish[NUM_FISH];
    Bubble _bubbles[NUM_BUBBLES];
    unsigned long _lastUpdate;

    void initFish();
    void initBubbles();
    void updateFish(float dt);
    void updateBubbles(float dt);
    void drawFish(GFXcanvas16 *canvas, const Fish &fish);
    void drawBubbles(GFXcanvas16 *canvas);
    void drawPlants(GFXcanvas16 *canvas);
};

extern FishTankApp fishTankApp;

#endif // FISH_TANK_APP_H
