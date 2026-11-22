#ifndef DRI_ADC_H
#define DRI_ADC_H

#include <cstdint>

enum class AdcTsEn {
    DISABLE = 0,
    ENABLE,
};

enum class AdcRet {
    SUCCESS = 0,
    NOT_READY,
    TIMEOUT,
};

enum class AdcCh {
    CH_0 = 0,
    CH_1,
    CH_2,
    CH_3,
    CH_TEMP
};

AdcRet ADC_init(AdcTsEn en_temp);
AdcRet ADC_readOneShot(AdcCh ch, uint16_t* adc_val);

#endif /* DRI_ADC_H */