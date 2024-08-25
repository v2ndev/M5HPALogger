#include <M5StickCPlus.h>
#include <WiFi.h>   

#define SOFTAP_SSID "M5HPALogger"
#define SOFTAP_PASSPHRASE "M5HPALogger0812"

WiFiUDP udpSock;

void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.println("M5 Initialized.");

    Wire.begin(0, 26);
    Wire.beginTransmission(0x28);
    auto err = Wire.endTransmission();
    M5.Lcd.printf("i2c %d\n", err);

    WiFi.begin(SOFTAP_SSID, SOFTAP_PASSPHRASE);
}

typedef struct {
    uint16_t raw;
    float mps;
} convert_table_t;

convert_table_t convert_table[] = {
    {409, 0.0f}, 
    {1203, 2.0f},
    {1597, 3.0f},
    {1908, 4.0f},
    {2187, 5.0f},
    {2400, 6.0f},
    {2629, 7.0f},
    {2801, 8.0f},
    {3006, 9.0f},
    {3178, 10.0f},
    {3309, 11.0f},
    {3563, 13.0f},
    {3686, 15.0f}
};

float getSpeed(uint16_t rawValue)
{
    const size_t last = sizeof(convert_table) / sizeof(convert_table_t) - 1;
    if (rawValue <= convert_table[0].raw) {
        return convert_table[0].mps;
    }
    if (rawValue >= convert_table[last].raw) {
        return convert_table[last].mps;
    }
    for (int i = 0; i < last; i++) {
        if (convert_table[i].raw <= rawValue & rawValue < convert_table[i + 1].raw) {
            return convert_table[i].mps + (convert_table[i + 1].mps - convert_table[i].mps) * (rawValue - convert_table[i].raw) / (convert_table[i + 1].raw - convert_table[i].raw);
        }
    }
    return 0.0f;
}

void loop()
{
    M5.Lcd.setCursor(0, 32);
    M5.Lcd.setTextSize(1);
    
    M5.Lcd.printf("battery: %5.3f V\n", M5.Axp.GetBatVoltage());

    M5.Lcd.print("IP Address: ");
    M5.Lcd.println(WiFi.localIP());
    
    auto len = Wire.requestFrom(0x28, 7);
    M5.Lcd.printf("received len = %d\n", len);

    uint8_t data[7] = {};
    uint8_t sum = 0;
    for (int i = 0; i < len && i < 7; i++) {
        data[i] = Wire.read();
        sum += data[i];
    }

    if (sum == 0) {
        M5.Lcd.printf("checksum ok\n");
    } else {
        M5.Lcd.printf("checksum NG\n");
    }

    M5.Lcd.setTextSize(4);
    uint16_t value = (((uint16_t) (data[1] & 0x0f)) << 8) + data[2];
    M5.Lcd.printf("raw: %5d\n", value);
    M5.Lcd.printf("mps: %5.2f\n", getSpeed(value));

    if (WiFi.isConnected()) {
        if (udpSock.begin(10000)) {
            float airSpeed = getSpeed(value);
            udpSock.beginPacket(WiFi.gatewayIP(), 10000);
            udpSock.write(reinterpret_cast<uint8_t*>(&airSpeed), sizeof(airSpeed));
            udpSock.endPacket();
        }
    }

    delay(100);
}
