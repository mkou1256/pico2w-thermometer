#include "common.h"
#include "led_toggle.h"

int main(void)
{
    stdio_init_all(); // USBシリアル通信の初期化
    printf("Hello, Raspberry Pi Pico 2 W with FreeRTOS!\n");

    // LEDトグルタスクの作成
    xTaskCreate(ledToggle_task, "LED Toggle Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // スケジューラの開始
    vTaskStartScheduler();

    // スケジューラが正常に開始された場合、ここには到達しない
    for (;;)
    {
        // エラー処理やリセットなどを行うことも可能
    }

    return 0; // 通常は到達しない
}