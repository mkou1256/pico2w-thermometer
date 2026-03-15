#include "freertos_wrapper.h"

/* ========================================================================
 * 内部ハンドルテーブル
 * インデックス 0 は未使用 (IDは1オリジン)
 * ======================================================================== */

static TaskHandle_t       s_task_tbl[MAX_TASK_ID + 1];
static SemaphoreHandle_t  s_sem_tbl[MAX_SEM_ID  + 1];
static SemaphoreHandle_t  s_mtx_tbl[MAX_MTX_ID  + 1];
static EventGroupHandle_t s_flg_tbl[MAX_FLG_ID  + 1];
static TimerHandle_t      s_cyc_tbl[MAX_CYC_ID  + 1];

/* 周期ハンドラのコールバック保存 (pvTimerID だけでは型安全に呼べないため) */
static void (*s_cyc_hdr_tbl[MAX_CYC_ID + 1])(ID cycid);

/* ========================================================================
 * 内部ユーティリティ
 * ======================================================================== */

/* タイムアウト値 → FreeRTOS TickType_t 変換 */
static TickType_t prv_tmo_to_ticks(TMO tmout)
{
    if (tmout == TMO_FEVR) return portMAX_DELAY;
    if (tmout == TMO_POL)  return 0;
    return pdMS_TO_TICKS((uint32_t)tmout);
}

/*
 * 割り込みコンテキスト判定 (Cortex-M 共通)
 * IPSR (Interrupt Program Status Register) が非ゼロなら ISR 内
 */
static BaseType_t prv_is_in_isr(void)
{
    uint32_t ipsr;
    __asm volatile("mrs %0, ipsr" : "=r"(ipsr));
    return (ipsr != 0u) ? pdTRUE : pdFALSE;
}

/* ========================================================================
 * タスク管理
 * ======================================================================== */

ER cre_tsk(ID tskid, const T_CTSK *pk_ctsk)
{
    if (tskid < 1 || tskid > MAX_TASK_ID) return E_ID;
    if (pk_ctsk == NULL || pk_ctsk->task == NULL) return E_ID;
    if (s_task_tbl[tskid] != NULL) return E_OBJ;

    /* 優先度をクランプ (0 はアイドルタスクと競合するため禁止) */
    PRI pri = pk_ctsk->itskpri;
    if (pri < 1) pri = 1;
    if (pri >= configMAX_PRIORITIES) pri = configMAX_PRIORITIES - 1;

    /* スタックサイズ: バイト → ワード数変換 */
    uint32_t depth = pk_ctsk->stksz / sizeof(StackType_t);
    if (depth == 0u) depth = configMINIMAL_STACK_SIZE;

    const char *name = (pk_ctsk->name != NULL) ? pk_ctsk->name : "task";

    TaskHandle_t handle = NULL;
    BaseType_t   ret;

    if (pk_ctsk->tskatr & TA_STATIC) {
        /* 静的確保: stk / tcb は呼び出し側で用意 */
        handle = xTaskCreateStatic(
            pk_ctsk->task,
            name,
            depth,
            pk_ctsk->exinf,
            (UBaseType_t)pri,
            pk_ctsk->stk,
            pk_ctsk->tcb
        );
        ret = (handle != NULL) ? pdPASS : errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    } else {
        /* 動的確保: FreeRTOS ヒープから割り当て */
        ret = xTaskCreate(
            pk_ctsk->task,
            name,
            (configSTACK_DEPTH_TYPE)depth,
            pk_ctsk->exinf,
            (UBaseType_t)pri,
            &handle
        );
    }

    if (ret == pdPASS) {
        taskENTER_CRITICAL();
        s_task_tbl[tskid] = handle;
        taskEXIT_CRITICAL();
    }

    return (ret == pdPASS) ? E_OK : E_NOMEM;
}

ER del_tsk(ID tskid)
{
    if (tskid < 1 || tskid > MAX_TASK_ID) return E_ID;

    taskENTER_CRITICAL();
    TaskHandle_t h = s_task_tbl[tskid];
    s_task_tbl[tskid] = NULL;
    taskEXIT_CRITICAL();

    if (h == NULL) return E_ID;
    vTaskDelete(h);
    return E_OK;
}

