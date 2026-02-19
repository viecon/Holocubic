#ifndef RACING_APP_H
#define RACING_APP_H

#include "app.h"
#include "display.h"
#include "mpu.h"

enum RacingState { RACING_PLAYING, RACING_GAMEOVER };

class RacingApp : public App
{
public:
    RacingApp();
    void onEnter() override;
    void onExit() override;
    void loop() override;
    const char *name() const override { return "Racing"; }

private:
    // Perspective constants
    static const int HORIZON_Y   = 36;   // y where road meets sky
    static const int PERSP_K     = 91;   // = 127 - HORIZON_Y
    static const int ROAD_MIN_HW = 4;    // road half-width at horizon (px)
    static const int ROAD_MAX_HW = 42;   // road half-width at bottom (px)
    static const int CAR_W       = 10;
    static const int CAR_H       = 14;

    RacingState   _state;
    float _carX;             // car center X (absolute pixels)
    float _roadCenterX;      // road center X at car depth
    float _curvature;        // current curvature (-1=hard left, +1=hard right)
    float _targetCurvature;
    unsigned long _curvatureChangeMs;
    float _speed;            // world-units / sec
    float _distance;         // total distance traveled (for strip animation)
    int   _score;
    unsigned long _lastUpdate;

    void resetGame();
    void drawScene(GFXcanvas16 *canvas);
    void drawCar(GFXcanvas16 *canvas);
    void drawHUD(GFXcanvas16 *canvas);
    void drawGameOver(GFXcanvas16 *canvas);
};

extern RacingApp racingApp;

#endif // RACING_APP_H
