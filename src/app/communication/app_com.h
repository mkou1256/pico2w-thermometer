#ifndef APP_COM_H
#define APP_COM_H

#include "ComModule.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum COM_CHANNEL {
    COM_CHANNEL_USB_SERIAL = 0,
    COM_CHANNEL_MAX,
};

bool AppCom_Init();
ComModule* getComModule(COM_CHANNEL);

#ifdef __cplusplus
}
#endif

#endif // APP_COM_H