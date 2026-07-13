/*
 * FreeRTOSConfig.h - Raspberry Pi Pico 2 W用
 * pico-examplesの公式設定をベースに詳細な説明を追加
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ========================================================================
 * スケジューラ関連設定
 * ======================================================================== */

/* プリエンプティブマルチタスク: 1=有効, 0=協調的マルチタスク
 * 有効にすると高優先度タスクが即座に実行される */
#define configUSE_PREEMPTION 1

/* チックレスアイドルモード: 1=有効, 0=無効
 * 有効にすると、アイドル時にティック割り込みを停止して省電力化 */
#define configUSE_TICKLESS_IDLE 0

/* アイドルフック関数: 1=有効, 0=無効
 * 有効にする場合は vApplicationIdleHook() を実装する必要がある */
#define configUSE_IDLE_HOOK 0

/* ティックフック関数: 1=有効, 0=無効
 * 有効にする場合は vApplicationTickHook() を実装する必要がある
 * 1msごとに呼ばれるので軽量な処理のみ */
#define configUSE_TICK_HOOK 0

/* システムティック周波数: 1000Hz = 1msごとにティック
 * vTaskDelay()の最小単位がこの値で決まる */
#define configTICK_RATE_HZ ((TickType_t)1000)

/* タスクの最大優先度: 0～31の32段階
 * 数値が大きいほど優先度が高い（31が最高優先度） */
#define configMAX_PRIORITIES 32

/* 最小スタックサイズ: 512ワード = 2048バイト
 * アイドルタスクなど最小限のタスクで使用される */
#define configMINIMAL_STACK_SIZE (configSTACK_DEPTH_TYPE)512

/* 16ビットティックカウンタ: 0=32ビット使用（推奨）
 * 32ビットにすることで約50日間のオーバーフロー周期 */
#define configUSE_16_BIT_TICKS 0

/* 同優先度タスクへのyield: 1=有効
 * 同じ優先度のタスクに定期的にCPU時間を譲る */
#define configIDLE_SHOULD_YIELD 1

/* ========================================================================
 * 同期・排他制御関連設定
 * ======================================================================== */

/* ミューテックス: 1=有効
 * リソースへの排他的アクセスを制御 */
#define configUSE_MUTEXES 1

/* 再帰ミューテックス: 1=有効
 * 同じタスクが複数回ロック可能なミューテックス */
#define configUSE_RECURSIVE_MUTEXES 1

/* アプリケーションタスクタグ: 1=有効
 * タスクに追加情報を関連付ける機能
 * 通常は使用しないので0でOK */
#define configUSE_APPLICATION_TASK_TAG 0

/* カウンティングセマフォ: 1=有効
 * リソースの数を管理するセマフォ */
#define configUSE_COUNTING_SEMAPHORES 1

/* キューレジストリサイズ: デバッグ用
 * キューに名前を付けて管理できる数 */
#define configQUEUE_REGISTRY_SIZE 8

/* キューセット: 1=有効
 * 複数のキュー/セマフォを同時に監視 */
#define configUSE_QUEUE_SETS 1

/* タイムスライシング: 1=有効
 * 同優先度タスク間でCPU時間を分割 */
#define configUSE_TIME_SLICING 1

/* Newlibリエントラント: 1=有効
 * Newlibの再入可能サポート（通常は不要） */
#define configUSE_NEWLIB_REENTRANT 0

/* 後方互換性: 1=有効
 * lwIP等の古いコードとの互換性のため有効化 */
#define configENABLE_BACKWARD_COMPATIBILITY 1

/* スレッドローカルストレージポインタ数: 5個
 * タスク固有のデータを保存できる領域の数 */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* ========================================================================
 * システム設定
 * ======================================================================== */

/* スタック深度の型: uint32_t
 * スタックサイズを指定する際の型 */
#define configSTACK_DEPTH_TYPE uint32_t

/* メッセージバッファ長の型: size_t
 * メッセージバッファのサイズ指定用 */
#define configMESSAGE_BUFFER_LENGTH_TYPE size_t

/* ========================================================================
 * メモリ割り当て設定
 * ======================================================================== */

/* 静的メモリ割り当て: 0=無効, 1=有効
 * コンパイル時にメモリを確保（動的割り当てなし） */
#ifndef configSUPPORT_STATIC_ALLOCATION
#define configSUPPORT_STATIC_ALLOCATION 1
#endif

/* 動的メモリ割り当て: 1=有効
 * 実行時にヒープからメモリを確保 */
#ifndef configSUPPORT_DYNAMIC_ALLOCATION
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#endif

/* ヒープサイズ: 128KB
 * タスク、キュー、セマフォなどに使用される総メモリ量
 * Pico 2 Wは264KB SRAMなので約半分を割り当て */
#define configTOTAL_HEAP_SIZE (128 * 1024)

