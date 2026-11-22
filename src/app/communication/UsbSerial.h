#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include "ComModule.h"
#include <stdint.h>

class UsbSerial : public ComModule {
public:
    virtual ~UsbSerial() = default;
    static ComModule* Create();
protected:
    int32_t WriteSome(const uint8_t* data, size_t length) override;
    int32_t ReadSome(uint8_t* data, size_t length) override;
private:
    UsbSerial() = default;
    bool Init() override;
};
#endif // USB_SERIAL_H