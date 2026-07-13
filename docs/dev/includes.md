# include 早見表（pico-sdk / FreeRTOS）

IWYU（Include What You Use）で実装するための早見表。使う機能ごとに必要な include とリンクをまとめる。
**自作の umbrella ヘッダ（common.h / kernel.h 等の全部入り）は作らず、ここを引いて必要分だけ書く**こと。

> 注意: **include しただけでは動かない**。CMake の `target_link_libraries` でリンクも要る（組み込みの落とし穴）。

## pico-sdk

| 機能 | include | CMake リンク |
|---|---|---|
| 標準初期化・GPIO・時間・UART（**公式 umbrella**） | `pico/stdlib.h` | `pico_stdlib` |
| USB シリアル出力（`stdio_usb_init`） | `pico/stdio_usb.h` | `pico_stdio_usb` |
| **ADC / 内蔵温度センサ** | `hardware/adc.h` | `hardware_adc` |
| GPIO 個別操作（stdlib に含まれる） | `hardware/gpio.h` | `hardware_gpio` |
| sleep_ms / 時間（stdlib に含まれる） | `pico/time.h` | （`pico_stdlib`） |
| I2C（将来 BME280 等） | `hardware/i2c.h` | `hardware_i2c` |
| SPI | `hardware/spi.h` | `hardware_spi` |
| PWM | `hardware/pwm.h` | `hardware_pwm` |
| **Pico W オンボード LED / Wi-Fi** | `pico/cyw43_arch.h` | LED のみ: `pico_cyw43_arch_none` / Wi-Fi: `pico_cyw43_arch_lwip_threadsafe_background` |
| ウォッチドッグ | `hardware/watchdog.h` | `hardware_watchdog` |

## FreeRTOS

| 機能 | include | 備考 |
|---|---|---|
| **コア（必須・最初）** | `FreeRTOS.h` | **必ず最初**に。`FreeRTOSConfig.h` を読む |
| タスク・遅延・スケジューラ・タスク通知 | `task.h` | `xTaskCreate` / `vTaskDelay` / `vTaskStartScheduler` / `xTaskNotify` |
| キュー | `queue.h` | `xQueueCreate` / `xQueueSend` / `xQueueReceive` |
| セマフォ・ミューテックス | `semphr.h` | `xSemaphoreCreateMutex` / `xSemaphoreTake` |
| ソフトウェアタイマー | `timers.h` | `xTimerCreate` |
| イベントグループ | `event_groups.h` | `xEventGroupSetBits` |
| ストリーム / メッセージバッファ | `stream_buffer.h` / `message_buffer.h` | センサ→送信のデータ流しに向く |

FreeRTOS のリンクは CMake で `FreeRTOS-Kernel` ＋ ヒープ実装（`FreeRTOS-Kernel-Heap4` など）。個別機能ごとのリンクは不要（カーネル一括）。

## お作法メモ

- **`FreeRTOS.h` は必ず最初、他の FreeRTOS ヘッダより前**（設定を読むため）。順序を間違うとコンパイルエラー。
- **`pico/stdlib.h` は使って良い umbrella**。「ライブラリが用意した公式の入口」なので、自作 umbrella とは別物。gpio/time/uart を束ねてくれる分は乗る。
- stdlib に**含まれない**もの（`hardware/adc.h` 等）は**個別に書く** = ここが IWYU の効きどころ。

## このプロジェクトのファイル別（実装時の目安）

```
main.cpp        : pico/stdlib.h, FreeRTOS.h, task.h
app/led_toggle  : FreeRTOS.h, task.h, pico/cyw43_arch.h   (Pico W の LED は cyw43 経由)
app/thermometer : FreeRTOS.h, task.h, hardware/adc.h, pico/stdio.h   (ADC + printf)
system/hooks    : FreeRTOS.h, task.h
system/fault    : FreeRTOS.h, task.h, pico/stdlib.h       (vTaskSuspendAll + sleep_ms/printf)
```
