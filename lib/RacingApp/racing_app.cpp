#include "racing_app.h"
#include "display.h"
#include <Arduino.h>

RacingApp racingApp;

static const float STRIP_LEN  = 8.0f;   // world units per colour strip

RacingApp::RacingApp()
    : _state(RACING_PLAYING), _carX(64.0f), _roadCenterX(64.0f),
      _curvature(0), _targetCurvature(0), _curvatureChangeMs(0),
      _speed(55.0f), _distance(0), _score(0), _lastUpdate(0)
{}

void RacingApp::onEnter()
{
    Serial.println("[RacingApp] Enter");
    resetGame();
}

void RacingApp::onExit()
{
    Serial.println("[RacingApp] Exit");
}

void RacingApp::resetGame()
{
    _state             = RACING_PLAYING;
    _carX              = 64.0f;
    _roadCenterX       = 64.0f;
    _curvature         = 0;
    _targetCurvature   = 0;
    _curvatureChangeMs = millis(); // Delay first curve so game starts straight
    _speed             = 55.0f;
    _distance          = 0;
    _score             = 0;
    _lastUpdate        = millis();
}

void RacingApp::drawScene(GFXcanvas16 *canvas)
{
    // ── Sky gradient ──────────────────────────────────────
    for (int y = 0; y < HORIZON_Y; y++)
    {
        uint16_t c = (y < HORIZON_Y / 2) ? 0x0318 : 0x54FB;
        canvas->drawFastHLine(0, y, 128, c);
    }

    // ── Road (perspective scan-lines) ─────────────────────
    // For each screen row y from horizon to bottom:
    //   t  : 0 = far (horizon), 1 = near (camera)
    //   Z  : perspective depth (world units ahead of car)  = PERSP_K / (y - HORIZON_Y)
    //   halfW : road half-width at that depth
    //   centerX: road centre, shifted by curvature toward the horizon

    for (int y = HORIZON_Y + 1; y < 128; y++)
    {
        float t     = (float)(y - HORIZON_Y) / (float)PERSP_K; // 0..1
        float Z     = (float)PERSP_K / (float)(y - HORIZON_Y); // depth ahead

        float halfW   = ROAD_MIN_HW + t * (ROAD_MAX_HW - ROAD_MIN_HW);
        // Hyperbolic formula: curve effect is large near horizon (Z large),
        // nearly zero at the bottom (Z=1). This creates realistic perspective curvature.
        float centerX = _roadCenterX + _curvature * Z * 0.55f;

        int left  = (int)(centerX - halfW);
        int right = (int)(centerX + halfW);

        // Alternating colour strips (perspective-correct, scroll with distance)
        bool bright = (((int)((_distance + Z) / STRIP_LEN)) % 2 == 0);

        // Grass
        uint16_t gCol = bright ? 0x07E0 : 0x03E0;
        if (left > 0)   canvas->drawFastHLine(0,     y, left,       gCol);
        if (right < 128) canvas->drawFastHLine(right, y, 128 - right, gCol);

        // Road surface
        uint16_t rCol = bright ? 0x6B4D : 0x4A49;
        int rL = max(0, left);
        int rR = min(128, right);
        if (rR > rL) canvas->drawFastHLine(rL, y, rR - rL, rCol);

        // White edge lines
        for (int e = 0; e <= 1; e++)
        {
            int lx = left  + e;
            int rx = right - e;
            if (lx >= 0 && lx < 128) canvas->drawPixel(lx, y, 0xFFFF);
            if (rx >= 0 && rx < 128) canvas->drawPixel(rx, y, 0xFFFF);
        }

        // Yellow dashed centre line
        if (((int)((_distance + Z) / (STRIP_LEN * 0.6f))) % 2 == 0)
        {
            int cx = (int)centerX;
            if (cx >= 0 && cx < 128) canvas->drawPixel(cx, y, 0xFFE0);
        }
    }
}

