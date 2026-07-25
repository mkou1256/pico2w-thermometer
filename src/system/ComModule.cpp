#include "ComModule.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/time.h"

ComStatus ComModule::Send(const uint8_t* data, size_t length, uint32_t timeout, size_t* bytes_sent) {
    // 引数チェック
    if (data == nullptr || length == 0) {
        return ComStatus::ArgError;
    }
    if (bytes_sent) {
        *bytes_sent = 0;
    }

    // タイムアウト計算開始。mutex取得でも待たされる可能性があるため
    absolute_time_t deadline = make_timeout_time_ms(timeout);

    // タスクコンテキストチェック & mutex取得
    BaseType_t taskstate = xTaskGetSchedulerState();
    bool doYield;
    if (taskstate != taskSCHEDULER_RUNNING) {
        doYield = false;
    } else {
        TickType_t wait = (timeout == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout);
        if (xSemaphoreTake(send_mtx, wait) != pdTRUE) {
            return ComStatus::Timeout;
        }
        doYield = true;
    }

    // 送信処理
    ComStatus ret = ComStatus::Ok;
    while (length > 0) {
        int32_t sent = WriteSome(data, length);
        if (sent == COM_ERR_CONNECT) {
            ret = ComStatus::NotConnected;
            goto exit;
        } else if (sent == COM_ERR_TRANSPORT) {
            ret = ComStatus::TransportError;
            goto exit;
        } else if (sent < 0) {
            ret = ComStatus::TransportError;
            goto exit;
        }

        data += sent;
        length -= sent;
        if (bytes_sent) {
            *bytes_sent += sent;
        }

        if (length == 0) {
            break;
        }

        if (timeout != UINT32_MAX && time_reached(deadline)) {
            ret = ComStatus::Timeout;
            goto exit;
        }

        if (doYield) {
            vTaskDelay(1);
        }
    }

exit:
    // mutex解放
    if (taskstate == taskSCHEDULER_RUNNING) {
        xSemaphoreGive(send_mtx);
    }
    return ret;
}

ComStatus ComModule::Recv(uint8_t* data, size_t length, uint32_t timeout, size_t* bytes_recv) {
    // Implementation for receiving data
    return ComStatus::NotConnected; // Placeholder return value
}
