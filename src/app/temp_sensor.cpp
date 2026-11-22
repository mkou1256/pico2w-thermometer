#include "temp_sensor.h"
#include "Logger.h"
#include "FreeRTOS.h"
#include "drivers/dri_adc.h"
#include "hardware/adc.h"
#include "task.h"
#include "projdefs.h"

static void tempSensor_init(void) {
    // adc_init();
}

#define VBE_AT_27C_UV 706000
#define SLOPE_UV_PER_C 1721
#define C_TO_MC 1000
#define V_REF_UV 3300000
#define C_REF_MC 27000
#define ADC_RANGE 4096
static int32_t calcTemp_mc(uint16_t adc_val) {
    int64_t v_uv = (int64_t) (((int64_t)adc_val * V_REF_UV) / ADC_RANGE);
    int32_t t_mc = (int32_t) (C_REF_MC - (int64_t)(v_uv - VBE_AT_27C_UV) * C_TO_MC / SLOPE_UV_PER_C);
    return t_mc;
}

void tempSensor_task(void *param) {
    (void)param;

    tempSensor_init();

    for (;;) {
        uint16_t adc_val;
        AdcRet ret = ADC_readOneShot(AdcCh::CH_TEMP, &adc_val);
        if (ret == AdcRet::SUCCESS) {
            int32_t temp_mc = calcTemp_mc(adc_val);
            LOG_INFO("original ret = %u, val = %u, temp = %u", ret, adc_val, temp_mc);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}