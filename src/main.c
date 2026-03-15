#include "common.h"
#include "static_object.h"

int main(void)
{
    stdio_init_all();
    printf("Hello, Raspberry Pi Pico 2 W with FreeRTOS!\n");
    ER ercd = cre_static_tasks();
    if (ercd != E_OK) {
        printf("Failed to create static tasks\n");
        return -1;
    }
    vTaskStartScheduler();

    for (;;) {}
    return 0;
}
