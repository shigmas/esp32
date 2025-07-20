#ifndef BLEEMITTER
#define BLEEMITTER

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include "Error.h"
#include <map>
#include <string>

class BLEEmitter {
public:
    // XXX it should probably handle *to* UUID, not UUID to handle, like we have here.
    // let's see if we need to look up too much.;xsy
    // pair of handles (def and val)
    typedef std::pair< uint16_t, uint16_t> CharacteristicHandlePair;
    // Type to map Characteristic UUID (16-bit) to the Def and Val handles
    typedef std::map< uint16_t, CharacteristicHandlePair > CharacteristicHandleMap;

    // Type to map the Service UUID (16-bit) to the handle.
    //typedef std::vector< uint16_t, uint16_t > ServiceHandleMap;
    typedef std::map<uint16_t, uint16_t> ServiceHandleMap;
    // Type to map a service UUID to the characteric handles
    typedef std::map< uint16_t, CharacteristicHandleMap > ServiceCharacteristicsMap;

    BLEEmitter(const std::string &name) :
        _name(name),
        _currentServiceUUID(0),
        _connectionHandleSet(false){   }
    virtual ~BLEEmitter() {};
    virtual void Init() {}

    virtual ble_gatt_svc_def GetService() = 0;
    virtual uint16_t GetServiceHandleForUUID(const uint16_t& uuidValue);
    // for now, we'll take other services
    virtual void RegisterServiceHandle(const uint16_t& uuidValue, const uint16_t& handle);

    // These need an index
    virtual uint16_t *GetCharacteristicHandle() { return &_charValHandle; }
    virtual uint16_t GetCharacteristicHandle() const { return _charValHandle; }
    virtual BlabError SetCharacteristicHandlesForUUID(const ble_uuid_t *uuid,
                                                      const uint16_t &defHandle,
                                                      const uint16_t &valHandle);

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
    bool IsConnectionHandleSet() const { return _connectionHandleSet; }
    void SetConnectionHandle(uint16_t handle) {
        _connectionHandle = handle;
        _connectionHandleSet = true;
    }
    uint16_t GetConnectionHandle() const { return _connectionHandle; }

    virtual ble_uuid16_t &GetServiceUUID() = 0;
    virtual ble_uuid16_t &GetCharacteristicUUID() = 0;

  protected:
    // subclasses implement so we can provide the service

    ble_gatt_chr_def _characteristics[2];

  private:
    std::string _name;

    uint16_t _connectionHandle;

    uint16_t _currentServiceUUID;
    ServiceHandleMap _serviceUUIDToHandles;
    ServiceCharacteristicsMap _serviceCharsMap;
    // mix of both worlds - keep handles
    std::map<uint16_t, uint16_t> _handleToUUIDMap;
    
    uint16_t _charValHandle;
    bool _connectionHandleSet;

};

#endif //BLEEMITTER
