#include "Logger.h"
#include "ComModule.h"
#include "projdefs.h"
#include <cstdarg>
#include <hardware/timer.h>
#include <pico/types.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "pico/time.h"

#define LOG_BUFFER_SIZE 256

static bool init_ = false;
static ComModule* com_module_ = nullptr;
static LogLvl log_level_ = LogLvl::INFO;
static SemaphoreHandle_t log_mutex_ = nullptr;
static char log_buffer_[LOG_BUFFER_SIZE];

void Logger::init(ComModule* comModule) {
    if (comModule == nullptr) {
        return;
    }

    com_module_ = comModule;
    log_level_ = LogLvl::DEBUG;
    log_mutex_ = xSemaphoreCreateMutex();
    if (log_mutex_ == nullptr) {
        return;
    }

    memset(log_buffer_, 0, sizeof(log_buffer_));

    init_ = true;
    return;
}

static const char* get_lvl_str(LogLvl lvl) {
    switch (lvl) {
        case LogLvl::DEBUG: return "D";
        case LogLvl::INFO: return "I";
        case LogLvl::WARNING: return "W";
        case LogLvl::ERROR: return "E";
        default: return "";
    }
}

static int ensure_crlf(char* buffer) {
    int len = strlen(buffer);
    if (len <= 2) {
        return len;
    }
    
    /* 終端OK */
    if ((buffer[len - 2] == '\r') && (buffer[len - 1] == '\n')) {
        return len;
    }

    /* LFのみ */
    if (buffer[len - 1] == '\n') {
        buffer[len - 1] = '\r';
        buffer[len] = '\n';
        buffer[len + 1] = '\0';
        return len + 1;
    }

    /* CRのみ */
    if (buffer[len - 1] == '\r') {
        buffer[len] = '\n';
        buffer[len + 1] = '\0';
        return len + 1;
    }

    /* 何もなし */
    buffer[len] = '\r';
    buffer[len + 1] = '\n';
    buffer[len + 2] = '\0';
    return len + 2;
}

void Logger::log(LogLvl level, int line, const char* message, ...) {
    if (!init_) return;
    if (!message) return;

    if (xSemaphoreTake(log_mutex_, 0) != pdTRUE) return;

    int len;
    va_list args;

    va_start(args, message);
    len = snprintf(log_buffer_, sizeof(log_buffer_), "[%u][%s][%d]", 
        (uint32_t)(time_us_64() / 1000), get_lvl_str(level), line);
    /* CRLFを書き込む可能性があるから後ろ2バイト空けておく */
    (void)vsniprintf(log_buffer_ + len, sizeof(log_buffer_) - len - 2, message, args);
    va_end(args);

    /* 改行コード保証: CRLF終端でない場合は、末尾3バイトを\r\n\0にする */
    ensure_crlf(log_buffer_);

    com_module_->Send((uint8_t*)log_buffer_, strlen(log_buffer_), 0, nullptr);

    xSemaphoreGive(log_mutex_);
}

LogLvl Logger::log_level() {
    return log_level_;
}