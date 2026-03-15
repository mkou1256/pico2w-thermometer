#include "common.h"

void app_assert(bool condition, const char *file, int line) {
    if (!condition) {
        vTaskSuspendAll();
        while (1) {
            printf("!!ASSERT!! @ %s l. %d\n", file, line);
            sleep_ms(1000);
        }
    }
}
