#pragma once
#include <stdint.h>

class HallSensor
{
    private:
        volatile uint32_t lastMillis;
        volatile uint32_t duration;
        static void interruptHandler(void *p);
        void interruptHandler(void);
    public:
        void begin(const uint8_t gpio, const int mode);
        float getFrequency();
};
