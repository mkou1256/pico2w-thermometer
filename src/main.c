#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"
#include "app_com.h"
#include "app_assert.h"
#include "test_target.h"

static bool pico_init();
static bool app_init();
static bool test_init();

int main(void)
{
    // USBシリアルの初期化
    if (!pico_init()) {
        ASSERT(0);
    }

    // アプリケーションの初期化
    if (!app_init()) {
        ASSERT(0);
    }

    // テストの初期化。未使用時はOFFにすること
    if (!test_init()) {
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

    if (cyw43_arch_init() != PICO_OK) {
        ASSERT(0);
        return false;
    }

    return true;
}

bool app_init()
{
    // タスク生成
    if (!initTasks()) {
        ASSERT(0);
        return false;
    }

    // 通信モジュールの初期化
    if (!AppCom_Init()) {
        ASSERT(0);
        return false;
    }
    return true;
}

bool test_init()
{
#ifdef USBCDC_COM_TEST
    // UsbCdcCom
    run_UsbCdcComTest();
#endif // USBCDC_COM_TEST

    return true;
}