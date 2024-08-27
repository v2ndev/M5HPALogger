#include <Arduino.h>
#include "HallSensor.h"

void HallSensor::interruptHandler(void *p)
{
    reinterpret_cast<HallSensor*>(p)->interruptHandler();
}

void HallSensor::interruptHandler(void)
{
    uint32_t currentMillis = millis();
    uint32_t duration = currentMillis - lastMillis;

    if (duration >= 100) {
        this->duration = duration;
        lastMillis = currentMillis;
    }
}

void HallSensor::begin(const uint8_t gpio, const int mode)
{
    lastMillis = 0;
    duration = 0;
    pinMode(gpio, INPUT);
    attachInterruptArg(gpio, HallSensor::interruptHandler, this, mode);
}

float HallSensor::getFrequency(void)
{
    uint32_t currentMillis = millis();
    uint32_t duration = currentMillis - lastMillis;
    float hallHz = this->duration > 0 ? (1000.0f / this->duration) : 0.0f;
    return duration < 2000 ? hallHz : 0.0f;
}
