#include "Ultrasonic.h"

Ultrasonic::Ultrasonic(TwoWire &wire, uint16_t addr, uint32_t frequency)
{
    this->wire = &wire;
    this->addr = addr;
    this->frequency = frequency;
    previousFrequency = 0;
}

void Ultrasonic::SetClock(void)
{
    if (frequency > 0) {
        previousFrequency = wire->getClock();
        wire->setClock(frequency);
    }
}

void Ultrasonic::RestoreClock(void)
{
    if (previousFrequency > 0) {
        wire->setClock(previousFrequency);
        previousFrequency = 0;
    }
}

bool Ultrasonic::Ping(void)
{
    bool success = true;

    SetClock();
    wire->beginTransmission(this->addr);
    success &= wire->write(0x01) == 1;
    success &= wire->endTransmission() == 0;
    RestoreClock();

    return success;
}

uint32_t Ultrasonic::Read(void)
{
    uint32_t alt = 0;

    SetClock();
    if (wire->requestFrom(addr, (size_t) 3) == 3) {
        alt = wire->read();
        alt = (alt << 8) | wire->read();
        alt = (alt << 8) | wire->read();
    }
    RestoreClock();

    return alt;
}
