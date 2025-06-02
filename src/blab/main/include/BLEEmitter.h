#ifndef BLEEMITTER
#define BLEEMITTER

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include <string>

class BLEEmitter {
public:
    BLEEmitter(const std::string &name) : _name(name) {}
    virtual ~BLEEmitter() {};
    virtual void Init() {}

    virtual ble_gatt_svc_def GetService();
    virtual uint16_t *GetHandle() { return &_handle; }
    virtual std::string GetName() const { return _name; }
    
    virtual uint8_t GetServiceType() const {
        return BLE_GATT_SVC_TYPE_PRIMARY;
    }
    virtual uint32_t GetEmissionInterval() const = 0;
    virtual int AccessData(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg) = 0;

protected:
    // subclasses implement so we can provide the service
    virtual ble_uuid16_t& _GetServiceUUID() = 0;
    virtual ble_uuid16_t& _GetCharacteristicUUID() = 0;

private:
    std::string _name;
    ble_gatt_chr_def _characteristics[2];
    uint16_t _handle;
};

#endif //BLEEMITTER
