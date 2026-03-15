#include "static_object.h"
#include "common.h"

#include "led_toggle.h"
DEF_CTSK(led_ctsk, ledToggle_task, 2, 512);

enum {
    LED_TASK_ID = 1,
    TASK_ID_MAX,
};

typedef struct {
    ID id;
    const T_CTSK *ctsk;
} StaticTaskInfo_t;

StaticTaskInfo_t s_static_tasks[] = {
    { LED_TASK_ID, &led_ctsk },
    { TASK_ID_MAX, NULL } /* 終端マーカー */
};

ER cre_static_tasks(void)
{
    StaticTaskInfo_t *info;
    for (size_t i = 0; i < TASK_ID_MAX; i++) {
        info = &s_static_tasks[i];
        if ((info->id == TASK_ID_MAX)) {
            break; /* 終端マーカーに到達 */
        }
        ER ercd = cre_tsk(info->id, info->ctsk);
        if (ercd != E_OK) {
            ASSERT(0);
            return ercd;
        }
    }
    return E_OK;
}