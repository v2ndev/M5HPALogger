#include <mutex>
#include <Arduino.h>
#include <M5UnitENV.h>
#include <M5Module_GNSS.h>
#include <SoftwareSerial.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <driver/adc.h>
#include "BinLog.h"
#include "Ultrasonic.h"
#include "SHT30.h"
#include "HallSensor.h"

#define SOFTAP_SSID "M5HPALogger"
#define SOFTAP_PASSPHRASE "M5HPALogger0812"
#define SOFTAP_IPADDR IPAddress(192, 168, 254, 1)
#define SOFTAP_GATEWAY IPAddress(192, 168, 254, 1)
#define SOFTAP_SUBNETMASK IPAddress(255, 255, 255, 0)
#define UDP_PORT (10000)

typedef struct
{
    double latitude;
    double longitude;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t status;
} GNSSData;

constexpr uint8_t BMI270_ADDR = 0x69;

BMI270::BMI270 bmi270;
BMP280 bmp280;
SHT30 sht30(Wire, SHT30_I2C_ADDR, 100000);
SoftwareSerial GNSSSerial(13, 15);
BinLog logger;
Ultrasonic ultrasonic(Wire, ULTRASONIC_I2C_ADDR, 50000);
WiFiUDP udpSock;
GNSSData gnssData;
std::mutex gnssMutex;
HallSensor cadenceSensor;

uint32_t nextTime;
uint8_t uiMode = 0;

void setup()
{
    M5.begin();
    M5.Power.begin();
    // adc_power_acquire();
    M5.Lcd.fillScreen(BLACK);

    M5.Lcd.println("Initializing WiFi softAP.");
    if (!WiFi.softAPConfig(SOFTAP_IPADDR, SOFTAP_GATEWAY, SOFTAP_SUBNETMASK))
    {
        M5.Lcd.println("soft ap config error.");
    }
    if (!WiFi.softAP(SOFTAP_SSID, SOFTAP_PASSPHRASE)) {
        M5.Lcd.println("soft ap start error.");
    }
    if (!udpSock.begin(UDP_PORT)) {
        M5.Lcd.println("udp socket initialize error.");
    }

    M5.Lcd.println("Initializing BinLog.");
    logger.OpenFileSequential("/log%05d.bin", 99999);

    M5.Lcd.println("Initializing Atmosphere Pressure sensor.");
    bmp280.begin(&Wire, BMP280_I2C_ADDR, 21, 22, 100000);

    M5.Lcd.println("Initializing IMU.");
    bmi270.init(I2C_NUM_0, BMI270_ADDR);

    M5.Lcd.println("Initializing Serial for GNSS.");
    GNSSSerial.begin(38400);
    GNSSSerial.setTimeout(10);
    xTaskCreateUniversal(gnssTask, "gnssTask", 4096, NULL, 1, NULL, 0);

    M5.Lcd.println("Initializing Hall sensor.");
    cadenceSensor.begin(36, FALLING);

    M5.Lcd.println("Initializing Environment sensor.");
    sht30.Update();

    M5.Lcd.println("Sending ping...");
    ultrasonic.Ping();
    delay(1000);

    nextTime = millis() + 200;
}

float airSpeed = 0.0f;

