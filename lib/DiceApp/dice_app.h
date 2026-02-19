#ifndef DICE_APP_H
#define DICE_APP_H

#include "app.h"
#include "display.h"
#include "mpu.h"

enum DiceState
{
    DICE_IDLE,
    DICE_ROLLING,
    DICE_RESULT
};

class DiceApp : public App
{
public:
    DiceApp();

    void onEnter() override;
    void onExit() override;
    void loop() override;
    const char *name() const override { return "Dice"; }

private:
    DiceState _state;
    int _currentFace;
    int _result;
    unsigned long _rollStartMs;
    unsigned long _lastFrameMs;
    unsigned long _lastUpdate;
    int _rollInterval;   // ms between face changes (decreases as roll slows)

    void drawDiceFace(GFXcanvas16 *canvas, int face);
    void drawDot(GFXcanvas16 *canvas, int cx, int cy);
    void startRoll();
    void updateRoll();
};

extern DiceApp diceApp;

#endif // DICE_APP_H
