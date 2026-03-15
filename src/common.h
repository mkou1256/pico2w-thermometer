#ifndef COMMON_H
#define COMMON_H

// For general C/C++ definitions
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// For Pico SDK
#include "pico/cyw43_arch.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

// For FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT(x) app_assert((x), __FILE__, __LINE__)
void app_assert(bool condition, const char *file, int line);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */