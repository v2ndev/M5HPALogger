#pragma once
#include <Wire.h>

#define ULTRASONIC_I2C_ADDR (0x57)

class Ultrasonic
{
    private:
        TwoWire *wire;
        uint16_t addr;
        uint32_t frequency;
        uint32_t previousFrequency;
        void SetClock(void);
        void RestoreClock(void);
    public:
        Ultrasonic(TwoWire &wire, uint16_t addr = ULTRASONIC_I2C_ADDR, uint32_t frequency = 0);
        bool Ping(void);
        uint32_t Read(void);
};
