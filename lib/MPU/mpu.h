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

class MPU
{
public:
    MPU();

    void begin();
    void calibrate(int samples = 300);
    int checkTiltChange();

private:
    float _offAx, _offAy, _offAz;
    TiltState _lastTilt;
    unsigned long _lastSwitchMs;

    void readAccel(float &ax, float &ay, float &az);
    float readRollDeg();
};

extern MPU mpu;

#endif // MPU_H
