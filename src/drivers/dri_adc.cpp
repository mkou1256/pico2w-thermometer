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

struct AdcRegs {
    // ADC Control and Status
    // 0x01ff0000 [24:16] RROBIN       (0x000) Round-robin sampling
    // 0x0000f000 [15:12] AINSEL       (0x0) Select analog mux input
    // 0x00000400 [10]    ERR_STICKY   (0) Some past ADC conversion encountered an error
    // 0x00000200 [9]     ERR          (0) The most recent ADC conversion encountered an error;...
    // 0x00000100 [8]     READY        (0) 1 if the ADC is ready to start a new conversion
    // 0x00000008 [3]     START_MANY   (0) Continuously perform conversions whilst this bit is 1
    // 0x00000004 [2]     START_ONCE   (0) Start a single conversion
    // 0x00000002 [1]     TS_EN        (0) Power on temperature sensor
    // 0x00000001 [0]     EN           (0) Power on ADC and enable its clock
    volatile uint32_t cs;

    // Result of most recent ADC conversion
    // 0x00000fff [11:0]  RESULT       (0x000) 
    volatile uint32_t result;

    // FIFO control and status
    // 0x0f000000 [27:24] THRESH       (0x0) DREQ/IRQ asserted when level >= threshold
    // 0x000f0000 [19:16] LEVEL        (0x0) The number of conversion results currently waiting in the FIFO
    // 0x00000800 [11]    OVER         (0) 1 if the FIFO has been overflowed
    // 0x00000400 [10]    UNDER        (0) 1 if the FIFO has been underflowed
    // 0x00000200 [9]     FULL         (0) 
    // 0x00000100 [8]     EMPTY        (0) 
    // 0x00000008 [3]     DREQ_EN      (0) If 1: assert DMA requests when FIFO contains data
    // 0x00000004 [2]     ERR          (0) If 1: conversion error bit appears in the FIFO alongside...
    // 0x00000002 [1]     SHIFT        (0) If 1: FIFO results are right-shifted to be one byte in size
    // 0x00000001 [0]     EN           (0) If 1: write result to the FIFO after each conversion
    volatile uint32_t fcs;

    // Conversion result FIFO
    // 0x00008000 [15]    ERR          (-) 1 if this particular sample experienced a conversion error
    // 0x00000fff [11:0]  VAL          (-) 
    volatile uint32_t fifo;

    // Clock divider
    // 0x00ffff00 [23:8]  INT          (0x0000) Integer part of clock divisor
    // 0x000000ff [7:0]   FRAC         (0x00) Fractional part of clock divisor
    volatile uint32_t div;

    // Raw Interrupts
    // 0x00000001 [0]     FIFO         (0) Triggered when the sample FIFO reaches a certain level
    volatile uint32_t intr;

    // Interrupt Enable
    // 0x00000001 [0]     FIFO         (0) Triggered when the sample FIFO reaches a certain level
    volatile uint32_t inte;

    // Interrupt Force
    // 0x00000001 [0]     FIFO         (0) Triggered when the sample FIFO reaches a certain level
    volatile uint32_t intf;

    // Interrupt status after masking & forcing
    // 0x00000001 [0]     FIFO         (0) Triggered when the sample FIFO reaches a certain level
    volatile uint32_t ints;
};
AdcRegs* const adc_reg = ((AdcRegs *)DRI_ADC_BASE);
static_assert(sizeof (AdcRegs) == 0x0024, "");


constexpr uint32_t POLLING_MS = 1;
constexpr uint32_t READ_TIMEOUT_MS = 100;

AdcRet ADC_init(AdcTsEn en_temp) {
    reg_set(&adc_reg->cs, make_mask(DRI_ADC_CS_EN_SHIFT, DRI_ADC_CS_EN_LEN));

    if (en_temp == AdcTsEn::ENABLE) {
        reg_set(&adc_reg->cs, make_mask(DRI_ADC_CS_TS_EN_SHIFT, DRI_ADC_CS_TS_EN_LEN));
    }
    return AdcRet::SUCCESS;
}

AdcRet ADC_readOneShot(AdcCh ch, uint16_t* val) {
    bool is_ready = (adc_reg->cs & make_mask(DRI_ADC_CS_READY_SHIFT, DRI_ADC_CS_READY_LEN));
    if (!is_ready) {
        return AdcRet::NOT_READY;
    }

    /* 先に AINSELをクリアする */
    reg_clr(&adc_reg->cs, make_mask(DRI_ADC_CS_AINSEL_SHIFT, DRI_ADC_CS_AINSEL_LEN));
    /* AINSEL に ch を書き込む*/
    reg_write_masked(&adc_reg->cs, (uint32_t) ((uint8_t)ch << DRI_ADC_CS_AINSEL_SHIFT), make_mask(DRI_ADC_CS_AINSEL_SHIFT, DRI_ADC_CS_AINSEL_LEN));
    /* ONCE 開始 */
    reg_set(&adc_reg->cs, make_mask(DRI_ADC_CS_START_ONCE_SHIFT, DRI_ADC_CS_START_ONCE_LEN));

    uint32_t tick_us = time_us_32();
    while (1) {
        is_ready = (adc_reg->cs & make_mask(DRI_ADC_CS_READY_SHIFT, DRI_ADC_CS_READY_LEN));
        if (is_ready) break;

        if ((time_us_32() - tick_us) > READ_TIMEOUT_MS * 1000) {
            return AdcRet::TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(POLLING_MS));
    }

    *val = adc_reg->result;

    return AdcRet::SUCCESS;
}