#include "app_com.h"
#include "ComModule.h"
#include "UsbCdcCom.h"

enum COM_CHANNEL {
    COM_CHANNEL_USB_CDC = 0,
    COM_CHANNEL_MAX,
};

static ComModule* s_comModules[COM_CHANNEL_MAX];

void comModule_Create() {
    s_comModules[COM_CHANNEL_USB_CDC] = UsbCdcCom::Create();
}

bool comModule_check() {
    for (int i = 0; i < COM_CHANNEL_MAX; ++i) {
        if (s_comModules[i] == nullptr) {
            return false;
        }
    }
    return true;
}

bool AppCom_Init() {
    comModule_Create();
    if (!comModule_check()) {
        return false;
    }
    return true;
}