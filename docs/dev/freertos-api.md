# FreeRTOS API 早見表

このプロジェクト（FreeRTOS-Kernel **V11.1.0+** / RP2350 SMP / `src/config/FreeRTOSConfig.h`）で
実際に使える API を、用途別にまとめたもの。**この設定で無効な機能はその旨を明記**してある。

include の対応は `includes.md` を参照（`FreeRTOS.h` を必ず最初に）。

---

## 0. このコンフィグの前提（先に把握すること）

| 項目 | 値 | 効いてくる所 |
|---|---|---|
| `configNUMBER_OF_CORES` | 2（**SMP**） | 2タスクが**本当に同時実行**される。「高優先度が動いている間は低優先度が止まる」は成立しない |
| `configUSE_CORE_AFFINITY` | 1 | `vTaskCoreAffinitySet()` が使える |
| `configRUN_MULTIPLE_PRIORITIES` | 1 | 異なる優先度のタスクが同時に別コアで走る |
| `configUSE_PREEMPTION` / `configUSE_TIME_SLICING` | 1 / 1 | 同一優先度はティックごとにラウンドロビン |
| `configTICK_RATE_HZ` | 1000 | **1 tick = 1 ms**。`pdMS_TO_TICKS()` は使うが変換は等倍 |
| `configMAX_PRIORITIES` | 32 | 0（最低）〜31。31 はタイマータスクが使用 |
| `configMINIMAL_STACK_SIZE` | 512 **words**（=2KB） | スタック指定は**バイトでなくワード**数 |
| `configTOTAL_HEAP_SIZE` | 128 KB | heap_4 |
| 静的/動的確保 | 両方 1 | `xTaskCreate` / `xTaskCreateStatic` どちらも可 |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | 上書きパターン検査あり。フックは `src/system/freertos_hooks.c` |
| `configUSE_MALLOC_FAILED_HOOK` | 1 | ヒープ枯渇はフックで捕まる |
| `configUSE_TIMERS` | 1 | ソフトウェアタイマー有効 |
| `configUSE_QUEUE_SETS` | 1 | キューセット有効 |
| **無効** | `configUSE_IDLE_HOOK`=0 / `configUSE_TICK_HOOK`=0 / `configUSE_CO_ROUTINES`=0 / `configGENERATE_RUN_TIME_STATS`=0 / `configUSE_TICKLESS_IDLE`=0 | 対応 API は使えない |

> `configSUPPORT_PICO_SYNC_INTEROP` / `_TIME_INTEROP` は **0**。
> つまり pico-sdk の `sleep_ms()` / `mutex_t` / `sem_t` は **FreeRTOS を認識しない**。
> タスク内でのビジー待ちは `sleep_ms()` ではなく **`vTaskDelay()`** を使うこと。

---

## 1. タスク（`task.h`）

### 生成・削除

```c
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,   // void f(void*)
                       const char *pcName,
                       configSTACK_DEPTH_TYPE usStackDepth,  // ★ワード数
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask);        // 不要なら NULL
```

| API | 用途 | 備考 |
|---|---|---|
| `xTaskCreate()` | 動的生成 | 戻り値 `pdPASS` 以外はヒープ不足。**必ず判定**（本プロジェクトは `app_tasks` で生成） |
| `xTaskCreateStatic()` | 静的生成 | `StaticTask_t` と スタック配列を呼び側が持つ。戻り値はハンドル（失敗時 NULL） |
| `vTaskDelete(handle)` | 削除。`NULL` で自タスク | 削除タスクのスタック/TCB 解放は**アイドルタスクが行う**。アイドルが回らないと解放されない |
| `vTaskStartScheduler()` | スケジューラ開始 | **正常時は戻らない**。戻ってきたらヒープ不足（アイドル/タイマータスクが作れなかった） |

- タスク関数は **戻ってはいけない**。終わらせるなら末尾で `vTaskDelete(NULL)`。
- SMP では `xTaskCreateAffinitySet()` で生成と同時にコア固定もできる。

### 遅延・周期

| API | 用途 |
|---|---|
| `vTaskDelay(pdMS_TO_TICKS(500))` | **相対**待ち。処理時間分ずれるので周期用途には不適 |
| `xTaskDelayUntil(&xLastWake, period)` | **絶対**周期。`xLastWake = xTaskGetTickCount()` で初期化してループ前に置く。**周期タスクはこちら** |
| `taskYIELD()` | 同優先度に明け渡す |

