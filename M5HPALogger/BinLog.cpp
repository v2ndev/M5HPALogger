#include <M5Stack.h>
#include "BinLog.h"

bool BinLog::OpenFileSequential(const char *format, const int max)
{
    int num = 0;

    do {
        snprintf(fname, sizeof(fname), format, num);
    } while (SD.exists(fname) && num++ < max);

    M5.Lcd.printf("File: %s\n", fname);
    fh = SD.open(fname, FILE_APPEND, true);
    if (!fh) {
        M5.Lcd.printf("File open ERROR\n");
        return false;
    }

    return true;
}

void BinLog::WriteLog(const uint8_t deviceId, const uint8_t pageId, const uint8_t *data, const size_t len)
{
    BinLogRecord log = {};

    log.header = '@';
    log.deviceId = deviceId;
    log.pageId = pageId;
    log.time_ms = millis();
    memcpy(log.data, data, min(len, sizeof(log.data)));
    fh.write(reinterpret_cast<uint8_t *>(&log), sizeof(log));
}

void BinLog::Flush(void)
{
    fh.flush();
}
