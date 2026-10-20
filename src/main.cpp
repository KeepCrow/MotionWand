#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <Wire.h>

#define SAMPLE_INTERVAL_US (10000)
#define THRESHOLD_GZ_BUTTONA (3.0f)
#define ACCEL_MAX_G (9.8f)

typedef struct
{
    int16_t stickX;
    int16_t stickY;
    bool buttonA;
} GamepadIntent;

Adafruit_MPU6050 mpu;
GamepadIntent intent = {0};
uint32_t next_timestamp_us;
uint32_t cur_timestamp_us;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Wire.begin(SDA, SCL);
    if (!mpu.begin())
    {
        Serial.println("Cannot find MPU6050, please check the wire!");
        while (true)
        {
            delay(10);
        }
    }

    next_timestamp_us = micros();
}

int16_t scaleToStickAxis(float accel)
{
    float normalized = accel / ACCEL_MAX_G;
    int32_t raw = (int32_t)(normalized * 32767.0f);
    return (int16_t)constrain(raw, -32767, 32767);
}

void updateIntent(const sensors_event_t &a, const sensors_event_t &g, GamepadIntent &out)
{
    out.stickX = scaleToStickAxis(a.acceleration.x);
    out.stickY = scaleToStickAxis(a.acceleration.y);
    out.buttonA = g.gyro.z >= THRESHOLD_GZ_BUTTONA;
}

void loop()
{
    cur_timestamp_us = micros();

    if ((int32_t)(cur_timestamp_us - next_timestamp_us) >= 0)
    {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        updateIntent(a, g, intent);
        Serial.printf(">stick_x:%d,stick_y:%d,btn_a:%d\n", intent.stickX, intent.stickY, intent.buttonA ? 1000 : 0);

        while ((int32_t)(cur_timestamp_us - next_timestamp_us) >= 0)
        {
            next_timestamp_us += SAMPLE_INTERVAL_US;
        }
    }
}