> `vTaskDelayUntil()`（旧名・戻り値なし）は互換用（`configENABLE_BACKWARD_COMPATIBILITY 1` で生存）。
> 新規は `xTaskDelayUntil()` を使う（実際に遅延したかを `pdTRUE/pdFALSE` で返す）。

### 状態・優先度・情報取得

| API | 用途 |
|---|---|
| `vTaskSuspend(h)` / `vTaskResume(h)` / `xTaskResumeFromISR(h)` | 中断・再開 |
| `vTaskPrioritySet(h, p)` / `uxTaskPriorityGet(h)` | 優先度変更・取得（`h=NULL` は自分） |
| `xTaskGetCurrentTaskHandle()` / `xTaskGetHandle("name")` | ハンドル取得。名前引きは**遅い**ので初期化時のみ |
| `eTaskGetState(h)` | `eRunning/eReady/eBlocked/eSuspended/eDeleted` |
| `xTaskGetTickCount()` / `xTaskGetTickCountFromISR()` | 現在ティック |
| `xTaskGetSchedulerState()` | `taskSCHEDULER_NOT_STARTED/RUNNING/SUSPENDED`。**起動前後で挙動を変えたい初期化コードで有用** |
| `uxTaskGetStackHighWaterMark(h)` | スタック残り最小値（**ワード**）。スタックサイズ決定はこれで測る |
| `uxTaskGetNumberOfTasks()` / `uxTaskGetSystemState()` | 一覧取得（`configUSE_TRACE_FACILITY 1` なので可） |
| `xTaskAbortDelay(h)` | 他タスクのブロックを強制解除 |

### SMP（コア割り当て）

```c
vTaskCoreAffinitySet(handle, (1 << 0));            // core0 に固定
UBaseType_t mask = vTaskCoreAffinityGet(handle);
```

- 既定は「どちらのコアでも走る」。**cyw43（Wi-Fi/LED）や特定ペリフェラルを触るタスクはコア固定を検討**する。
- `configTICK_CORE 0`：ティック割り込みは core0。

---

## 2. タスク通知（`task.h`）— 最軽量の同期

1対1（受信側が1タスク固定）なら、キュー/セマフォより**速くRAMも食わない**。各タスクが 32bit の通知値を持つ。

| 用途 | 送信（タスク） | 送信（ISR） | 受信 |
|---|---|---|---|
| 二値/カウンティングセマフォ代替 | `xTaskNotifyGive(h)` | `vTaskNotifyGiveFromISR(h, &woken)` | `ulTaskNotifyTake(pdTRUE, timeout)` |
| 値/ビット渡し | `xTaskNotify(h, val, eAction)` | `xTaskNotifyFromISR(...)` | `xTaskNotifyWait(clearOnEntry, clearOnExit, &val, timeout)` |

- `eAction`: `eSetBits` / `eIncrement` / `eSetValueWithOverwrite` / `eSetValueWithoutOverwrite` / `eNoAction`。
- `ulTaskNotifyTake(pdTRUE, ...)` は「取ったら 0 クリア」＝二値セマフォ相当、`pdFALSE` はデクリメント＝カウンティング相当。
- **制約**: 受信できるのは通知先タスク1つだけ。ブロードキャストやキューイングは不可（溢れた値は落ちる）。

---

## 3. キュー（`queue.h`）— データを渡す

```c
QueueHandle_t q = xQueueCreate(len, sizeof(TempSample));  // ★値コピー
xQueueSend(q, &sample, pdMS_TO_TICKS(10));                // = xQueueSendToBack
xQueueReceive(q, &out, portMAX_DELAY);
```

| API | 備考 |
|---|---|
| `xQueueCreate` / `xQueueCreateStatic` | 生成。**NULL チェック必須** |
| `xQueueSend` / `xQueueSendToBack` / `xQueueSendToFront` | 満杯なら timeout まで待つ。戻り `errQUEUE_FULL` |
| `xQueueOverwrite(q, &v)` | 長さ1専用。常に成功（最新値レジスタ的な使い方） |
| `xQueueReceive` / `xQueuePeek` | 取り出す / 覗く |
| `xQueue*FromISR(..., &xHigherPriorityTaskWoken)` | ISR 版。**タイムアウト引数は無い** |
| `uxQueueMessagesWaiting` / `uxQueueSpacesAvailable` | 個数確認 |
| `vQueueDelete` / `xQueueReset` | 破棄・空に |
| `vQueueAddToRegistry(q, "name")` | デバッガ表示用（`configQUEUE_REGISTRY_SIZE 8`） |

