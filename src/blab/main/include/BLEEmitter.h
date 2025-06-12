#ifndef BLEEMITTER
#define BLEEMITTER

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include <string>

class BLEEmitter {
public:
    BLEEmitter(const std::string &name) : _name(name), _connectionHandleSet(false) {}
    virtual ~BLEEmitter() {};
    virtual void Init() {}

    virtual ble_gatt_svc_def GetService();
    virtual uint16_t *GetCharacteristicHandle() { return &_charValHandle; }
    virtual uint16_t GetCharacteristicHandle() const { return _charValHandle; }
    virtual std::string GetName() const { return _name; }
    virtual void Emit() = 0;
    virtual uint8_t GetServiceType() const {
        return BLE_GATT_SVC_TYPE_PRIMARY;
    }

    virtual void SubscribeHandler(struct ble_gap_event *event) = 0;
    virtual uint32_t GetEmissionInterval() const = 0;
    virtual int AccessData(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg) = 0;
    bool IsConnectionHandleSet() const { return _connectionHandleSet; }
    void SetConnectionHandle(uint16_t handle) {
        _connectionHandle = handle;
        _connectionHandleSet = true;
    }
    uint16_t GetConnectionHandle() const { return _connectionHandle; }

  protected:
    // subclasses implement so we can provide the service
    virtual ble_uuid16_t& _GetServiceUUID() = 0;
    virtual ble_uuid16_t& _GetCharacteristicUUID() = 0;

private:
    std::string _name;
    ble_gatt_chr_def _characteristics[2];
    uint16_t _charValHandle;
    bool _connectionHandleSet;
    uint16_t _connectionHandle;
};

#endif //BLEEMITTER
