#ifndef COMMODULE_H
#define COMMODULE_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "app_assert.h"

#define COM_ERR_CONNECT (-1)
#define COM_ERR_TRANSPORT (-2)

enum class ComStatus {
    Ok = 0,
    Timeout,
    NotConnected,
    ArgError,
    TransportError,
};

class ComModule {
public:
    virtual ~ComModule() = default;
    ComStatus Send(const uint8_t* data, size_t length, uint32_t timeout, size_t* bytes_sent);
    ComStatus Recv(uint8_t* data, size_t length, uint32_t timeout, size_t* bytes_recv);
protected:
    ComModule() {
        send_mtx = xSemaphoreCreateMutex();
        recv_mtx = xSemaphoreCreateMutex();
        ASSERT(send_mtx != nullptr);
        ASSERT(recv_mtx != nullptr);
    }
    virtual int32_t WriteSome(const uint8_t* data, size_t length) = 0;
    virtual int32_t ReadSome(uint8_t* data, size_t length) = 0;
private:
    virtual bool Init() = 0;
    SemaphoreHandle_t send_mtx;
    SemaphoreHandle_t recv_mtx;
};
#endif // COMMODULE_H