/* アプリケーション割り当てヒープ: 0=FreeRTOSが管理
 * 1にするとアプリケーション側でヒープ配列を定義する必要あり */
#define configAPPLICATION_ALLOCATED_HEAP 0

/* ========================================================================
 * フック関数設定
 * ======================================================================== */

/* スタックオーバーフロー検出: 0=無効, 1=方法1, 2=方法2
 * TODO: デバッグ時は2に設定し、vApplicationStackOverflowHook()を実装 */
#define configCHECK_FOR_STACK_OVERFLOW 2

/* malloc失敗フック: 0=無効, 1=有効
 * TODO: メモリ不足検出が必要な場合は1に設定し、
 *       vApplicationMallocFailedHook()を実装 */
#define configUSE_MALLOC_FAILED_HOOK 1

/* デーモンタスク起動フック: 0=無効, 1=有効
 * タイマーサービスタスク起動時に呼ばれる
 * TODO: 初期化処理が必要な場合に1に設定 */
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

/* ========================================================================
 * 実行時統計・トレース設定
 * ======================================================================== */

/* 実行時統計生成: 0=無効, 1=有効
 * 各タスクのCPU使用率を計測
 * TODO: パフォーマンス分析時に1に設定 */
#define configGENERATE_RUN_TIME_STATS 0

/* トレース機能: 1=有効
 * タスク状態のトレースを記録（デバッグツール用） */
#define configUSE_TRACE_FACILITY 1

/* 統計フォーマット関数: 0=無効
 * 統計情報を文字列化する関数群
 * TODO: 統計表示が必要な場合に1に設定 */
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

/* ========================================================================
 * コルーチン設定（非推奨機能）
 * ======================================================================== */

/* コルーチン: 0=無効
 * 古い軽量タスク機構（現在は非推奨、タスクを使用推奨） */
#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES 1

/* ========================================================================
 * ソフトウェアタイマー設定
 * ======================================================================== */

/* ソフトウェアタイマー: 1=有効
 * タイマーコールバック機能を使用可能 */
#define configUSE_TIMERS 1

/* タイマータスク優先度: 最高優先度-1
 * タイマーの精度を確保するため高優先度に設定 */
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/* タイマーキュー長: 10個
 * 同時にキューイングできるタイマーコマンド数 */
#define configTIMER_QUEUE_LENGTH 10

/* タイマータスクスタック: 1024ワード = 4096バイト
 * タイマーコールバックで複雑な処理をする場合は増やす */
#define configTIMER_TASK_STACK_DEPTH 1024

/* ========================================================================
 * SMP（マルチコア）設定
 * ======================================================================== */

#if FREE_RTOS_KERNEL_SMP // RP2xxx SMP portで自動定義される

/* コア数: 2（Pico 2 WはデュアルコアCortex-M33）
 * 両方のコアでFreeRTOSを実行 */
#ifndef configNUMBER_OF_CORES
#define configNUMBER_OF_CORES 2
#endif
#define configNUM_CORES configNUMBER_OF_CORES

/* ティック生成コア: Core 0
 * システムティックを生成するコアを指定 */
#define configTICK_CORE 0

/* 複数優先度の並列実行: 1=有効
 * 異なる優先度のタスクを両コアで同時実行可能 */
#define configRUN_MULTIPLE_PRIORITIES 1

/* コアアフィニティ: 1=有効（デュアルコア時）
 * タスクを特定のコアに固定できる機能 */
#if configNUMBER_OF_CORES > 1
#define configUSE_CORE_AFFINITY 1
#endif

/* パッシブアイドルフック: 0=無効
 * 各コアのアイドル状態を個別に処理 */
#define configUSE_PASSIVE_IDLE_HOOK 0

#endif // FREE_RTOS_KERNEL_SMP

/* ========================================================================
 * Pico SDK連携設定
 * ======================================================================== */

/* Pico同期機構との相互運用: 1=有効
 * Pico SDKのmutex/semaphoreとFreeRTOSの統合 */
#define configSUPPORT_PICO_SYNC_INTEROP 0

/* Pico時間関数との相互運用: 1=有効
 * Pico SDKの時間関数とFreeRTOSの統合 */
#define configSUPPORT_PICO_TIME_INTEROP 0

/* ========================================================================
 * デバッグ設定
 * ======================================================================== */

#include <assert.h>
#include <stdio.h>

/* アサーション: デバッグ時にエラーをトラップ
 * リリースビルドではassertを無効化することも可能 */
