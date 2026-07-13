#include "app_assert.h"
#include "pico/stdio_usb.h"
#include "FreeRTOS.h"
#include "task.h"

void app_assert(bool condition, const char *file, int line) {
    if (!condition) {
        printf("!!ASSERT!! @ %s l. %d\n", file, line);
        taskDISABLE_INTERRUPTS();
        while (1) {__wfi();}
    }
}
