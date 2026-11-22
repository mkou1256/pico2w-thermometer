#ifndef USB_CDC_COM_H
#define USB_CDC_COM_H

#include "ComModule.h"
#include <stdint.h>

class UsbCdcCom : public ComModule {
public:
    virtual ~UsbCdcCom() = default;
    static ComModule* Create();
protected:
    int32_t WriteSome(const uint8_t* data, size_t length) override;
    int32_t ReadSome(uint8_t* data, size_t length) override;
private:
    UsbCdcCom() = default;
    bool Init() override;
};
#endif // USB_CDC_COM_H