void loop()
{
    while(millis() < nextTime) {delay(1);}
    nextTime += 200;

    int8_t batteryLevel = M5.Power.getBatteryLevel();

    float altitude = (float) ultrasonic.Read() / 1000;

    float gX, gY, gZ, aX, aY, aZ;
    int16_t mX, mY, mZ;
    if (bmi270.accelerationAvailable()) {bmi270.readAcceleration(aX, aY, aZ);}
    if (bmi270.gyroscopeAvailable()) {bmi270.readGyroscope(gX, gY, gZ);}
    if (bmi270.magneticFieldAvailable()) {bmi270.readMagneticField(mX, mY, mZ);}

    float bmpTemp, bmpPressure;
    bmpTemp = bmp280.readTemperature();
    bmpPressure = bmp280.readPressure();

    float cTemp, humidity;
    sht30.Read(cTemp, humidity);

    float cadence = cadenceSensor.getFrequency() * 60;

    // update sensor
    sht30.Update();
    ultrasonic.Ping();

    // parse packet
    while (int len = udpSock.parsePacket()) {
        char buf[len];
        udpSock.read(buf, len);
        if (len == 4) {
            airSpeed = *(reinterpret_cast<float*>(buf));
        }
    }

    GNSSData gnss;
    {
        std::lock_guard<std::mutex> lock(gnssMutex);
        gnss = gnssData;
    }
    unsigned long gnssTime = (((unsigned long) gnssData.hour * 60 + gnssData.min) * 60 + gnssData.sec) * 1000;
    
    // write log
    float log0[6] = {aX, aY, aZ, gX, gY, gZ};
    logger.WriteLog(0, 0, log0, sizeof(log0));
    int16_t log1[4] = {mX, mY, mZ, batteryLevel};
    logger.WriteLog(0, 1, log1, sizeof(log1));
    float log2[7] = {cTemp, humidity, bmpTemp, bmpPressure, altitude, airSpeed, cadence};
    logger.WriteLog(0, 2, log2, sizeof(log2));
    logger.WriteLog(0, 3, &gnss, sizeof(gnss));
    logger.Flush();

    // check button
    M5.update();
    if (M5.BtnA.wasPressed()) {uiMode = 0; M5.Lcd.clear();}
    if (M5.BtnB.wasPressed()) {uiMode = 1; M5.Lcd.clear();}

    // update ui
    if (uiMode == 0) {
        M5.Lcd.setTextSize(1);
        printTimeMills(0, "boottime", millis());
        printMeter(1, "battery", (int)batteryLevel, 0, 100);
        printMeter(2, "altitude", (int) altitude, 0, 4500);
        printMeter(3, "accelX", (int)(aX * 1000), -2000, +2000);
        printMeter(4, "accelY", (int)(aY * 1000), -2000, +2000);
        printMeter(5, "accelZ", (int)(aZ * 1000), -2000, +2000);
        printMeter(6, "gyroX", (int)(gX), -200, +200);
        printMeter(7, "gyroY", (int)(gY), -200, +200);
        printMeter(8, "gyroZ", (int)(gZ), -200, +200);
        printMeter(9, "magX", (mX), -200, +200);
        printMeter(10, "magY", (mY), -200, +200);
        printMeter(11, "magZ", (mZ), -200, +200);
        printMeter(12, "temp(IMU)", (int) bmpTemp, 0, 80);
        printMeter(13, "pressure", (int) bmpPressure, 900, 1100);
        printMeter(14, "temp", (int) cTemp, 0, 80);
        printMeter(15, "humidity", (int) humidity, 0, 100);
        printMeter(16, "cadence", (int) cadence, 0, 150);
        printMeterFloat(17, "airSpeed", airSpeed, 0.0f, 15.0f);

        printTimeMills(18, "gnss time", gnssTime);
        printDouble(19, "lat", gnss.latitude);
        printDouble(20, "long", gnss.longitude);
    } else if (uiMode == 1) {
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(4);
        M5.Lcd.printf("Spd: %5.1f\n\n", airSpeed);
        M5.Lcd.printf("Alt: %5.1f\n\n", altitude / 1000.f);
        M5.Lcd.printf("Cad: %3d\n", cadence);
    }
}

void gnssTask(void *pvParameters)
{
    char buf[128] = {};
    size_t ptr = 0;
    while (true) {
        int c = GNSSSerial.read();
        if (c == -1) {delay(1); continue;}
        if (ptr == 0 && c != '$') {continue;}
        if (c == '\r') {continue;}
        if (c != '\n') {buf[ptr++] = c; continue;}
        buf[ptr] = 0;
        // Serial.printf("GNSS: %s\n", buf);
        gnssParse(buf);

        ptr = 0;
    }
}

