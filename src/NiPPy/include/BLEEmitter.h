#ifndef BLEEMITTER
#define BLEEMITTER

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include "Error.h"
#include <map>
#include <string>

#include "BLEEmitterHandler.h"

// BLEEmitters emit data for their service. It will be one or more characteristics.
class BLEEmitter {
public:
    BLEEmitter(const std::string &name) :
        _name(name) {   }
    virtual ~BLEEmitter() {};
    virtual void Init() {}

    void SetHandler(BLEEmitterHandler *handler) { _handler = handler; }
    virtual ble_gatt_svc_def GetService() = 0;

    virtual uint16_t *GetCharacteristicHandle() { return &_charValHandle; }
    virtual uint16_t GetCharacteristicHandle() const { return _charValHandle; }

    virtual std::string GetName() const { return _name; }
    virtual void Emit() = 0;
    virtual uint8_t GetServiceType() const {
        return BLE_GATT_SVC_TYPE_PRIMARY;
    }

    // GATT registration handler
    //virtual void RegisterHandler(struct ble_gap_event *event) = 0;
    // GAP event handler for subscribe.
    virtual void SubscribeHandler(struct ble_gap_event *event) = 0;
    virtual uint32_t GetEmissionInterval() const = 0;
    virtual int AccessData(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg) = 0;

    virtual ble_uuid16_t &GetServiceUUID() = 0;
    virtual ble_uuid16_t &GetCharacteristicUUID() = 0;

  protected:
    // subclasses implement so we can provide the service
    BLEEmitterHandler *_GetHandler() const { return _handler; }
    ble_gatt_chr_def _characteristics[2];

  private:
    std::string _name;
    BLEEmitterHandler *_handler;
    
    uint16_t _charValHandle;


};

#endif //BLEEMITTER
