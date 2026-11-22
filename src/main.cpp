#include "Logger.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "kernel.h"
#include <pico/time.h>
#include "app_assert.h"
#include "communication/app_com.h"
#include "dri_adc.h"

#define INIT_TASK_STACKDEPTH    STACK_DEPTH_MID
#define INIT_TASK_PRI           TASK_PRI_HIGHEST
#define INIT_TASK_PRI_LOW       TASK_PRI_LOWEST

static bool init_drivers(void) {
    if (cyw43_arch_init() != 0) return false;
    ADC_init(AdcTsEn::ENABLE);

    return true;
}

void init_task(void*) {
    /* ログ初期化 */
    AppCom_Init();
    Logger::init(getComModule(COM_CHANNEL_USB_SERIAL));
    sleep_ms(1000);

    /* ドライバー初期化 */
    bool ret_dri = init_drivers();
    ASSERT(ret_dri == true);

    /* カーネル生成 */
    int ret_kernel = kernel_init();
    ASSERT(ret_kernel == true);

    /* アプリケーション初期化 */


    vTaskDelete(nullptr);
}

int main(void)
{
    /* ボード初期化 */
    stdio_init_all();

    xTaskCreate(init_task, 
        "init_task", 
        INIT_TASK_STACKDEPTH, 
        nullptr, 
        INIT_TASK_PRI, 
        nullptr);
    vTaskStartScheduler();

    for (;;) {}
    return 0;
}