- キューは**ポインタでなく実体をコピー**する。大きい構造体はポインタを送る設計も可だが、その場合**所有権とライフタイム**を決めること。
- **キューセット**（有効）: `xQueueCreateSet` / `xQueueAddToSet` / `xQueueSelectFromSet` で複数キューを1箇所で待てる。ただし可読性は落ちるので、まずタスク通知やメッセージバッファで足りないか考える。

---

## 4. セマフォ / ミューテックス（`semphr.h`）

| 生成 | 用途 |
|---|---|
| `xSemaphoreCreateBinary()` | イベント通知（ISR→タスク）。**タスク通知で代替できるならそちらが軽い** |
| `xSemaphoreCreateCounting(max, initial)` | 資源カウント |
| `xSemaphoreCreateMutex()` | 相互排他。**優先度継承あり** |
| `xSemaphoreCreateRecursiveMutex()` | 再帰ロック（`xSemaphoreTakeRecursive/GiveRecursive`） |

| 操作 | 備考 |
|---|---|
| `xSemaphoreTake(s, timeout)` | 失敗（`pdFALSE`）を**握り潰さない**。`portMAX_DELAY` は永久待ち＝デッドロック要因 |
| `xSemaphoreGive(s)` | |
| `xSemaphoreGiveFromISR(s, &woken)` | **ISR から Take は不可**。ミューテックスは ISR から一切触れない |
| `xSemaphoreGetMutexHolder(s)` | 保持タスク（デバッグ用） |

- **ミューテックスをイベント通知に使わない**（優先度継承の意味が壊れる）。逆にバイナリセマフォを排他に使わない（優先度逆転する）。
- SMP では「同一優先度だから排他される」は**成立しない**。共有データは必ず明示的に守る。

---

## 5. ストリーム / メッセージバッファ（`stream_buffer.h` / `message_buffer.h`）

**1対1（1writer/1reader）専用**。ロック無しで軽い。センサ→送信のようなパイプに向く。

| API | 備考 |
|---|---|
| `xStreamBufferCreate(size, triggerLevel)` | バイトストリーム。`triggerLevel` バイト溜まると受信側が起きる |
| `xStreamBufferSend/Receive(..., timeout)` | 部分送受信あり（返り値=実バイト数）。**要求量が届く保証はない** |
| `xMessageBufferCreate(size)` | 可変長メッセージを境界付きで。1メッセージあたり長さヘッダ（4B）を余分に消費 |
| `xMessageBufferSend/Receive` | メッセージ単位。**入り切らなければ 0 を返す**（部分送信しない） |
| `*FromISR` 版 | あり |

> **複数 writer / 複数 reader は不可**。その場合はキューを使う。

---

## 6. イベントグループ（`event_groups.h`）

複数条件の待ち合わせ（初期化完了フラグ、Wi-Fi 接続完了など）。24bit 分のフラグ。

| API | 備考 |
|---|---|
| `xEventGroupCreate()` | |
| `xEventGroupSetBits(g, bits)` / `xEventGroupSetBitsFromISR(g, bits, &woken)` | ISR 版は**タイマータスク経由**なので `configUSE_TIMERS` 必須（有効） |
| `xEventGroupWaitBits(g, bits, clearOnExit, waitForAll, timeout)` | `waitForAll=pdTRUE` で AND 待ち |
| `xEventGroupClearBits` / `xEventGroupGetBits` | |
| `xEventGroupSync(g, setBits, waitBits, timeout)` | ランデブー（全員揃うまで待つ） |

---

## 7. ソフトウェアタイマー（`timers.h`）

タイマーサービスタスク（優先度 31 / スタック 1024 words / キュー長 10）上で動く。

| API | 備考 |
|---|---|
| `xTimerCreate(name, period, autoReload, id, callback)` | 生成しただけでは動かない |
| `xTimerStart/Stop/Reset/ChangePeriod(..., ticksToWait)` | 実体は**コマンドキューへの送信**。キュー満杯なら失敗する（戻り値を見る） |
| `pvTimerGetTimerID(t)` | 複数タイマーで同一コールバックを共有する時の識別 |
| `xTimerPendFunctionCall` / `FromISR` | **ISR の処理をタスク文脈へ遅延実行**（deferred interrupt）。重い処理を ISR から追い出す定石 |

- **コールバック内でブロックするな**（`vTaskDelay` / 待ち付き Take は禁止）。タイマータスク全体が止まる。
- 優先度 31 なので、コールバックが重いと**アプリ最上位より優先**されてしまう点に注意。

---

## 8. 排他区間・スケジューラ制御

