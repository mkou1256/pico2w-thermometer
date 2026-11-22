#include "UsbCdcComTest.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ComModule.h"
#include "UsbCdcCom.h"
#include <string.h>
#include <stdio.h>

static uint8_t s_buf[1024];

static void comSmokeTest(void* pvParameters) {
    ComModule* comModule = (ComModule*)pvParameters;
    const char* testMessage = "Hello, World!\r\n";

    for (;;) {
        comModule->Send((const uint8_t*)testMessage, strlen(testMessage), 100, nullptr);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void comSweepTest(void* pvParameters) {
    ComModule* comModule = (ComModule*)pvParameters;
    
    // s_bufに0x00から0xFFまでの値を順番に格納
    for (int i = 0; i < sizeof(s_buf); ++i) {
        s_buf[i] = (uint8_t)('0' + (i % 10));
    }

    for (size_t n = 1; n<= 1024; n++) {
        char hdr[16];
        int hl = snprintf(hdr, sizeof(hdr), "[%4u] ", n);
        comModule->Send((const uint8_t*)hdr, hl, 100, nullptr);

        size_t sent;
        ComStatus status = comModule->Send(s_buf, n, 1000, &sent);

        char tail[48];
        int tl = snprintf(tail, sizeof(tail), " st=%d sent=%u %s\r\n", 
            status, sent, (status == ComStatus::Ok) ? "OK" : "NG");
        comModule->Send((const uint8_t*)tail, tl, 100, nullptr);

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);

}

void run_UsbCdcComTest() {
    ComModule* comModule = UsbCdcCom::Create();
    if (comModule == nullptr) {
        return;
    }

    xTaskCreate(comSweepTest, "COM Sweep Test", configMINIMAL_STACK_SIZE*2, (void*)comModule, tskIDLE_PRIORITY + 1, NULL);
}