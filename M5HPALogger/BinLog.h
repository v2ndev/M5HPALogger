#pragma once
#include <M5Stack.h>

#define BINLOG_HEADER_LENGTH (8)
#define BINLOG_DATA_LENGTH (32)

typedef struct 
{
    uint8_t header;
    uint8_t _reserved;
    uint8_t deviceId;
    uint8_t pageId;
    uint32_t time_ms;
    uint8_t data[BINLOG_DATA_LENGTH];
} BinLogRecord;

static_assert(sizeof(BinLogRecord) == BINLOG_HEADER_LENGTH + BINLOG_DATA_LENGTH, "BinLogFormat size mismatch.");

class BinLog
{
    private:
        File fh;
        char fname[64];
    public:
        bool OpenFileSequential(const char *name, const int max);
        template <typename T>
        void WriteLog(const uint8_t deviceId, const uint8_t pageId, const T *data) {WriteLog(deviceId, pageId, data, sizeof(T));}
        template <typename T>
        void WriteLog(const uint8_t deviceId, const uint8_t pageId, const T *data, const size_t len)  {WriteLog(deviceId, pageId, reinterpret_cast<const uint8_t *>(data), len);}
        void WriteLog(const uint8_t deviceId, const uint8_t pageId, const uint8_t *data, const size_t len);
        void Flush(void);
};


