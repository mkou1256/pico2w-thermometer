#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "kernel.h"
#include <pico/time.h>

#if 1
#define BOOT_DBG(fmt, ...) printf("[BOOT] " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#else
#define BOOT_DBG(str)   void(str)
#endif

#define INIT_TASK_STACKDEPTH    STACK_DEPTH_MID
#define INIT_TASK_PRI           TASK_PRI_HIGHEST
#define INIT_TASK_PRI_LOW       TASK_PRI_LOWEST
void init_task(void*) {
    BOOT_DBG("===> start init_task()\r");
    /* ドライバー初期化 */
    int ret_cyw43 = cyw43_arch_init();
    BOOT_DBG("cyw43_arch_init() ret = %d", ret_cyw43);

    /* カーネル生成 */
    int ret_kernel = kernel_init();
    BOOT_DBG("kernel_init() ret = %d", ret_kernel);


    /* アプリケーションタスク起動 */

    BOOT_DBG("<=== end init_task()\r");
    vTaskDelete(nullptr);
}

int main(void)
{
    /* ボード初期化 */
    stdio_init_all();
    BOOT_DBG("stdio_init_all() done\r");

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
