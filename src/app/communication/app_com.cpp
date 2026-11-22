#include "app_com.h"
#include "ComModule.h"
#include "UsbSerial.h"

static ComModule* s_comModules[COM_CHANNEL_MAX];

static void comModule_Create() {
    s_comModules[COM_CHANNEL_USB_SERIAL] = UsbSerial::Create();
}

static bool comModule_check() {
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

ComModule* getComModule(COM_CHANNEL ch) {
    if (ch >= COM_CHANNEL_MAX) return nullptr;
    return s_comModules[ch];
}
