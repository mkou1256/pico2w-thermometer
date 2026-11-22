#include "UsbCdcCom.h"
#include "tusb.h"

ComModule* UsbCdcCom::Create() {
    /* itfにつき一つのみのインスタンスに制限する */
    static uint32_t count = 0;
    if (count >= CFG_TUD_CDC) {
        ASSERT(0);
        return nullptr;
    }

    UsbCdcCom* com = new UsbCdcCom();
    if (!com->Init()) {
        delete com;
        return nullptr;
    }
    count++;
    return com;
}

bool UsbCdcCom::Init() {
    /* stdio_usb_init() は起動時に呼ばれる。
     * 2回呼び出さないようここでは初期化確認だけ行う。
     */
    return tud_inited();
}

int32_t UsbCdcCom::WriteSome(const uint8_t* data, size_t length) {
    if (!tud_cdc_connected()) {
        return COM_ERR_CONNECT;
    }
    int32_t sent = (int32_t)tud_cdc_write(data, length);
    tud_cdc_write_flush();
    return sent;
}

int32_t UsbCdcCom::ReadSome(uint8_t* data, size_t length) {
    if (!tud_cdc_connected()) {
        return COM_ERR_CONNECT;
    }
    int32_t recv = (int32_t)tud_cdc_read(data, length);
    return recv;
}
