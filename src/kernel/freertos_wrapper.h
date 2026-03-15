#ifndef FREERTOS_WRAPPER_H
#define FREERTOS_WRAPPER_H

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"

/* ITRONスタンダード基本型 */
typedef int          ER;      /* エラーコード */
typedef int          ID;      /* オブジェクトID */
typedef unsigned int ATR;     /* オブジェクト属性 */
typedef int          PRI;     /* タスク優先度 */
typedef unsigned int SIZE;    /* メモリサイズ (バイト) */
typedef int          TMO;     /* タイムアウト値 (ms / 特殊値) */
typedef unsigned int FLGPTN;  /* イベントフラグ ビットパターン */
typedef unsigned int MODE;    /* 待ちモード */
/* タイムアウト特殊値 */
#define TMO_POL   ((TMO)0)   /* ポーリング (即時返却) */
#define TMO_FEVR  ((TMO)-1)  /* 永久待ち */

/* エラーコード */
#define E_OK      ( 0)
#define E_NOMEM   (-5)   /* メモリ不足 */
#define E_ID      (-18)  /* ID範囲外 */
#define E_CTX     (-25)  /* コンテキスト不正 (ISR内) */
#define E_OBJ     (-41)  /* オブジェクト状態不正 (既に生成済み等) */
#define E_TMOUT   (-50)  /* タイムアウト */
#define E_RLWAI   (-49)  /* 強制解除 */

/* 属性ビット */
#define TA_NULL    (0x00U)  /* 指定なし */
#define TA_STATIC  (0x80U)  /* 静的確保 (呼び出し側がバッファを用意) */

/* イベントフラグ 待ちモード */
#define TWF_ANDW   (0x00U)  /* AND待ち (全ビット成立) */
#define TWF_ORW    (0x01U)  /* OR待ち  (いずれか成立) */
#define TWF_CLR    (0x10U)  /* 待ち解除後に自動クリア */


/* タスク関数型 (FreeRTOS TaskFunction_t と互換) */
typedef void (*TASK)(void *exinf);


/* ---- タスク ---- */
typedef struct t_ctsk {
    ATR         tskatr;   /* 属性 (TA_STATIC 等) */
    void       *exinf;   /* タスク引数 (pvParameters 相当) */
    TASK  task;       /* タスク関数ポインタ */
    PRI   itskpri;    /* 初期優先度 (1〜configMAX_PRIORITIES-1) */
    SIZE  stksz;      /* スタックサイズ (バイト) */
    const char *name; /* タスク名 (デバッグ用, NULLは"task"で代替) */
    /* --- TA_STATIC 時のみ使用 --- */
    StackType_t  *stk;  /* スタック領域ポインタ */
    StaticTask_t *tcb;  /* TCB領域ポインタ */
} T_CTSK;

/* ---- セマフォ ---- */
typedef struct t_csem {
    ATR  sematr;   /* 属性 */
    int  isemcnt;  /* 初期カウント値 */
    int  maxsem;   /* 最大カウント値 */
    /* --- TA_STATIC 時のみ使用 --- */
    StaticSemaphore_t *scb; /* セマフォ制御ブロック */
} T_CSEM;

/* ---- ミューテックス ---- */
typedef struct t_cmtx {
    ATR  mtxatr;   /* 属性 */
    /* --- TA_STATIC 時のみ使用 --- */
    StaticSemaphore_t *mcb;
} T_CMTX;

/* ---- イベントフラグ ---- */
typedef struct t_cflg {
    ATR    flgatr;   /* 属性 */
    FLGPTN iflgptn;  /* 初期ビットパターン */
    /* --- TA_STATIC 時のみ使用 --- */
    StaticEventGroup_t *ecb;
} T_CFLG;

/* ---- 周期ハンドラ (タイマ) ---- */
typedef struct t_ccyc {
    ATR   cycatr;    /* 属性 */
    void  (*cychdr)(ID cycid); /* ハンドラ関数 */
    TMO   cyctim;    /* 周期 (ms) */
    const char *name;
    /* --- TA_STATIC 時のみ使用 --- */
    StaticTimer_t *tcb;
} T_CCYC;

/* ハンドルテーブルサイズ上限 */
#define MAX_TASK_ID   8
#define MAX_SEM_ID    8
#define MAX_MTX_ID    8
#define MAX_FLG_ID    8
#define MAX_CYC_ID    8

/* ========================================================================
 * 静的確保ヘルパーマクロ (μITRON コンフィギュレータ相当)
 *
 * スタック配列・制御ブロックの宣言と T_C*** 構造体の初期化を一括で行う。
 * TA_STATIC / stk / tcb フィールドをアプリ側から隠蔽できる。
 *
 * 使用例:
 *   DEF_CTSK(led_ctsk, led_task, 2, 512);   // ファイルスコープで宣言
 *   cre_tsk(1, &led_ctsk);                  // main などで使用
 * ======================================================================== */

/**
 * @brief 静的タスク生成情報マクロ
 * @param _var    生成する T_CTSK 変数名
 * @param _func   タスク関数 (TASK 型)
 * @param _pri    初期優先度
 * @param _words  スタックサイズ [ワード数 (StackType_t 単位)]
 */
