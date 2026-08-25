#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <Wire.h>


#define LED_PIN 48

// 阈值，需调试
const float FREEEFALL_TH = 4.0;
const float IMPACT_TH = 25.0;
const float STILL_TH = 1.5;
const float STAND_TH = 5.0;

// 时间窗
const unsigned long FALL_WINDOW = 600;
const unsigned long IMPACT_WINDOW = 3000;
const unsigned long STILL_NEEDED = 1500;
unsigned long tFreefall, tImpact, tStill;

enum State { NORMAL, FREEFALL, IMPACTED, FALLEN };
enum LED_COLOR { GREEN, BLUE, YELLOW, RED };

State state = NORMAL;
Adafruit_MPU6050 mpu;
Adafruit_Sensor *acc;
sensors_event_t a;
float ax, ay, az;
float gravity, mag, motion;
unsigned long now;


void setLED(LED_COLOR color) {
    switch (color)
    {
    case GREEN:
        neopixelWrite(LED_PIN, 0, 8, 0);
        break;
    case BLUE:
        neopixelWrite(LED_PIN, 0, 0, 32);
        break;
    case YELLOW:
        neopixelWrite(LED_PIN, 16, 12, 0);
        break;
    case RED:
        neopixelWrite(LED_PIN, 32, 0, 0);
        break;
    default:
        break;
    }
}


float readMag() {
    acc = mpu.getAccelerometerSensor();
    acc->getEvent(&a);
    ax = a.acceleration.x, ay = a.acceleration.y, az = a.acceleration.z;
    return sqrt(ax * ax + ay * ay + az * az);
}


void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin(SDA, SCL);
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU60550, please check the wire!");
        while (true) { delay(1000); }
    }

    float sum = 0;
    for (int i = 0; i < 200; i++) {
        sum += readMag();
    }
    gravity = sum / 200;
    Serial.print("The base gravity="); Serial.println(gravity, 2);
    setLED(GREEN);
}

State expect_freefall(float motion, unsigned long now) {
    if (mag < FREEEFALL_TH) {
        tFreefall = now;
        setLED(BLUE);
        Serial.println("Warn: Freefall detected.");

        return FREEFALL;
    }

    return state;
}

State expect_impact(float motion, unsigned long now) {
    if (mag > IMPACT_TH) {
        Serial.println("Warn: Impact detected after freefall.");
        tImpact = now;
        setLED(YELLOW);
        return IMPACTED;
    }

    if (now - tFreefall > FALL_WINDOW) {
        Serial.println("Info: No impact after freefall for 600ms, warn canceled.");
        setLED(GREEN);
        return NORMAL;
    }

    return state;
}

State expect_still(float motion, unsigned long now) {
    // 且相对向量低于静止阈值一段时间：修改当前状态为摔倒态，亮红灯，输出告警
    // 且相对向量高于静止阈值一段时间：修改当前状态为正常态，亮绿灯，输出提示
    if (motion > STILL_TH) {
        tStill = now;
        if (now - tImpact > IMPACT_WINDOW) {
            Serial.println("Info: No still after impact for 3000ms, warn canceled.");
            setLED(GREEN);
            return NORMAL;
        }
    } else {
        if (now - tStill > STILL_NEEDED) {
            Serial.println("Warn: Detected still for 1500ms after impact, FALLEN!!!");
            setLED(RED);
            return FALLEN;
        }
    }

    return state;
}

State expect_stand_up(float motion, unsigned long now) {
    if (motion > STAND_TH) {
        Serial.println("Info: detected moving, warn canceled");
        setLED(GREEN);
        return NORMAL;
    }

    return state;
}

void print_state(State state) {
    switch (state)
    {
    case NORMAL:
        Serial.print("[NORMAL]");
        break;
    case FREEFALL:
        Serial.print("[FREEFALL]");
        break;
    case IMPACTED:
        Serial.print("[IMPACTED]");
        break;
    case FALLEN:
        Serial.print("[FALLED]");
        break;
    default:
        break;
    }
}

void loop() {
    mag = readMag();
    motion = fabs(mag - gravity);
    now = millis();

    print_state(state);
    Serial.print(": ");
    Serial.println(mag);
    switch (state)
    {
    case NORMAL:
        state = expect_freefall(motion, now);
        break;
    case FREEFALL:
        state = expect_impact(motion, now);
        break;
    case IMPACTED:
        state = expect_still(motion, now);
        break;
    case FALLEN:
        state = expect_stand_up(motion, now);
        break;
    
    default:
        break;
    }
    
    delay(20);
}
