#pragma once
#include <Wire.h>

#define SHT30_I2C_ADDR (0x44)

class SHT30
{
    private:
        TwoWire *wire;
        uint16_t addr;
        uint32_t frequency;
        uint32_t previousFrequency;
        void SetClock(void);
        void RestoreClock(void);
    public:
        SHT30(TwoWire &wire, uint16_t addr = SHT30_I2C_ADDR, uint32_t frequency = 0);
        bool Update(void);
        void Read(float &temp, float &humidity);
};
