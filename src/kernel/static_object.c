#include "static_object.h"
#include "freertos_wrapper.h"

#include "led_toggle.h"
DEF_CTSK(led_ctsk, ledToggle_task, 2, 512);

ER cre_static_tasks(void)
{
    ER ercd = cre_tsk(1, &led_ctsk);
    if (ercd != E_OK) return ercd;
    return E_OK;
}