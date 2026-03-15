# FreeRTOS ITRON風ラッパー API仕様書

**対象ファイル:** `src/kernel/freertos_wrapper.h` / `src/kernel/freertos_wrapper.c`
**対象環境:** Raspberry Pi Pico 2W (RP2350 / Cortex-M33) + FreeRTOS SMP
**準拠仕様:** μITRON4.0 仕様書 (部分準拠)

---

## 目次

1. [概要](#1-概要)
2. [基本型](#2-基本型)
3. [定数](#3-定数)
4. [生成情報構造体](#4-生成情報構造体)
5. [静的確保ヘルパーマクロ](#5-静的確保ヘルパーマクロ)
6. [API仕様](#6-api仕様)
7. [内部動作](#7-内部動作)
8. [FreeRTOS対応表](#8-freertos対応表)
9. [制約・注意事項](#9-制約注意事項)

---

## 1. 概要

FreeRTOS の各種API（タスク・セマフォ・ミューテックス・イベントフラグ・ソフトウェアタイマ）を μITRON4.0 風にラップする。

### 設計方針

- **ID管理:** 各オブジェクトに整数IDを割り当て、内部でFreeRTOSハンドルに変換する
- **静的/動的の透過的切り替え:** `ATR` フィールドの `TA_STATIC` / `TA_NULL` で切り替え。インタフェースは共通
- **静的確保の簡略化:** `DEF_C***` マクロにより、スタック配列・制御ブロックの宣言を1行に集約
- **ISR対応:** `sig_sem` / `set_flg` / `wup_tsk` はISR内からの呼び出しを自動判定して `FromISR` 系に切り替える

### 必要な FreeRTOSConfig.h 設定

```c
#define configSUPPORT_STATIC_ALLOCATION  1  // DEF_C*** マクロ使用時に必要
#define configSUPPORT_DYNAMIC_ALLOCATION 1  // TA_NULL 使用時に必要
#define configUSE_MUTEXES                1
#define configUSE_COUNTING_SEMAPHORES    1
#define configUSE_TIMERS                 1  // set_flg のISR対応にも必要
```

---

## 2. 基本型

```c
typedef int          ER;      // エラーコード
typedef int          ID;      // オブジェクトID (1オリジン、0は無効)
typedef unsigned int ATR;     // オブジェクト属性ビットマスク
typedef int          PRI;     // タスク優先度
typedef unsigned int SIZE;    // メモリサイズ (バイト単位)
typedef int          TMO;     // タイムアウト値 (ms または特殊値)
typedef unsigned int FLGPTN;  // イベントフラグ ビットパターン
typedef unsigned int MODE;    // 待ちモード
typedef void (*TASK)(void *exinf);  // タスク関数ポインタ型
```

`TASK` 型は FreeRTOS の `TaskFunction_t` (`void (*)(void*)`) と完全互換。

---

## 3. 定数

### タイムアウト特殊値

| 定数 | 値 | 意味 |
|------|----|------|
| `TMO_POL` | `0` | ポーリング。待たずに即時返却 |
| `TMO_FEVR` | `-1` | 永久待ち。条件成立まで無限に待機 |

### エラーコード

| 定数 | 値 | 発生条件 |
|------|----|---------|
| `E_OK` | `0` | 正常終了 |
| `E_NOMEM` | `-5` | メモリ確保失敗（ヒープ枯渇または静的バッファNULL） |
| `E_ID` | `-18` | IDが範囲外、または未登録オブジェクトへのアクセス |
| `E_CTX` | `-25` | ISR内からの呼び出し禁止APIの呼び出し、またはスケジューラ未起動 |
| `E_TMOUT` | `-50` | タイムアウトによる待ち解除 |

### オブジェクト属性ビット

| 定数 | 値 | 意味 |
|------|----|------|
| `TA_NULL` | `0x00` | 動的確保（FreeRTOSヒープから割り当て） |
| `TA_STATIC` | `0x80` | 静的確保（呼び出し側またはマクロがバッファを用意） |

### イベントフラグ 待ちモード

`wai_flg` の `wfmode` 引数に指定する。ビットORで組み合わせ可能。

| 定数 | 値 | 意味 |
|------|----|------|
| `TWF_ANDW` | `0x00` | AND待ち。`waiptn` の全ビットが成立で解除 |
| `TWF_ORW` | `0x01` | OR待ち。`waiptn` のいずれか1ビットが成立で解除 |
| `TWF_CLR` | `0x10` | 待ち解除後、成立ビットを自動クリア |

### ハンドルテーブル上限

| 定数 | 値 | 対象 |
|------|----|------|
| `MAX_TASK_ID` | `8` | タスク |
| `MAX_SEM_ID` | `8` | セマフォ |
| `MAX_MTX_ID` | `8` | ミューテックス |
| `MAX_FLG_ID` | `8` | イベントフラグ |
| `MAX_CYC_ID` | `8` | 周期ハンドラ |

有効なIDの範囲は `1` 〜 `MAX_*_ID`。

---

## 4. 生成情報構造体

### `T_CTSK` — タスク

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `tskatr` | `ATR` | `TA_NULL` または `TA_STATIC` |
| `exinf` | `void *` | タスク引数（`pvParameters` 相当） |
| `task` | `TASK` | タスク関数ポインタ |
| `itskpri` | `PRI` | 初期優先度（1 〜 `configMAX_PRIORITIES-1`、範囲外は自動クランプ） |
| `stksz` | `SIZE` | スタックサイズ **[バイト単位]**（内部でワード数に変換） |
| `name` | `const char *` | タスク名（デバッグ用。`NULL` の場合 `"task"` を使用） |
| `stk` | `StackType_t *` | **[`TA_STATIC` のみ]** スタック領域へのポインタ |
| `tcb` | `StaticTask_t *` | **[`TA_STATIC` のみ]** TCB領域へのポインタ |

### `T_CSEM` — セマフォ

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `sematr` | `ATR` | `TA_NULL` または `TA_STATIC` |
| `isemcnt` | `int` | 初期カウント値 |
| `maxsem` | `int` | 最大カウント値 |
| `scb` | `StaticSemaphore_t *` | **[`TA_STATIC` のみ]** |

### `T_CMTX` — ミューテックス

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `mtxatr` | `ATR` | `TA_NULL` または `TA_STATIC` |
| `mcb` | `StaticSemaphore_t *` | **[`TA_STATIC` のみ]** |

### `T_CFLG` — イベントフラグ

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `flgatr` | `ATR` | `TA_NULL` または `TA_STATIC` |
| `iflgptn` | `FLGPTN` | 初期ビットパターン（`0` で全ビットOFF） |
| `ecb` | `StaticEventGroup_t *` | **[`TA_STATIC` のみ]** |

### `T_CCYC` — 周期ハンドラ

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `cycatr` | `ATR` | `TA_NULL` または `TA_STATIC` |
| `cychdr` | `void (*)(ID cycid)` | コールバック関数 |
| `cyctim` | `TMO` | 周期 [ms] |
| `name` | `const char *` | タイマ名（`NULL` の場合 `"cyc"` を使用） |
| `tcb` | `StaticTimer_t *` | **[`TA_STATIC` のみ]** |

---

## 5. 静的確保ヘルパーマクロ

μITRON のコンフィギュレータに相当する機能をプリプロセッサマクロで実現する。
スタック配列・制御ブロック（TCB/SCB等）の宣言と `T_C***` 構造体の初期化を1行で記述できる。

> **前提条件:** `configSUPPORT_STATIC_ALLOCATION 1` が必要。

---

### `DEF_CTSK` — タスク

```c
DEF_CTSK(_var, _func, _pri, _words);
```

| 引数 | 説明 |
|------|------|
| `_var` | 生成する `T_CTSK` 変数名 |
| `_func` | タスク関数（`TASK` 型） |
| `_pri` | 初期優先度 |
| `_words` | スタックサイズ **[ワード数 (`StackType_t` 単位)]** |

**展開内容:**

```c
static StackType_t  _stk_<_var>[_words];
static StaticTask_t _tcb_<_var>;
static const T_CTSK _var = {
    .tskatr  = TA_STATIC,
    .exinf   = NULL,
    .task    = _func,
    .itskpri = _pri,
    .stksz   = sizeof(_stk_<_var>),  // バイト単位
    .name    = "<_var>",
    .stk     = _stk_<_var>,
    .tcb     = &_tcb_<_var>,
};
```

**使用例:**

```c
// ファイルスコープで宣言
DEF_CTSK(led_ctsk, led_task, 2, 512);

// 使用
cre_tsk(1, &led_ctsk);
```

---

### `DEF_CSEM` — セマフォ

```c
DEF_CSEM(_var, _init, _max);
```

| 引数 | 説明 |
|------|------|
| `_var` | 生成する `T_CSEM` 変数名 |
| `_init` | 初期カウント値 |
| `_max` | 最大カウント値 |

**使用例:**

```c
DEF_CSEM(uart_sem, 0, 1);  // バイナリセマフォ相当

cre_sem(1, &uart_sem);
```

---

### `DEF_CMTX` — ミューテックス

```c
DEF_CMTX(_var);
```

| 引数 | 説明 |
|------|------|
| `_var` | 生成する `T_CMTX` 変数名 |

**使用例:**

```c
DEF_CMTX(spi_mtx);

cre_mtx(1, &spi_mtx);
```

---

### `DEF_CFLG` — イベントフラグ

```c
DEF_CFLG(_var, _initptn);
```

| 引数 | 説明 |
|------|------|
| `_var` | 生成する `T_CFLG` 変数名 |
| `_initptn` | 初期ビットパターン（`0` で全ビットOFF） |

**使用例:**

```c
DEF_CFLG(sensor_flg, 0x00);

cre_flg(1, &sensor_flg);
```

---

### `DEF_CCYC` — 周期ハンドラ

```c
DEF_CCYC(_var, _handler, _period_ms);
```

| 引数 | 説明 |
|------|------|
| `_var` | 生成する `T_CCYC` 変数名 |
| `_handler` | コールバック関数（`void (*)(ID cycid)` 型） |
| `_period_ms` | 周期 [ms] |

**使用例:**

```c
void blink_handler(ID cycid) { /* ... */ }

DEF_CCYC(blink_cyc, blink_handler, 500);

cre_cyc(1, &blink_cyc);
sta_cyc(1);
```

---

## 6. API仕様

### 6.1 タスク管理

#### `cre_tsk` — タスク生成

```c
ER cre_tsk(ID tskid, const T_CTSK *pk_ctsk);
```

生成と同時に Runnable 状態になる（μITRON の `act_tsk` を兼ねる）。
FreeRTOS の `xTaskCreate` / `xTaskCreateStatic` の両方に対応。

| 引数/返値 | 説明 |
|-----------|------|
| `tskid` | 割り当てるID（1 〜 `MAX_TASK_ID`） |
| `pk_ctsk` | タスク生成情報へのポインタ |
| `E_OK` | 正常生成 |
| `E_ID` | IDが範囲外またはNULLポインタ |
| `E_NOMEM` | メモリ確保失敗 |

**優先度クランプ:** `itskpri < 1` の場合は `1` に、`itskpri >= configMAX_PRIORITIES` の場合は `configMAX_PRIORITIES - 1` に自動調整される。

**スタックサイズ変換:** `stksz` はバイト単位で渡す。内部で `stksz / sizeof(StackType_t)` によりワード数に変換される。`0` を指定した場合は `configMINIMAL_STACK_SIZE` が使用される。

---

#### `del_tsk` — タスク削除

```c
ER del_tsk(ID tskid);
```

ハンドルテーブルから削除し、`vTaskDelete` を呼び出す。
**自タスクの削除には使用しないこと。** 自タスクを終了する場合は `vTaskDelete(NULL)` を直接呼ぶ。

---

#### `dly_tsk` — 相対時間待ち

```c
ER dly_tsk(TMO dlytim);
```

| `dlytim` の値 | 動作 |
|--------------|------|
| `TMO_POL` / `0` | `taskYIELD()` のみ（CPUを譲るが即座に戻る） |
| `TMO_FEVR` | `portMAX_DELAY` で永久待ち |
| ms値 | `vTaskDelay(pdMS_TO_TICKS(dlytim))` |

ISR内またはスケジューラ未起動時は `E_CTX` を返す。

---

#### `slp_tsk` — タスク起床待ち

```c
ER slp_tsk(TMO tmout);
```

FreeRTOS タスク通知（`ulTaskNotifyTake`）を使用して `wup_tsk` からの起床を待つ。
ISR内またはスケジューラ未起動時は `E_CTX` を返す。

| 返値 | 条件 |
|------|------|
| `E_OK` | `wup_tsk` により起床 |
| `E_TMOUT` | タイムアウト |
| `E_CTX` | ISR内またはスケジューラ未起動 |

---

#### `wup_tsk` — タスク起床

```c
ER wup_tsk(ID tskid);
```

`slp_tsk` で待機中のタスクを起こす。**ISR内から呼び出し可。**
内部でコンテキストを自動判定し、`xTaskNotifyGive` / `vTaskNotifyGiveFromISR` を使い分ける。

---

### 6.2 セマフォ

| API | 説明 | ISR対応 |
|-----|------|---------|
| `cre_sem(ID, const T_CSEM*)` | カウンティングセマフォ生成 | — |
| `del_sem(ID)` | セマフォ削除 | — |
| `wai_sem(ID, TMO)` | セマフォ獲得（タイムアウト指定） | 不可 |
| `pol_sem(ID)` | `wai_sem(id, TMO_POL)` の糖衣（inline） | 不可 |
| `sig_sem(ID)` | セマフォ返却 | **可** |

`sig_sem` はISR内では `xSemaphoreGiveFromISR` + `portYIELD_FROM_ISR` を使用する。

---

### 6.3 ミューテックス

| API | 説明 | ISR対応 |
|-----|------|---------|
| `cre_mtx(ID, const T_CMTX*)` | ミューテックス生成（優先度継承付き） | — |
| `del_mtx(ID)` | ミューテックス削除 | — |
| `loc_mtx(ID, TMO)` | ミューテックス獲得（タイムアウト指定） | 不可 |
| `unl_mtx(ID)` | ミューテックス解放 | 不可 |

`unl_mtx` をロックしていないタスクから呼び出した場合は `E_CTX` を返す。

---

### 6.4 イベントフラグ

| API | 説明 | ISR対応 |
|-----|------|---------|
| `cre_flg(ID, const T_CFLG*)` | イベントフラグ生成 | — |
| `del_flg(ID)` | イベントフラグ削除 | — |
| `set_flg(ID, FLGPTN)` | ビットセット | **可** |
| `clr_flg(ID, FLGPTN)` | ビットクリア | 不可 |
| `wai_flg(ID, FLGPTN, MODE, FLGPTN*, TMO)` | ビットパターン待ち | 不可 |

`cre_flg` で `iflgptn != 0` を指定した場合、生成直後に `xEventGroupSetBits` で初期値をセットする。

#### `wai_flg` 詳細

```c
ER wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout);
```

| 引数 | 説明 |
|------|------|
| `waiptn` | 待ちビットパターン |
| `wfmode` | `TWF_ANDW` / `TWF_ORW` \| `TWF_CLR`（ORで組み合わせ） |
| `p_flgptn` | 解除時点のビットパターンを格納する変数へのポインタ（`NULL` 不可） |
| `tmout` | `TMO_POL` / `TMO_FEVR` / ms値 |

`xEventGroupWaitBits` はタイムアウト時も現在値を返すため、返値は `waiptn` との照合で判定する。

---

### 6.5 周期ハンドラ

| API | 説明 |
|-----|------|
| `cre_cyc(ID, const T_CCYC*)` | 周期ハンドラ生成（**停止状態**で作成） |
| `del_cyc(ID)` | 周期ハンドラ削除（`portMAX_DELAY` 待ちで停止確認後に削除） |
| `sta_cyc(ID)` | 周期ハンドラ開始 |
| `stp_cyc(ID)` | 周期ハンドラ停止 |

コールバック関数のシグネチャ:

```c
void my_handler(ID cycid);
```

FreeRTOS ソフトウェアタイマの自動リロードモードで動作する。`pvTimerID` に `cycid` を埋め込み、内部の共通コールバック `prv_cyc_callback` 経由でハンドラを呼び出す。

---

## 7. 内部動作

### ハンドルテーブル

各オブジェクト種別ごとに静的配列でIDとFreeRTOSハンドルを対応付ける。

```
s_task_tbl[MAX_TASK_ID + 1]  : TaskHandle_t
s_sem_tbl [MAX_SEM_ID  + 1]  : SemaphoreHandle_t
s_mtx_tbl [MAX_MTX_ID  + 1]  : SemaphoreHandle_t
s_flg_tbl [MAX_FLG_ID  + 1]  : EventGroupHandle_t
s_cyc_tbl [MAX_CYC_ID  + 1]  : TimerHandle_t
```

インデックス `0` は未使用（IDは1オリジン）。テーブルへのアクセスは `taskENTER_CRITICAL` / `taskEXIT_CRITICAL` で保護する（RP2350のSMPではスピンロックとして動作）。

### タイムアウト変換

```
TMO_FEVR (-1) → portMAX_DELAY
TMO_POL  ( 0) → 0 (TickType_t)
ms値          → pdMS_TO_TICKS(tmout)
```

### ISRコンテキスト判定

Cortex-M の `IPSR` (Interrupt Program Status Register) を `MRS` 命令で読み出す。非ゼロであれば割り込みコンテキストと判定する。RP2350固有のAPIに依存しないため移植性が高い。

```c
uint32_t ipsr;
__asm volatile("mrs %0, ipsr" : "=r"(ipsr));
// ipsr != 0 → ISR内
```

---

## 8. FreeRTOS対応表

| ラッパーAPI | FreeRTOS API（動的） | FreeRTOS API（静的） |
|------------|---------------------|---------------------|
| `cre_tsk` | `xTaskCreate` | `xTaskCreateStatic` |
| `del_tsk` | `vTaskDelete` | 同左 |
| `dly_tsk` | `vTaskDelay` | 同左 |
| `slp_tsk` | `ulTaskNotifyTake` | 同左 |
| `wup_tsk` | `xTaskNotifyGive` / `FromISR` | 同左 |
| `cre_sem` | `xSemaphoreCreateCounting` | `xSemaphoreCreateCountingStatic` |
| `wai_sem` | `xSemaphoreTake` | 同左 |
| `sig_sem` | `xSemaphoreGive` / `FromISR` | 同左 |
| `cre_mtx` | `xSemaphoreCreateMutex` | `xSemaphoreCreateMutexStatic` |
| `loc_mtx` | `xSemaphoreTake` | 同左 |
| `unl_mtx` | `xSemaphoreGive` | 同左 |
| `cre_flg` | `xEventGroupCreate` | `xEventGroupCreateStatic` |
| `set_flg` | `xEventGroupSetBits` / `FromISR` | 同左 |
| `clr_flg` | `xEventGroupClearBits` | 同左 |
| `wai_flg` | `xEventGroupWaitBits` | 同左 |
| `cre_cyc` | `xTimerCreate` | `xTimerCreateStatic` |
| `sta_cyc` | `xTimerStart` | 同左 |
| `stp_cyc` | `xTimerStop` | 同左 |
| `del_cyc` | `xTimerDelete` | 同左 |

---

## 9. 制約・注意事項

### スコープ制約

| API | ISR内 | スケジューラ未起動 |
|-----|-------|-----------------|
| `cre_tsk` | 不可 | **可** |
| `dly_tsk` / `slp_tsk` | 不可 → `E_CTX` | 不可 → `E_CTX` |
| `wup_tsk` / `sig_sem` / `set_flg` | **可** | — |
| `wai_sem` / `loc_mtx` / `wai_flg` | 不可 | 不可 |

### 優先度制約

優先度 `0` (`tskIDLE_PRIORITY`) への設定は禁止。`cre_tsk` 内で `1` に自動クランプされる。

### ミューテックスのオーナーシップ

`unl_mtx` はロックしたタスクのみが呼び出せる。オーナー以外から呼んだ場合は `E_CTX` を返す（FreeRTOS の `xSemaphoreGive` が `pdFALSE` を返すことで検出）。

### `set_flg` のISR対応要件

ISR内からの `set_flg` は `xEventGroupSetBitsFromISR` を使用する。この関数はタイマデーモンタスクへメッセージを投げる実装のため、`configUSE_TIMERS 1` が必須。

### 自タスクの終了

`del_tsk` は他タスクの削除に使用する。自タスクを終了する場合は直接 `vTaskDelete(NULL)` を呼ぶこと。

### `DEF_C***` マクロのスコープ

`DEF_C***` マクロはファイルスコープ（関数外）で使用すること。関数内で使用すると `static` 変数がローカルスコープに閉じてしまい、再入時の動作が不定になる。

```c
// NG: 関数内での使用
void init(void) {
    DEF_CTSK(my_task, task_func, 2, 512);  // static がローカルスコープになる
}

// OK: ファイルスコープでの使用
DEF_CTSK(my_task, task_func, 2, 512);
void init(void) {
    cre_tsk(1, &my_task);
}
```
