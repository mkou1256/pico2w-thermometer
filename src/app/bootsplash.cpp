#include "bootsplash.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define SPLASH_LINE(line) printf("%s\n", line)

static const char* const s_splash_raspi[] = {
    R"(     _ )",
    R"(    | |  50-   _____  _   _  _____  ____   __  __   ___ )",
    R"(    | |       |_   _|| | | || ____||  _ \ |  \/  | / _ \ )",
    R"(    | |  40-    | |  | |_| ||  _|  | |_) || |\/| || | | | )",
    R"(    | |         | |  |  _  || |___ |  _ < | |  | || |_| | )",
    R"(    |#|  30-    |_|  |_| |_||_____||_| \_\|_|  |_| \___/ )",
    R"(    |#| )",
    R"(    |#|  20-    pico2w thermometer  v0.1.0 (a1b2c3d) )",
    R"(    |#|         ------------------------------------ )",
    R"(    |#|  10-    Build : 2026-11-22 21:05 )",
    R"(   .'#'.        Reset : power-on )",
    R"(  ( ### )       Chip  : RP2350 @ 150MHz )",
    R"(   '---'        Temp  : 23.4 C )",
};

void bootSplash_task(void* params) {
    (void)params;

    /* USB接続待ち */
    while (!stdio_usb_connected()) {
        vTaskDelay(100);
    }

    int arr_size = sizeof(s_splash_raspi) / sizeof(s_splash_raspi[0]);
    for (int i = 0; i < arr_size; i++) {
        const char* line = s_splash_raspi[i];
        SPLASH_LINE(line);
    }
    vTaskDelete(nullptr);
}