ER dly_tsk(TMO dlytim)
{
    if (prv_is_in_isr()) return E_CTX;
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return E_CTX;

    if (dlytim == TMO_POL || dlytim == 0) {
        taskYIELD();
    } else {
        TickType_t ticks = (dlytim == TMO_FEVR)
            ? portMAX_DELAY
            : pdMS_TO_TICKS((uint32_t)dlytim);
        vTaskDelay(ticks);
    }
    return E_OK;
}

ER slp_tsk(TMO tmout)
{
    if (prv_is_in_isr()) return E_CTX;
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return E_CTX;

    uint32_t notif = ulTaskNotifyTake(pdTRUE, prv_tmo_to_ticks(tmout));
    return (notif > 0u) ? E_OK : E_TMOUT;
}

ER wup_tsk(ID tskid)
{
    if (tskid < 1 || tskid > MAX_TASK_ID) return E_ID;
    TaskHandle_t h = s_task_tbl[tskid];
    if (h == NULL) return E_ID;

    if (prv_is_in_isr()) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(h, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xTaskNotifyGive(h);
    }
    return E_OK;
}

/* ========================================================================
 * セマフォ
 * ======================================================================== */

ER cre_sem(ID semid, const T_CSEM *pk_csem)
{
    if (semid < 1 || semid > MAX_SEM_ID) return E_ID;
    if (pk_csem == NULL) return E_ID;
    if (s_sem_tbl[semid] != NULL) return E_OBJ;

    SemaphoreHandle_t h;
    if (pk_csem->sematr & TA_STATIC) {
        h = xSemaphoreCreateCountingStatic(
            (UBaseType_t)pk_csem->maxsem,
            (UBaseType_t)pk_csem->isemcnt,
            pk_csem->scb
        );
    } else {
        h = xSemaphoreCreateCounting(
            (UBaseType_t)pk_csem->maxsem,
            (UBaseType_t)pk_csem->isemcnt
        );
    }

    if (h == NULL) return E_NOMEM;

    taskENTER_CRITICAL();
    s_sem_tbl[semid] = h;
    taskEXIT_CRITICAL();

    return E_OK;
}

ER del_sem(ID semid)
{
    if (semid < 1 || semid > MAX_SEM_ID) return E_ID;

    taskENTER_CRITICAL();
    SemaphoreHandle_t h = s_sem_tbl[semid];
    s_sem_tbl[semid] = NULL;
    taskEXIT_CRITICAL();

    if (h == NULL) return E_ID;
    vSemaphoreDelete(h);
    return E_OK;
}

ER wai_sem(ID semid, TMO tmout)
{
    if (semid < 1 || semid > MAX_SEM_ID) return E_ID;
    SemaphoreHandle_t h = s_sem_tbl[semid];
    if (h == NULL) return E_ID;

    BaseType_t ret = xSemaphoreTake(h, prv_tmo_to_ticks(tmout));
    return (ret == pdTRUE) ? E_OK : E_TMOUT;
}

