#include "led_toggle.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"

void ledToggle_task(void *pvParameters)
{
    (void)pvParameters; // 未使用のパラメータを明示的に無視

    while (true)
    {
        // LEDの状態をトグル
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // LED ON
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms待機
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // LED OFF
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms待機
    }
}
