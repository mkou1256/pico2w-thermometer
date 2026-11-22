#include "app_tasks.h"
#include "led_toggle.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_assert.h"

enum TaskID {
    LED_TASK_ID = 1,
    TASK_ID_MAX,
};

typedef struct {
    TaskID id;
    TaskFunction_t taskFunction;
    const char *name;
    const configSTACK_DEPTH_TYPE stackDepth;
    void *parameters;
    UBaseType_t priority;
    TaskHandle_t *taskHandle;
} TaskInfo_t;

TaskInfo_t s_tasks[] = {
    { LED_TASK_ID, &ledToggle_task, "LED Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL },
    { TASK_ID_MAX, NULL, NULL, 0, NULL, 0, NULL } /* 終端マーカー */
};

bool initTasks(void)
{
    TaskInfo_t *info;
    BaseType_t xRet;
    int i = 0;
    for (;;) {
        info = &s_tasks[i++];
        if ((info->id == TASK_ID_MAX)) {
            break; /* 終端マーカーに到達 */
        }
        xRet = xTaskCreate(info->taskFunction, info->name, info->stackDepth, info->parameters, info->priority, info->taskHandle);
        if (xRet != pdPASS) {
            ASSERT(0);
            return false;
        }
    }
    return true;
}