ER sig_sem(ID semid)
{
    if (semid < 1 || semid > MAX_SEM_ID) return E_ID;
    SemaphoreHandle_t h = s_sem_tbl[semid];
    if (h == NULL) return E_ID;

    if (prv_is_in_isr()) {
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(h, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xSemaphoreGive(h);
    }
    return E_OK;
}

/* ========================================================================
 * ミューテックス
 * ======================================================================== */

ER cre_mtx(ID mtxid, const T_CMTX *pk_cmtx)
{
    if (mtxid < 1 || mtxid > MAX_MTX_ID) return E_ID;
    if (pk_cmtx == NULL) return E_ID;
    if (s_mtx_tbl[mtxid] != NULL) return E_OBJ;

    SemaphoreHandle_t h;
    if (pk_cmtx->mtxatr & TA_STATIC) {
        h = xSemaphoreCreateMutexStatic(pk_cmtx->mcb);
    } else {
        h = xSemaphoreCreateMutex();
    }

    if (h == NULL) return E_NOMEM;

    taskENTER_CRITICAL();
    s_mtx_tbl[mtxid] = h;
    taskEXIT_CRITICAL();

    return E_OK;
}

ER del_mtx(ID mtxid)
{
    if (mtxid < 1 || mtxid > MAX_MTX_ID) return E_ID;

    taskENTER_CRITICAL();
    SemaphoreHandle_t h = s_mtx_tbl[mtxid];
    s_mtx_tbl[mtxid] = NULL;
    taskEXIT_CRITICAL();

    if (h == NULL) return E_ID;
    vSemaphoreDelete(h);
    return E_OK;
}

ER loc_mtx(ID mtxid, TMO tmout)
{
    if (mtxid < 1 || mtxid > MAX_MTX_ID) return E_ID;
    SemaphoreHandle_t h = s_mtx_tbl[mtxid];
    if (h == NULL) return E_ID;

    BaseType_t ret = xSemaphoreTake(h, prv_tmo_to_ticks(tmout));
    return (ret == pdTRUE) ? E_OK : E_TMOUT;
}

ER unl_mtx(ID mtxid)
{
    if (mtxid < 1 || mtxid > MAX_MTX_ID) return E_ID;
    SemaphoreHandle_t h = s_mtx_tbl[mtxid];
    if (h == NULL) return E_ID;

    /* オーナー以外が呼んだ場合 xSemaphoreGive は pdFALSE を返す */
    BaseType_t ret = xSemaphoreGive(h);
    return (ret == pdTRUE) ? E_OK : E_CTX;
}

/* ========================================================================
 * イベントフラグ
 * ======================================================================== */

ER cre_flg(ID flgid, const T_CFLG *pk_cflg)
{
    if (flgid < 1 || flgid > MAX_FLG_ID) return E_ID;
    if (pk_cflg == NULL) return E_ID;
    if (s_flg_tbl[flgid] != NULL) return E_OBJ;

    EventGroupHandle_t h;
    if (pk_cflg->flgatr & TA_STATIC) {
        h = xEventGroupCreateStatic(pk_cflg->ecb);
    } else {
        h = xEventGroupCreate();
    }

    if (h == NULL) return E_NOMEM;

    /* xEventGroupCreate は初期値 0 固定のため、初期パターンを別途セット */
    if (pk_cflg->iflgptn != 0u) {
        xEventGroupSetBits(h, (EventBits_t)pk_cflg->iflgptn);
    }

    taskENTER_CRITICAL();
    s_flg_tbl[flgid] = h;
    taskEXIT_CRITICAL();

    return E_OK;
}

ER del_flg(ID flgid)
{
    if (flgid < 1 || flgid > MAX_FLG_ID) return E_ID;

    taskENTER_CRITICAL();
    EventGroupHandle_t h = s_flg_tbl[flgid];
    s_flg_tbl[flgid] = NULL;
    taskEXIT_CRITICAL();

    if (h == NULL) return E_ID;
    vEventGroupDelete(h);
    return E_OK;
}

ER set_flg(ID flgid, FLGPTN setptn)
{
    if (flgid < 1 || flgid > MAX_FLG_ID) return E_ID;
    EventGroupHandle_t h = s_flg_tbl[flgid];
    if (h == NULL) return E_ID;

    if (prv_is_in_isr()) {
        /*
         * ISR 内では xEventGroupSetBitsFromISR を使用。
         * 内部でタイマデーモンへメッセージを投げるため configUSE_TIMERS=1 が必要。
         */
        BaseType_t woken = pdFALSE;
        xEventGroupSetBitsFromISR(h, (EventBits_t)setptn, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xEventGroupSetBits(h, (EventBits_t)setptn);
    }
    return E_OK;
}

ER clr_flg(ID flgid, FLGPTN clrptn)
{
    if (flgid < 1 || flgid > MAX_FLG_ID) return E_ID;
    EventGroupHandle_t h = s_flg_tbl[flgid];
    if (h == NULL) return E_ID;

    xEventGroupClearBits(h, (EventBits_t)clrptn);
    return E_OK;
}

ER wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout)
{
    if (prv_is_in_isr()) return E_CTX;
    if (flgid < 1 || flgid > MAX_FLG_ID) return E_ID;
    if (p_flgptn == NULL) return E_ID;
    EventGroupHandle_t h = s_flg_tbl[flgid];
    if (h == NULL) return E_ID;

    BaseType_t wait_all   = (wfmode & TWF_ORW) ? pdFALSE : pdTRUE;
    BaseType_t clr_on_exit = (wfmode & TWF_CLR) ? pdTRUE  : pdFALSE;

    EventBits_t bits = xEventGroupWaitBits(
        h,
        (EventBits_t)waiptn,
        clr_on_exit,
        wait_all,
        prv_tmo_to_ticks(tmout)
    );

    *p_flgptn = (FLGPTN)bits;

    /* 条件成立チェック (xEventGroupWaitBits はタイムアウト時も現在値を返す) */
    if (wait_all) {
        return ((bits & (EventBits_t)waiptn) == (EventBits_t)waiptn) ? E_OK : E_TMOUT;
    } else {
        return ((bits & (EventBits_t)waiptn) != 0u) ? E_OK : E_TMOUT;
    }
}

/* ========================================================================
 * 周期ハンドラ (ソフトウェアタイマ)
 * ======================================================================== */

/* FreeRTOS タイマコールバック共通ルーティン */
static void prv_cyc_callback(TimerHandle_t xTimer)
{
    /* pvTimerID に cycid を格納してある */
    ID cycid = (ID)(uintptr_t)pvTimerGetTimerID(xTimer);
    if (cycid >= 1 && cycid <= MAX_CYC_ID && s_cyc_hdr_tbl[cycid] != NULL) {
        s_cyc_hdr_tbl[cycid](cycid);
    }
}

ER cre_cyc(ID cycid, const T_CCYC *pk_ccyc)
{
    if (cycid < 1 || cycid > MAX_CYC_ID) return E_ID;
    if (pk_ccyc == NULL || pk_ccyc->cychdr == NULL) return E_ID;
    if (s_cyc_tbl[cycid] != NULL) return E_OBJ;

    TickType_t  period = pdMS_TO_TICKS((uint32_t)pk_ccyc->cyctim);
    const char *name   = (pk_ccyc->name != NULL) ? pk_ccyc->name : "cyc";
    void       *timer_id = (void *)(uintptr_t)cycid;

    TimerHandle_t h;
    if (pk_ccyc->cycatr & TA_STATIC) {
        h = xTimerCreateStatic(
            name,
            period,
            pdTRUE,        /* 自動リロード (周期動作) */
            timer_id,
            prv_cyc_callback,
            pk_ccyc->tcb
        );
    } else {
        h = xTimerCreate(
            name,
            period,
            pdTRUE,
            timer_id,
            prv_cyc_callback
        );
    }

    if (h == NULL) return E_NOMEM;

    s_cyc_hdr_tbl[cycid] = pk_ccyc->cychdr;

    taskENTER_CRITICAL();
    s_cyc_tbl[cycid] = h;
    taskEXIT_CRITICAL();

    return E_OK;
}

ER del_cyc(ID cycid)
{
    if (cycid < 1 || cycid > MAX_CYC_ID) return E_ID;

    taskENTER_CRITICAL();
    TimerHandle_t h = s_cyc_tbl[cycid];
    s_cyc_tbl[cycid] = NULL;
    taskEXIT_CRITICAL();

    if (h == NULL) return E_ID;
    xTimerDelete(h, portMAX_DELAY);
    s_cyc_hdr_tbl[cycid] = NULL;
    return E_OK;
}

ER sta_cyc(ID cycid)
{
    if (cycid < 1 || cycid > MAX_CYC_ID) return E_ID;
    TimerHandle_t h = s_cyc_tbl[cycid];
    if (h == NULL) return E_ID;

    xTimerStart(h, 0);
    return E_OK;
}

ER stp_cyc(ID cycid)
{
    if (cycid < 1 || cycid > MAX_CYC_ID) return E_ID;
    TimerHandle_t h = s_cyc_tbl[cycid];
    if (h == NULL) return E_ID;

    xTimerStop(h, 0);
    return E_OK;
}
