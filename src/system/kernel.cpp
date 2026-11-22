#include "FreeRTOS.h"
#include "task.h"
#include "kernel.h"

/* タスク関数用のinclude */
#include "led_toggle.h"
#include "bootsplash.h"
#include "temp_sensor.h"
#include <cstddef>

struct TaskItem {
    TaskFunction_t fn;
    const char * const name;
    const configSTACK_DEPTH_TYPE stack_depth;
    void * const params;
    UBaseType_t pri;
};

enum TaskNum {
    TASK_LED_TOGGLE = 0,
    TASK_BOOT_SPLASH,
    TASK_TEMP_SENSOR,
    TASK_MAX,
};
/* この表をもとにタスク生成する */
static TaskItem s_taskTable[TASK_MAX] = {
    {ledToggle_task,    "led_toggle",   STACK_DEPTH_SMALL,   nullptr,   TASK_PRI_LOW},
    {bootSplash_task,   "boot_splash",  STACK_DEPTH_LARGE,  nullptr,        TASK_PRI_HIGH},
    {tempSensor_task,   "temp_sensor",  STACK_DEPTH_MID,    nullptr,    TASK_PRI_HIGH},
};
static TaskHandle_t s_taskHandles[TASK_MAX];    /* 生成したハンドルはここに入れる */ 

/**
 * RTOS資源生成を行う関数。
 * vTaskStartScheduler() 後にタスク生成すると即座に開始される。
**/
bool kernel_init(void) {
    /* タスク生成 */
    for (int i = 0; i < TASK_MAX; i++) {
        TaskItem tmp = s_taskTable[i];
        TaskHandle_t *handle = &s_taskHandles[i];
        BaseType_t ret  = xTaskCreate(tmp.fn, tmp.name, tmp.stack_depth, tmp.params, tmp.pri, handle);
        if (ret != pdPASS) { 
            return false;
        }
    }
    return true;
}
