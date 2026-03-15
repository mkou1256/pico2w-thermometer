#include "led_toggle.h"
#include "common.h"

void ledToggle_task(void *pvParameters)
{
    (void)pvParameters; // 未使用のパラメータを明示的に無視
    cyw43_arch_init(); // Wi-Fiアーキテクチャの初期化（LED制御に必要）
    
    while (true)
    {
        // LEDの状態をトグル
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // LED ON
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms待機
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // LED OFF
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms待機
    }
}
