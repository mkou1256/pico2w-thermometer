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
    for (;;) {__wfi();}
}

/* --- malloc失敗検出 (configUSE_MALLOC_FAILED_HOOK 1) --- */
void vApplicationMallocFailedHook(void)
{
    printf("MALLOC FAILED\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {__wfi();}
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