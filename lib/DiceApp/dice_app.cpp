#include "dice_app.h"
#include "display.h"
#include <Arduino.h>

DiceApp diceApp;

// Dot positions for each face (up to 7 dots per face, -1 = unused)
// Each entry: {cx_offset, cy_offset} relative to dice center (64, 64)
// Offsets for a 3x3 grid with 20px spacing
static const int8_t DOT_POS[6][7][2] = {
    // Face 1: center
    {{ 0,  0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // Face 2: top-right, bottom-left
    {{ 18,-18}, {-18, 18}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // Face 3: top-right, center, bottom-left
    {{ 18,-18}, { 0,  0}, {-18, 18}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    // Face 4: four corners
    {{ 18,-18}, {-18,-18}, { 18, 18}, {-18, 18}, {0, 0}, {0, 0}, {0, 0}},
    // Face 5: four corners + center
    {{ 18,-18}, {-18,-18}, { 18, 18}, {-18, 18}, { 0, 0}, {0, 0}, {0, 0}},
    // Face 6: two columns of three
    {{ 18,-18}, {-18,-18}, { 18,  0}, {-18,  0}, { 18, 18}, {-18, 18}, {0, 0}},
};

static const int DOT_COUNT[] = {1, 2, 3, 4, 5, 6};

DiceApp::DiceApp()
    : _state(DICE_IDLE), _currentFace(1), _result(1),
      _rollStartMs(0), _lastFrameMs(0), _lastUpdate(0), _rollInterval(80)
{
}

void DiceApp::onEnter()
{
    Serial.println("[DiceApp] Enter");
    _state = DICE_IDLE;
    _currentFace = 1;
    _lastUpdate = millis();
}

void DiceApp::onExit()
{
    Serial.println("[DiceApp] Exit");
}

void DiceApp::startRoll()
{
    _state = DICE_ROLLING;
    _rollStartMs = millis();
    _lastFrameMs = millis();
    _rollInterval = 60; // Start fast
    _result = random(1, 7);
    Serial.printf("[DiceApp] Roll started, result will be %d\n", _result);
}

void DiceApp::updateRoll()
{
    unsigned long now = millis();
    unsigned long elapsed = now - _rollStartMs;

    if (elapsed > 1800)
    {
        // Roll complete - show result
        _currentFace = _result;
        _state = DICE_RESULT;
        Serial.printf("[DiceApp] Result: %d\n", _result);
        return;
    }

    // Slow down roll over time: interval grows from 60ms to 200ms
    _rollInterval = 60 + (int)(elapsed * 0.077f);

    if (now - _lastFrameMs >= (unsigned long)_rollInterval)
    {
        _lastFrameMs = now;
        _currentFace = random(1, 7);
    }
}

void DiceApp::drawDot(GFXcanvas16 *canvas, int cx, int cy)
{
    canvas->fillCircle(cx, cy, 6, 0x0000);       // Black dot
    canvas->fillCircle(cx, cy, 5, 0x18C3);       // Dark navy fill for depth
}

void DiceApp::drawDiceFace(GFXcanvas16 *canvas, int face)
{
    int f = face - 1; // 0-indexed
    int cx = 64, cy = 64;

    // Dice body
    canvas->fillRoundRect(20, 20, 88, 88, 10, 0xFFFF);   // White face
    canvas->drawRoundRect(20, 20, 88, 88, 10, 0x0000);   // Black border
    canvas->drawRoundRect(21, 21, 86, 86, 10, 0x8410);   // Inner shadow

    // Draw dots
    for (int d = 0; d < DOT_COUNT[f]; d++)
    {
        int dx = cx + DOT_POS[f][d][0];
        int dy = cy + DOT_POS[f][d][1];
        drawDot(canvas, dx, dy);
    }
}

void DiceApp::loop()
{
    unsigned long now = millis();
    if (now - _lastUpdate < 33) return; // ~30 FPS
    _lastUpdate = now;

    // Check for shake to roll (or re-roll if result shown)
    if (_state == DICE_IDLE || _state == DICE_RESULT)
    {
        if (mpu.checkShake())
            startRoll();
    }

    if (_state == DICE_ROLLING)
        updateRoll();

    GFXcanvas16 *canvas = display.getBackCanvas();
    canvas->fillScreen(0x0019); // Dark blue background

    drawDiceFace(canvas, _currentFace);

    // Status text
    if (_state == DICE_IDLE)
    {
        canvas->setTextColor(0xFFFF);
        canvas->setTextSize(1);
        canvas->setCursor(22, 116);
        canvas->print("Shake to roll!");
    }
    else if (_state == DICE_RESULT)
    {
        canvas->setTextColor(0xFFE0); // Yellow
        canvas->setTextSize(1);
        canvas->setCursor(38, 116);
        canvas->print("Result: ");
        canvas->print(_result);
    }

    updateOverlay();
    if (isOverlayVisible())
        display.drawOverlay(false, display.getTimeString(), name(), 0, 0);

    display.swapAndRender();
}
