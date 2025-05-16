#ifndef BLEEMITTER
#define BLEEMITTER

#include "host/ble_gatt.h"

#include <string>

class BLEEmitter {
public:
    BLEEmitter(const std::string &name) : _name(name) {}
    virtual ~BLEEmitter() {};
    virtual void Init() {}

    virtual std::string GetName() const { return _name; }
    virtual uint8_t GetServiceType() const {
        return BLE_GATT_SVC_TYPE_PRIMARY;
    }
    virtual const ble_uuid16_t& GetServiceUUID() const = 0;
    virtual const ble_uuid16_t& GetCharacteristicUUID() const = 0;
    virtual uint32_t GetEmissionInterval() const = 0;
    virtual int AccessData(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg) = 0;
    
private:
    std::string _name;
};

#endif //BLEEMITTER