// #define configASSERT(x) assert(x)
/* デバッグ用に変更: 失敗箇所をprintf出力 */
#define configASSERT(x)                                                                            \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            __asm volatile("cpsid i" ::: "memory");                                                \
            printf("ASSERT FAILED: %s, file %s, line %d\n", #x, __FILE__, __LINE__);               \
            for (;;)                                                                               \
            {                                                                                      \
                __asm volatile("nop");                                                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/* ========================================================================
 * API関数の有効化
 * ======================================================================== */

/* タスク優先度設定: 1=有効 */
#define INCLUDE_vTaskPrioritySet 1

/* タスク優先度取得: 1=有効 */
#define INCLUDE_uxTaskPriorityGet 1

/* タスク削除: 1=有効 */
#define INCLUDE_vTaskDelete 1

/* タスク一時停止: 1=有効 */
#define INCLUDE_vTaskSuspend 1

/* 絶対時刻での遅延: 1=有効
 * vTaskDelayUntil()で周期タスクを実装 */
#define INCLUDE_vTaskDelayUntil 1

/* 相対時刻での遅延: 1=有効
 * vTaskDelay()で指定時間待機 */
#define INCLUDE_vTaskDelay 1

/* スケジューラ状態取得: 1=有効 */
#define INCLUDE_xTaskGetSchedulerState 1

/* 現在のタスクハンドル取得: 1=有効 */
#define INCLUDE_xTaskGetCurrentTaskHandle 1

/* スタック使用量取得: 1=有効
 * デバッグ時にスタック使用状況を確認 */
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/* アイドルタスクハンドル取得: 1=有効 */
#define INCLUDE_xTaskGetIdleTaskHandle 1

/* タスク状態取得: 1=有効 */
#define INCLUDE_eTaskGetState 1

/* タイマーからの関数コール: 1=有効
 * タイマーコンテキストから関数を遅延実行 */
#define INCLUDE_xTimerPendFunctionCall 1

/* 遅延中断: 1=有効
 * vTaskDelay中のタスクを即座に起床 */
#define INCLUDE_xTaskAbortDelay 1

/* タスクハンドル取得: 1=有効
 * タスク名からハンドルを取得 */
#define INCLUDE_xTaskGetHandle 1

/* ISRからのタスク再開: 1=有効 */
#define INCLUDE_xTaskResumeFromISR 1

/* ミューテックス保持者取得: 1=有効
 * デッドロック解析に使用 */
#define INCLUDE_xQueueGetMutexHolder 1

/* ========================================================================
 * RP2350（Cortex-M33）固有設定
 * ======================================================================== */

#if PICO_RP2350

/* MPU（メモリ保護ユニット）: 0=無効
 * メモリアクセス違反検出機能
 * TODO: セキュリティが必要な場合に1に設定 */
#define configENABLE_MPU 0

/* TrustZone: 0=無効
 * ARMのセキュリティ機能（セキュア/非セキュア分離）
 * TODO: セキュアアプリケーションで1に設定 */
#define configENABLE_TRUSTZONE 0

/* FreeRTOSをセキュアのみで実行: 1=有効
 * TrustZoneを使わない場合の標準設定 */
#define configRUN_FREERTOS_SECURE_ONLY 1

/* FPU（浮動小数点ユニット）: 1=有効
 * RP2350はFPUを搭載しているので有効化推奨
 * float/double演算が高速化される */
#define configENABLE_FPU 1

/* システムコール可能な最高割り込み優先度: 16
 * これより高優先度(0-15)の割り込みはFreeRTOS APIを呼べない
 * 低優先度(16-255)の割り込みはAPI呼び出し可能 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16

#endif // PICO_RP2350

#endif /* FREERTOS_CONFIG_H */

/* ========================================================================
 * 設定のカスタマイズガイド
 * ======================================================================== */

/*
【デバッグを有効にする場合】
configCHECK_FOR_STACK_OVERFLOW = 2
configUSE_MALLOC_FAILED_HOOK = 1
以下の関数を実装:
  void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
  void vApplicationMallocFailedHook(void);

【省電力化する場合】
configUSE_TICKLESS_IDLE = 1
configUSE_IDLE_HOOK = 1
以下の関数を実装:
  void vApplicationIdleHook(void) {
      __wfi(); // Wait For Interrupt
  }

【パフォーマンス分析する場合】
configGENERATE_RUN_TIME_STATS = 1
configUSE_STATS_FORMATTING_FUNCTIONS = 1
タイマーを設定してCPU使用率を計測

【マルチコアでタスクを分散する場合】
xTaskCreate()の代わりにxTaskCreateAffinitySet()を使用:
  xTaskCreateAffinitySet(task_func, "Task1", 256, NULL, 1,
                         (1 << 0), &task_handle); // Core 0に固定
  xTaskCreateAffinitySet(task_func, "Task2", 256, NULL, 1,
                         (1 << 1), &task_handle); // Core 1に固定

【ヒープサイズの調整】
メモリ不足の場合: configTOTAL_HEAP_SIZE を増やす
使用可能なSRAM: 264KB - プログラムサイズ - スタック
推奨値: 128KB～200KB
*/
