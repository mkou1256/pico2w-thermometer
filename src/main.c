#include <stdbool.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"
#include "app_assert.h"

static bool pico_init();

int main(void)
{
    // USBシリアルの初期化
    if (!pico_init()) {
        ASSERT(0);
    }

    // 静的タスクの作成
    if (!initTasks()) {
        ASSERT(0);
    }

    vTaskStartScheduler();

    for (;;) {}
    return 0;
}

bool pico_init()
{
    if (!stdio_init_all()) {
        ASSERT(0);
        return false;
    }

    if (!cyw43_arch_init()) {
        ASSERT(0);
        return false;
    }

    return true;
}