| API | 効果 | 使いどころ |
|---|---|---|
| `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` | 割り込み禁止＋SMP スピンロック | **極短時間**のみ。中で API 呼び出し・printf 禁止 |
| `taskENTER_CRITICAL_FROM_ISR()` / `taskEXIT_CRITICAL_FROM_ISR(x)` | ISR 版 | |
| `vTaskSuspendAll()` / `xTaskResumeAll()` | スケジューラ停止（割り込みは生きる） | 中で**ブロックする API を呼んではいけない** |
| `taskDISABLE_INTERRUPTS()` | 全割り込み禁止 | 復帰不能な致命エラー時のみ。`src/system/app_assert.c` が assert 失敗時に使用（printf → 割り込み禁止 → `__wfi()` ループ） |

- SMP では critical section は**両コアを止める**。長く握ると相方コアも道連れ。

---

## 9. ISR から呼べる API（`~FromISR`）の作法

```c
void isr(void) {
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(q, &data, &woken);
    portYIELD_FROM_ISR(woken);   // ★忘れると起床が最大1tick遅れる
}
```

- **`FromISR` が付かない API を ISR から呼ばない**（configASSERT で落ちる／落ちずに壊れる）。
- ISR 内で**タイムアウト待ちはできない**。
- 重い処理は `xTimerPendFunctionCallFromISR()` かタスク通知でタスク側へ逃がす。
- `configMAX_SYSCALL_INTERRUPT_PRIORITY 16`：これより**高優先度（数値が小さい）の割り込みからは FreeRTOS API を呼べない**。

---

## 10. ヒープ・診断

| API | 備考 |
|---|---|
| `pvPortMalloc` / `vPortFree` | heap_4。**タスク内の動的確保は原則避ける**（起動時に確保して以後固定が組み込みの定石） |
| `xPortGetFreeHeapSize()` / `xPortGetMinimumEverFreeHeapSize()` | 残量・最小残量 |
| `vApplicationStackOverflowHook(task, name)` | 実装済み（`freertos_hooks.c`）。**必ず出力して止める** |
| `vApplicationMallocFailedHook()` | 同上 |
| `vApplicationGetIdleTaskMemory` / `GetTimerTaskMemory` / `GetPassiveIdleTaskMemory` | **静的確保有効時は必須**。SMP なので Passive Idle（core1 用）版も要る |
| `configASSERT(x)` | 失敗箇所を printf する実装に差し替え済み |

---

## 11. ハマりどころ（このボード固有を含む）

1. **スタック指定はワード**。`512` は 2048 バイト。バイト感覚で `128` にすると即オーバーフロー。
2. **`vTaskStartScheduler()` の前にブロック系 API を呼ばない**。キュー/セマフォの「生成」は可、「待ち」は不可。
3. **タスク内で `sleep_ms()` を使わない**（pico-sdk interop が 0）。`vTaskDelay()` へ。
4. **SMP を忘れない**。「優先度で守られている」共有データは守られていない。
5. `printf`（USB CDC）は**遅く、内部でロックを取る**。critical section 内・ISR 内・タイマーコールバック内では呼ばない。
6. **cyw43（LED / Wi-Fi）は複数タスクから触らない**。触る所を1タスクに集約するかコア固定する。
7. 生成系（`xTaskCreate` / `xQueueCreate` / `xSemaphoreCreate*`）の**戻り値を必ず判定**する。起動時の静かな失敗が一番デバッグしづらい。
8. `portMAX_DELAY` の常用は「ハングの原因を見えなくする」。タイムアウト＋エラー処理を既定にする。

---

## 12. 用途からの逆引き

| やりたいこと | 第一候補 | 備考 |
|---|---|---|
| 一定周期で処理 | `xTaskDelayUntil` | `vTaskDelay` は周期がずれる |
| ISR → 特定タスクを起こす | タスク通知（`vTaskNotifyGiveFromISR`） | 最軽量 |
| センサ値をタスク間で流す（1→1） | メッセージバッファ or キュー | 型が固定ならキューが素直 |
| 複数タスク → 1タスクへ集約 | キュー | バッファ系は 1writer 制限 |
| 最新値だけ共有 | 長さ1キュー + `xQueueOverwrite` | ロック不要で読み書きできる |
| 共有リソースの排他 | ミューテックス | 短時間なら critical section |
| 複数の初期化完了を待つ | イベントグループ | AND 待ちができる |
| ISR の重い後処理 | `xTimerPendFunctionCallFromISR` | deferred interrupt |
| スタックサイズを決める | `uxTaskGetStackHighWaterMark` | 実測して詰める |