#define DEF_CTSK(_var, _func, _pri, _words)              \
    static StackType_t  _stk_##_var[(_words)];           \
    static StaticTask_t _tcb_##_var;                     \
    static const T_CTSK _var = {                         \
        .tskatr  = TA_STATIC,                            \
        .exinf   = NULL,                                 \
        .task    = (_func),                              \
        .itskpri = (_pri),                               \
        .stksz   = sizeof(_stk_##_var),                  \
        .name    = #_var,                                \
        .stk     = _stk_##_var,                          \
        .tcb     = &_tcb_##_var,                         \
    }

/**
 * @brief 静的セマフォ生成情報マクロ
 * @param _var   生成する T_CSEM 変数名
 * @param _init  初期カウント値
 * @param _max   最大カウント値
 */
#define DEF_CSEM(_var, _init, _max)                      \
    static StaticSemaphore_t _scb_##_var;                \
    static const T_CSEM _var = {                         \
        .sematr  = TA_STATIC,                            \
        .isemcnt = (_init),                              \
        .maxsem  = (_max),                               \
        .scb     = &_scb_##_var,                         \
    }

/**
 * @brief 静的ミューテックス生成情報マクロ
 * @param _var  生成する T_CMTX 変数名
 */
#define DEF_CMTX(_var)                                   \
    static StaticSemaphore_t _mcb_##_var;                \
    static const T_CMTX _var = {                         \
        .mtxatr = TA_STATIC,                             \
        .mcb    = &_mcb_##_var,                          \
    }

/**
 * @brief 静的イベントフラグ生成情報マクロ
 * @param _var      生成する T_CFLG 変数名
 * @param _initptn  初期ビットパターン
 */
#define DEF_CFLG(_var, _initptn)                         \
    static StaticEventGroup_t _ecb_##_var;               \
    static const T_CFLG _var = {                         \
        .flgatr  = TA_STATIC,                            \
        .iflgptn = (_initptn),                           \
        .ecb     = &_ecb_##_var,                         \
    }

/**
 * @brief 静的周期ハンドラ生成情報マクロ
 * @param _var        生成する T_CCYC 変数名
 * @param _handler    コールバック関数 (void (*)(ID))
 * @param _period_ms  周期 [ms]
 */
#define DEF_CCYC(_var, _handler, _period_ms)             \
    static StaticTimer_t _tmr_##_var;                    \
    static const T_CCYC _var = {                         \
        .cycatr  = TA_STATIC,                            \
        .cychdr  = (_handler),                           \
        .cyctim  = (_period_ms),                         \
        .name    = #_var,                                \
        .tcb     = &_tmr_##_var,                         \
    }


/**
 * @brief タスク生成
 *
 * @param tskid  割り当てるタスクID (1〜MAX_TASK_ID)
 * @param pk_ctsk 生成情報。TA_STATIC 指定時は stk/tcb に有効なアドレスを渡すこと
 * @return E_OK / E_NOMEM / E_ID
 *
 * @note 生成直後からRunnable状態になる (ITRON の act_tsk 相当を兼ねる)
 *       FreeRTOS では xTaskCreate/xTaskCreateStatic が生成と起動を一括処理するため
 */
ER cre_tsk(ID tskid, const T_CTSK *pk_ctsk);

/**
 * @brief タスク削除 (自タスクは vTaskDelete(NULL) で代替)
 */
ER del_tsk(ID tskid);

/**
 * @brief 相対時間待ち (ms 指定)
 *
 * @param dlytim 待ち時間 [ms]。0 の場合は yield のみ
 */
ER dly_tsk(TMO dlytim);

/**
 * @brief タスク起床待ち (通知ベース簡易スリープ)
 *
 * @param tmout TMO_FEVR / TMO_POL / ms値
 * @return E_OK (起床) / E_TMOUT (タイムアウト) / E_CTX (ISR内呼び出し)
 */
ER slp_tsk(TMO tmout);

/**
 * @brief タスク起床 (wup_tsk)
 */
ER wup_tsk(ID tskid);


ER cre_sem(ID semid, const T_CSEM *pk_csem);
ER del_sem(ID semid);

/**
 * @brief セマフォ獲得
 *
 * @param semid セマフォID
 * @param tmout TMO_FEVR / TMO_POL / ms値
 */
ER wai_sem(ID semid, TMO tmout);

/** ポーリング (wai_sem(id, TMO_POL) の糖衣) */
static inline ER pol_sem(ID semid) { return wai_sem(semid, TMO_POL); }

ER sig_sem(ID semid);   /* ISR内からも呼び出し可 */


ER cre_mtx(ID mtxid, const T_CMTX *pk_cmtx);
ER del_mtx(ID mtxid);

/**
 * @param tmout TMO_FEVR / TMO_POL / ms値
 */
ER loc_mtx(ID mtxid, TMO tmout);
ER unl_mtx(ID mtxid);


ER cre_flg(ID flgid, const T_CFLG *pk_cflg);
ER del_flg(ID flgid);

ER set_flg(ID flgid, FLGPTN setptn);   /* ISR内からも呼び出し可 */
ER clr_flg(ID flgid, FLGPTN clrptn);

/**
 * @param waiptn 待ちビットパターン
 * @param wfmode TWF_ANDW / TWF_ORW | TWF_CLR (論理OR指定可)
 * @param p_flgptn 成立時のビットパターンを格納するポインタ (NULLは不可)
 * @param tmout   TMO_FEVR / TMO_POL / ms値
 */
ER wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout);


ER cre_cyc(ID cycid, const T_CCYC *pk_ccyc);
ER del_cyc(ID cycid);
ER sta_cyc(ID cycid);
ER stp_cyc(ID cycid);


#endif /* FREERTOS_WRAPPER_H */