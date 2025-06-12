#include "BLEEmitter.h"

#include "esp_log.h"

#include "CallbackDef.h"

#define TAG "BLEEmitter"

// move this to the subclass - this only does one characteristic.
ble_gatt_svc_def BLEEmitter::GetService() {
    Callback<int(uint16_t, uint16_t, ble_gatt_access_ctxt *, void *)>::func = std::bind(&BLEEmitter::AccessData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    _characteristics[0] = {
        // ble_uuid_t: just the type, but I guess the first element
        // of the struct is basically the ref to the struct?
        .uuid = &_GetCharacteristicUUID().u,
        .access_cb =
        Callback<int(uint16_t, uint16_t, ble_gatt_access_ctxt *,
                     void *)>::callback,
        // can add a flags to the emitter
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
        // 'value' is const &, so fetch it.
        .val_handle = GetCharacteristicHandle(),
    };
    _characteristics[1] = {
        0, /* No more characteristics in this service. */
    };

    ble_gatt_svc_def svc = {
      .type = GetServiceType(),
      // ble_uuid_t: just the type
      .uuid = &_GetServiceUUID().u,
      .characteristics = _characteristics,
    };

    return svc;
}
