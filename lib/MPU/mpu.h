#ifndef MPU_H
#define MPU_H

#include <Arduino.h>
#include "config.h"

enum TiltState
{
    TILT_NEUTRAL,
    TILT_LEFT,
    TILT_RIGHT
};

enum PitchState
{
    PITCH_NEUTRAL,
    PITCH_FORWARD,
    PITCH_BACKWARD
};

class MPU
{
public:
    MPU();

    void begin();
    void calibrate(int samples = 300);
    int checkTiltChange();
    int checkPitchChange();
    bool checkShake();
    float getTiltPosition();
    float getAccelMagnitude();

private:
    float _offAx, _offAy, _offAz;
    TiltState _lastTilt;
    PitchState _lastPitch;
    unsigned long _lastSwitchMs;
    unsigned long _lastPitchMs;
    unsigned long _lastShakeMs;
    float _smoothedTilt;

    void readAccel(float &ax, float &ay, float &az);
    float readRollDeg();
    float readPitchDeg();
};

extern MPU mpu;

#endif // MPU_H