void gnssParse(char *nmea)
{
    size_t len = strlen(nmea);
    char *token[20];
    constexpr size_t maxToken = sizeof(token) / sizeof(char *);

    if (len < 11) {return;}
    if (nmea[len - 3] != '*') {return;}
    nmea[len - 3] = '\0';

    uint8_t checksum;
    if (sscanf(nmea + len - 2, "%hhx", &checksum) != 1) {return;}
    
    uint8_t sum = 0;
    for (int i = 1; i < len - 3; i++) {sum ^= nmea[i];}

    if (sum != checksum) {
        Serial.printf("checksum mismatch! (%02x, %02x)\n", checksum, sum);
        return;
    }
    
    token[0] = nmea;
    size_t numToken = 1;
    for (; numToken < maxToken; numToken++) {
        char *comma = strchr(token[numToken - 1], ',');
        if (comma == NULL) {break;}
        *comma = '\0';
        token[numToken] = comma + 1;
    }

    if (strcmp(token[0], "$GNRMC") == 0 && numToken == 14) {
        int date = atoi(token[9]);
        int year = 2000 + date % 100;
        int month = date / 100 % 100;
        int day = date / 10000;
        int time = (int)atof(token[1]);
        int sec = time % 100;
        int minute = time / 100 % 100;
        int hour = time / 10000;
        Serial.printf("GNSS date: %04d/%02d/%02d %02d:%02d:%02d\n", year, month, day, hour, minute, sec);

        std::lock_guard<std::mutex> lock(gnssMutex);
        gnssData.year = year;
        gnssData.month = month;
        gnssData.day = day;
        gnssData.hour = hour;
        gnssData.min = minute;
        gnssData.sec = sec;
        gnssData.status = (uint8_t) token[2][0];

        if (strcmp(token[2], "A") == 0) {
            // 測位有効
            bool isNorthLat = strcmp(token[4], "N") == 0;
            bool isEastLong = strcmp(token[6], "E") == 0;
            double latitude = (isNorthLat ? 1 : -1) * gnssConvert(atof(token[3]));
            double longitude = (isEastLong ? 1 : -1) * gnssConvert(atof(token[5]));

            gnssData.latitude = latitude;
            gnssData.longitude = longitude;

            Serial.printf("GNSS pos: %lf,%lf\n", latitude, longitude);
        }
    }
}

float gnssConvert(float value)
{
    int a = value / 100;
    float b = (value / 100 - a) / 60 * 100;
    return a + b;
}

const int textHeight = 8;
const int barWidth = 200;

void printTimeMills(int line, char *name, unsigned long millis)
{
    int y = textHeight * line;

    unsigned long secs = millis / 1000;
    unsigned long mins = secs / 60;
    unsigned long hours = mins / 60;

    M5.Lcd.fillRect(0, y, 320, textHeight, BLACK);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf(name);
    M5.Lcd.setCursor(100, y);
    M5.Lcd.printf("%02ld:%02ld:%02ld.%03ld", hours, mins % 60, secs % 60, millis % 1000);
}

void printMeter(int line, char *name, int value, int minValue, int maxValue)
{
    int y = textHeight * line;
    int zeroPos = (int)((long)-minValue * barWidth / (maxValue - minValue));
    int currentPos = (int)((long)(value - minValue) * barWidth / (maxValue - minValue));

    zeroPos = min(max(zeroPos, 0), barWidth);
    currentPos = min(max(currentPos, 0), barWidth);

    M5.Lcd.fillRect(0, y, 320, textHeight, BLACK);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf(name);
    M5.Lcd.setCursor(50, y);
    M5.Lcd.printf("%7d", value);

    if (zeroPos < currentPos)
    {
        M5.Lcd.fillRect(100 + zeroPos, y, max(currentPos - zeroPos, 1), textHeight - 1, WHITE);
    }
    else
    {
        M5.Lcd.fillRect(100 + currentPos, y, max(zeroPos - currentPos, 1), textHeight - 1, WHITE);
    }
}

void printMeterFloat(int line, char *name, float value, float minValue, float maxValue)
{
    int y = textHeight * line;
    int zeroPos = (int)(-minValue * barWidth / (maxValue - minValue));
    int currentPos = (int)((value - minValue) * barWidth / (maxValue - minValue));

    zeroPos = min(max(zeroPos, 0), barWidth);
    currentPos = min(max(currentPos, 0), barWidth);

    M5.Lcd.fillRect(0, y, 320, textHeight, BLACK);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf(name);
    M5.Lcd.setCursor(50, y);
    M5.Lcd.printf("%7.2f", value);

    if (zeroPos < currentPos)
    {
        M5.Lcd.fillRect(100 + zeroPos, y, max(currentPos - zeroPos, 1), textHeight - 1, WHITE);
    }
    else
    {
        M5.Lcd.fillRect(100 + currentPos, y, max(zeroPos - currentPos, 1), textHeight - 1, WHITE);
    }
}

void printDouble(int line, char *name, double value)
{
    int y = textHeight * line;

    M5.Lcd.fillRect(0, y, 320, textHeight, BLACK);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf(name);
    M5.Lcd.setCursor(100, y);
    M5.Lcd.printf("%12.7lf", value);
}