void RacingApp::drawCar(GFXcanvas16 *canvas)
{
    int x = (int)_carX;
    int y = 128 - CAR_H - 2;

    canvas->fillRect(x - CAR_W / 2,     y,            CAR_W, CAR_H, 0x001F); // body
    canvas->fillRect(x - 3,             y + 2,         6,     4,     0x07FF); // windshield
    canvas->fillRect(x - CAR_W / 2,     y,             2,     4,     0x0000); // wheel FL
    canvas->fillRect(x + CAR_W / 2 - 2, y,             2,     4,     0x0000); // wheel FR
    canvas->fillRect(x - CAR_W / 2,     y + CAR_H - 4, 2,     4,     0x0000); // wheel RL
    canvas->fillRect(x + CAR_W / 2 - 2, y + CAR_H - 4, 2,     4,     0x0000); // wheel RR
}

void RacingApp::drawHUD(GFXcanvas16 *canvas)
{
    canvas->setTextColor(0xFFFF);
    canvas->setTextSize(1);
    canvas->setCursor(2, 2);
    canvas->print(_score);
}

void RacingApp::drawGameOver(GFXcanvas16 *canvas)
{
    canvas->fillScreen(0x0000);
    canvas->setTextColor(0xF800);
    canvas->setTextSize(2);
    canvas->setCursor(10, 28);
    canvas->print("GAME");
    canvas->setCursor(10, 48);
    canvas->print("OVER");
    canvas->setTextColor(0xFFFF);
    canvas->setTextSize(1);
    canvas->setCursor(10, 78);
    canvas->print("Score: ");
    canvas->print(_score);
    canvas->setCursor(5, 98);
    canvas->print("Shake to retry");
}

void RacingApp::loop()
{
    unsigned long now = millis();
    float dt = (now - _lastUpdate) / 1000.0f;
    if (dt < 0.033f) return;
    _lastUpdate = now;

    GFXcanvas16 *canvas = display.getBackCanvas();

    if (_state == RACING_GAMEOVER)
    {
        drawGameOver(canvas);
        if (mpu.checkShake()) resetGame();
    }
    else
    {
        // ── Update curvature ──────────────────────────────
        if (now - _curvatureChangeMs > 1800)
        {
            _curvatureChangeMs = now;
            _targetCurvature = (random(201) - 100) / 100.0f; // -1.0 .. +1.0
        }
        _curvature += (_targetCurvature - _curvature) * 4.0f * dt;

        // Road centre drifts with curvature; gently pulls back to 64
        _roadCenterX += _curvature * _speed * 0.25f * dt;
        _roadCenterX += (64.0f - _roadCenterX) * 0.012f;
        if (_roadCenterX < 28)  { _roadCenterX = 28;  _targetCurvature =  0.6f; }
        if (_roadCenterX > 100) { _roadCenterX = 100; _targetCurvature = -0.6f; }

        // ── Steer car with tilt ───────────────────────────
        float tilt = mpu.getTiltPosition();
        _carX += tilt * _speed * 0.55f * dt;
        if (_carX <   4) _carX =   4;
        if (_carX > 124) _carX = 124;

        // ── Distance & score ─────────────────────────────
        _distance += _speed * dt;
        _score = (int)(_distance / 10.0f);

        // ── Speed up gradually ────────────────────────────
        _speed += 3.0f * dt;
        if (_speed > 200.0f) _speed = 200.0f;

        // ── Collision: car off road at bottom row ─────────
        float halfRoad = (float)ROAD_MAX_HW - (float)CAR_W / 2.0f;
        if (_carX < _roadCenterX - halfRoad || _carX > _roadCenterX + halfRoad)
        {
            _state = RACING_GAMEOVER;
            Serial.printf("[RacingApp] Off road! Score: %d\n", _score);
        }

        drawScene(canvas);
        drawCar(canvas);
        drawHUD(canvas);
    }

    updateOverlay();
    if (isOverlayVisible())
        display.drawOverlay(false, display.getTimeString(), name(), 0, 0);

    display.swapAndRender();
}
