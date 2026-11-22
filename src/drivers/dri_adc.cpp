#include "dri_adc.h"
#include "dri_reg_access.h"
#include "Logger.h"
#include <hardware/timer.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "pico/time.h"

constexpr uintptr_t DRI_ADC_BASE = 0x400a0000;
constexpr uintptr_t DRI_ADC_SET = DRI_ADC_BASE + DRI_REG_SET;
constexpr uintptr_t DRI_ADC_CLR = DRI_ADC_BASE + DRI_REG_CLR;
constexpr uintptr_t DRI_ADC_CS_OFFSET = 0x000;
constexpr uintptr_t DRI_ADC_RESULT_OFFSET = 0x004;

/* bit shift */
constexpr uint8_t DRI_ADC_CS_EN_SHIFT = 0;
constexpr uint8_t DRI_ADC_CS_TS_EN_SHIFT = 1;
constexpr uint8_t DRI_ADC_CS_START_ONCE_SHIFT = 2;
constexpr uint8_t DRI_ADC_CS_START_MANY_SHIFT = 3;
constexpr uint8_t DRI_ADC_CS_READY_SHIFT = 8;
constexpr uint8_t DRI_ADC_CS_ERR_SHIFT = 9;
constexpr uint8_t DRI_ADC_CS_ERR_STICKY_SHIFT = 10;
constexpr uint8_t DRI_ADC_CS_AINSEL_SHIFT = 12;
constexpr uint8_t DRI_ADC_CS_RROBIN_SHIFT = 16;

/* bit length */
constexpr uint8_t DRI_ADC_CS_EN_LEN = 1;
constexpr uint8_t DRI_ADC_CS_TS_EN_LEN = 1;
constexpr uint8_t DRI_ADC_CS_START_ONCE_LEN = 1;
constexpr uint8_t DRI_ADC_CS_START_MANY_LEN = 1;
constexpr uint8_t DRI_ADC_CS_READY_LEN = 1;
constexpr uint8_t DRI_ADC_CS_ERR_LEN = 1;
constexpr uint8_t DRI_ADC_CS_ERR_STICKY_LEN = 1;
constexpr uint8_t DRI_ADC_CS_AINSEL_LEN = 4;
constexpr uint8_t DRI_ADC_CS_RROBIN_LEN = 9;

constexpr uint32_t POLLING_MS = 1;
constexpr uint32_t READ_TIMEOUT_MS = 100;

AdcRet ADC_init(AdcTsEn en_temp) {
    volatile uint32_t* cs_reg = (volatile uint32_t*) (DRI_ADC_BASE + DRI_ADC_CS_OFFSET);
    LOG_INFO("initial adc_cs reg = 0x%08lx", *cs_reg);

    volatile uint32_t* cs_set_reg = (volatile uint32_t*) (DRI_ADC_SET + DRI_ADC_CS_OFFSET);
    *cs_set_reg = mask(DRI_ADC_CS_EN_SHIFT, DRI_ADC_CS_EN_LEN);

    if (en_temp == AdcTsEn::ENABLE) {
        *cs_set_reg = mask(DRI_ADC_CS_TS_EN_SHIFT, DRI_ADC_CS_TS_EN_LEN);
    }

    LOG_INFO("after adc_cs reg = 0x%08lx", *cs_reg);
    return AdcRet::SUCCESS;
}

AdcRet ADC_readOneShot(AdcCh ch, uint16_t* val) {
    volatile uint32_t* cs_reg = (volatile uint32_t*) (DRI_ADC_BASE + DRI_ADC_CS_OFFSET);
    volatile uint32_t* cs_set_reg = (volatile uint32_t*) (DRI_ADC_SET + DRI_ADC_CS_OFFSET);
    volatile uint32_t* cs_clr_reg = (volatile uint32_t*) (DRI_ADC_CLR + DRI_ADC_CS_OFFSET);

    bool is_ready = (*cs_reg & mask(DRI_ADC_CS_READY_SHIFT, DRI_ADC_CS_READY_LEN));
    if (!is_ready) {
        return AdcRet::NOT_READY;
    }

    /* 先に AINSELをクリアする */
    *cs_clr_reg = mask(DRI_ADC_CS_AINSEL_SHIFT, DRI_ADC_CS_AINSEL_LEN);
    /* AINSEL に ch を書き込む*/
    *cs_set_reg = (uint32_t) ((uint8_t)ch << DRI_ADC_CS_AINSEL_SHIFT);
    /* ONCE 開始 */
    *cs_set_reg = mask(DRI_ADC_CS_START_ONCE_SHIFT, DRI_ADC_CS_START_ONCE_LEN);

    uint32_t tick_us = time_us_32();
    while (1) {
        is_ready = (*cs_reg & mask(DRI_ADC_CS_READY_SHIFT, DRI_ADC_CS_READY_LEN));
        if (is_ready) break;

        if ((time_us_32() - tick_us) > READ_TIMEOUT_MS * 1000) {
            return AdcRet::TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(POLLING_MS));
    }

    volatile uint16_t* result_reg = (volatile uint16_t*) (DRI_ADC_BASE + DRI_ADC_RESULT_OFFSET);
    *val = *result_reg;

    return AdcRet::SUCCESS;
}