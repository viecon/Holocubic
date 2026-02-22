#include "mpu.h"
#include <Wire.h>

MPU mpu;

MPU::MPU()
    : _offAx(0), _offAy(0), _offAz(0), _lastTilt(TILT_NEUTRAL), _lastPitch(PITCH_NEUTRAL),
      _lastSwitchMs(0), _lastPitchMs(0), _lastShakeMs(0), _smoothedTilt(0)
{
}

void MPU::begin()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_FREQUENCY);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(REG_PWR_MGMT_1);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(50);

    Serial.println("[MPU] Initialized");
}

void MPU::calibrate(int samples)
{
    Serial.printf("[MPU] Calibrating with %d samples...\n", samples);

    float sumx = 0, sumy = 0, sumz = 0;
    for (int i = 0; i < samples; i++)
    {
        float ax, ay, az;
        readAccel(ax, ay, az);
        sumx += ax;
        sumy += ay;
        sumz += az;
        delay(2);
    }

    _offAx = sumx / samples;
    _offAy = sumy / samples;
    _offAz = sumz / samples - 1.0f;

    Serial.printf("[MPU] Calibration done. Offsets: %.3f, %.3f, %.3f\n",
                  _offAx, _offAy, _offAz);
}

void MPU::readAccel(float &ax, float &ay, float &az)
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(REG_ACCEL_XOUT);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6, (uint8_t)1);

    int16_t raw_ax = (Wire.read() << 8) | Wire.read();
    int16_t raw_ay = (Wire.read() << 8) | Wire.read();
    int16_t raw_az = (Wire.read() << 8) | Wire.read();

    ax = (float)raw_ax / 16384.0f - _offAx;
    ay = (float)raw_ay / 16384.0f - _offAy;
    az = (float)raw_az / 16384.0f - _offAz;
}

float MPU::readRollDeg()
{
    float ax, ay, az;
    readAccel(ax, ay, az);
    return atan2f(ay, az) * 180.0f / PI;
}

float MPU::readPitchDeg()
{
    float ax, ay, az;
    readAccel(ax, ay, az);
    return atan2f(ax, az) * 180.0f / PI;
}

int MPU::checkTiltChange()
{
    float roll = readRollDeg();
    unsigned long now = millis();

    if (_lastTilt == TILT_NEUTRAL)
    {
        if (roll >= ENTER_TILT_DEG && (now - _lastSwitchMs >= SWITCH_COOLDOWN_MS))
        {
            _lastSwitchMs = now;
            _lastTilt = TILT_RIGHT;
            return +1;
        }
        else if (roll <= -ENTER_TILT_DEG && (now - _lastSwitchMs >= SWITCH_COOLDOWN_MS))
        {
            _lastSwitchMs = now;
            _lastTilt = TILT_LEFT;
            return -1;
        }
    }
    else if (_lastTilt == TILT_RIGHT)
    {
        if (roll < EXIT_TILT_DEG)
        {
            _lastTilt = TILT_NEUTRAL;
        }
    }
    else if (_lastTilt == TILT_LEFT)
    {
        if (roll > -EXIT_TILT_DEG)
        {
            _lastTilt = TILT_NEUTRAL;
        }
    }

    return 0;
}

int MPU::checkPitchChange()
{
    float pitch = readPitchDeg();
    unsigned long now = millis();

    if (_lastPitch == PITCH_NEUTRAL)
    {
        if (pitch >= ENTER_TILT_DEG && (now - _lastPitchMs >= SWITCH_COOLDOWN_MS))
        {
            _lastPitchMs = now;
            _lastPitch = PITCH_FORWARD;
            return +1;
        }
        else if (pitch <= -ENTER_TILT_DEG && (now - _lastPitchMs >= SWITCH_COOLDOWN_MS))
        {
            _lastPitchMs = now;
            _lastPitch = PITCH_BACKWARD;
            return -1;
        }
    }
    else if (_lastPitch == PITCH_FORWARD)
    {
        if (pitch < EXIT_TILT_DEG)
            _lastPitch = PITCH_NEUTRAL;
    }
    else if (_lastPitch == PITCH_BACKWARD)
    {
        if (pitch > -EXIT_TILT_DEG)
            _lastPitch = PITCH_NEUTRAL;
    }

    return 0;
}

float MPU::getAccelMagnitude()
{
    float ax, ay, az;
    readAccel(ax, ay, az);
    return sqrtf(ax * ax + ay * ay + az * az);
}

bool MPU::checkShake()
{
    unsigned long now = millis();

    // Cooldown period after last shake detection
    if (now - _lastShakeMs < 500)
    {
        return false;
    }

    float magnitude = getAccelMagnitude();

    // Shake threshold: detect sudden acceleration above 2.5g
    // Normal gravity is ~1.0g, so shake will spike significantly higher
    if (magnitude > 2.5f)
    {
        _lastShakeMs = now;
        Serial.printf("[MPU] Shake detected! Magnitude: %.2f\n", magnitude);
        return true;
    }

    return false;
}

float MPU::getTiltPosition()
{
    float roll = readPitchDeg();

    // Map roll angle to normalized position (-1.0 to +1.0)
    // Use wider range for better sensitivity: -45° to +45°
    float position = roll / 45.0f;

    // Clamp to valid range
    if (position < -1.0f)
        position = -1.0f;
    if (position > 1.0f)
        position = 1.0f;

    // Apply smoothing with exponential moving average (alpha = 0.3)
    _smoothedTilt = 0.3f * position + 0.7f * _smoothedTilt;

    // Apply dead zone around center to prevent drift
    if (fabsf(_smoothedTilt) < 0.1f)
    {
        _smoothedTilt = 0.0f;
    }

    return _smoothedTilt;
}
