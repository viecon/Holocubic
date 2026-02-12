#include "mpu.h"
#include <Wire.h>

MPU mpu;

MPU::MPU()
    : _offAx(0), _offAy(0), _offAz(0), _lastTilt(TILT_NEUTRAL), _lastPitch(PITCH_NEUTRAL),
      _lastSwitchMs(0), _lastPitchMs(0)
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
