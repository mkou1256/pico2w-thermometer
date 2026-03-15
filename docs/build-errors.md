# ビルドエラー解析レポート

## 概要

`build/build.sh` 実行時に **リンクエラーが6件** 発生。CMakeの設定フェーズは正常終了し、コンパイルも全て成功するが、最終リンク段階で失敗する。

---

## エラー一覧

```
undefined reference to `ledToggle_task'
undefined reference to `vApplicationMallocFailedHook'
undefined reference to `vApplicationStackOverflowHook'
undefined reference to `vApplicationGetIdleTaskMemory'
undefined reference to `vApplicationGetPassiveIdleTaskMemory'
undefined reference to `vApplicationGetTimerTaskMemory'
```

---

## エラー詳細

### 1. `undefined reference to 'ledToggle_task'`

**原因: C/C++ リンケージの不一致**

| ファイル | 言語 | シンボル名（リンカ視点） |
|---|---|---|
| `src/main.c` | C | `ledToggle_task`（マングリングなし） |
| `src/app/led_toggle.cpp` | C++ | `_Z13ledToggle_taskPv`（C++マングリング） |

`main.c`（Cファイル）から`ledToggle_task`を呼び出しているが、`led_toggle.cpp`（C++ファイル）でC++の名前マングリングが適用されるため、リンカがシンボルを解決できない。

`src/app/led_toggle.h`に`extern "C"`ガードが不足している。

**修正方法:**

```c
// src/app/led_toggle.h
#ifndef LED_TOGGLE_H
#define LED_TOGGLE_H

#ifdef __cplusplus
extern "C" {
#endif

void ledToggle_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* LED_TOGGLE_H */
```

---

### 2〜6. FreeRTOSフック関数・静的メモリ関数の未実装

`FreeRTOSConfig.h`で有効化したオプションに対応するフック関数が、ユーザーコード側に実装されていない。

| エラー | 原因となる設定 | ファイル |
|---|---|---|
| `vApplicationMallocFailedHook` | `configUSE_MALLOC_FAILED_HOOK 1` | `FreeRTOSConfig.h:143` |
| `vApplicationStackOverflowHook` | `configCHECK_FOR_STACK_OVERFLOW 2` | `FreeRTOSConfig.h:138` |
| `vApplicationGetIdleTaskMemory` | `configSUPPORT_STATIC_ALLOCATION 1` | `FreeRTOSConfig.h:114` |
| `vApplicationGetPassiveIdleTaskMemory` | `configSUPPORT_STATIC_ALLOCATION 1` + RP2350ポート | `FreeRTOSConfig.h:114` |
| `vApplicationGetTimerTaskMemory` | `configSUPPORT_STATIC_ALLOCATION 1` + `configUSE_TIMERS 1` | `FreeRTOSConfig.h:114,183` |

`vApplicationGetPassiveIdleTaskMemory`はRP2350ポート（`RP2350_ARM_NTZ`）特有の要求であり、静的割り当てが有効な場合は常に必要になる（`FREE_RTOS_KERNEL_SMP`の有無によらない）。

---

## FreeRTOSConfig.h の修正方針

### 最低限の起動に必要な変更（最小構成）

コンパイル・起動を通すだけなら、以下の2点を変更してフック関数の要求を無効化する。

```c
// デバッグフックを無効化（実装なしで起動できる）
#define configCHECK_FOR_STACK_OVERFLOW 0  // 変更: 2 → 0
#define configUSE_MALLOC_FAILED_HOOK   0  // 変更: 1 → 0

// 静的割り当てを無効化（GetMemory系関数が不要になる）
#define configSUPPORT_STATIC_ALLOCATION 0  // 変更: 1 → 0
```

> **注意:** `configSUPPORT_STATIC_ALLOCATION 0`にした場合、`configSUPPORT_DYNAMIC_ALLOCATION 1`が必須となる。現在の設定はすでに有効なので問題なし。

---

### ベストプラクティス: フック関数を実装する（推奨）

設定を変えずに、全フック関数をユーザーコードに実装する。デバッグ能力を維持したまま起動できる。

新規ファイル `src/freertos_hooks.c` を作成し、以下を実装する:

```c
// src/freertos_hooks.c
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

/* --- スタックオーバーフロー検出 (configCHECK_FOR_STACK_OVERFLOW 2) --- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("STACK OVERFLOW: task '%s'\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

/* --- malloc失敗検出 (configUSE_MALLOC_FAILED_HOOK 1) --- */
void vApplicationMallocFailedHook(void)
{
    printf("MALLOC FAILED\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

/* --- アイドルタスク用静的メモリ (configSUPPORT_STATIC_ALLOCATION 1) --- */
void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t  **ppxIdleTaskStackBuffer,
    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* --- パッシブアイドルタスク用静的メモリ (RP2350ポート固有) --- */
void vApplicationGetPassiveIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t  **ppxIdleTaskStackBuffer,
    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize,
    BaseType_t xPassiveIdleTaskIndex)
{
    /* コア数分のバッファを用意 (RP2350はデュアルコア) */
    static StaticTask_t xPassiveIdleTaskTCB[configNUMBER_OF_CORES - 1];
    static StackType_t  uxPassiveIdleTaskStack[configNUMBER_OF_CORES - 1][configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xPassiveIdleTaskTCB[xPassiveIdleTaskIndex];
    *ppxIdleTaskStackBuffer = uxPassiveIdleTaskStack[xPassiveIdleTaskIndex];
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* --- タイマータスク用静的メモリ (configUSE_TIMERS 1 + 静的割り当て) --- */
void vApplicationGetTimerTaskMemory(
    StaticTask_t **ppxTimerTaskTCBBuffer,
    StackType_t  **ppxTimerTaskStackBuffer,
    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
```

---

## 修正サマリー

| # | 修正対象 | 修正内容 |
|---|---|---|
| 1 | `src/app/led_toggle.h` | `extern "C"` ガードを追加 |
| 2 | `src/freertos_hooks.c`（新規） | 5つのフック・静的メモリ関数を実装 |

> `FreeRTOSConfig.h` 自体の設定値はベストプラクティス構成のまま変更不要。フック関数を実装すれば全エラーが解消される。

---

## 参考: FreeRTOSConfig.h 設定の依存関係

```
configSUPPORT_STATIC_ALLOCATION = 1
  → vApplicationGetIdleTaskMemory()       が必須
  → vApplicationGetPassiveIdleTaskMemory() が必須 (RP2350ポート)
  → vApplicationGetTimerTaskMemory()      が必須 (configUSE_TIMERS=1 と組み合わせ)

configCHECK_FOR_STACK_OVERFLOW = 2
  → vApplicationStackOverflowHook()       が必須

configUSE_MALLOC_FAILED_HOOK = 1
  → vApplicationMallocFailedHook()        が必須
```