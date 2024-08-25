#include "SHT30.h"

SHT30::SHT30(TwoWire &wire, uint16_t addr, uint32_t frequency)
{
    this->wire = &wire;
    this->addr = addr;
    this->frequency = frequency;
    previousFrequency = 0;
}

void SHT30::SetClock(void)
{
    if (frequency > 0) {
        previousFrequency = wire->getClock();
        wire->setClock(frequency);
    }
}

void SHT30::RestoreClock(void)
{
    if (previousFrequency > 0) {
        wire->setClock(previousFrequency);
        previousFrequency = 0;
    }
}

bool SHT30::Update(void)
{
    bool success = true;

    SetClock();
    wire->beginTransmission(this->addr);
    success &= wire->write(0x2c) == 1;
    success &= wire->write(0x06) == 1;
    success &= wire->endTransmission() == 0;
    RestoreClock();

    return success;
}

void SHT30::Read(float &temp, float &humidity)
{
    uint32_t alt = 0;

    SetClock();
    if (wire->requestFrom(addr, (size_t) 6) == 6) {
        uint8_t data[6];
        if (wire->readBytes(data, 6) == 6) {
            temp = (((float) (data[0] << 8 | data[1]) * 175) / 65535) - 45;
            humidity = (((float) (data[3] << 8 | data[4]) * 100) / 65535);
        }
    }
    RestoreClock();

    return;
}
