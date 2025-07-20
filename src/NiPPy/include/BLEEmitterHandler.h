#ifndef BLE_EMITTERHANDLER
#define BLE_EMITTERHANDLER

#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include <stdexcept>
#include <vector>

#include "Error.h"

class BLEEmitter;

// BLEEmitterHandler is the abstract class to hold the handles given to us by NimBLE.
// These methods are implemented by the BLEEmitterContainer.
class BLEEmitterHandler {
public:
    virtual uint16_t GetServiceHandleForUUID(const uint16_t& uuidValue) = 0;
    virtual void RegisterServiceHandle(const uint16_t& uuidValue, const uint16_t& handle) = 0;

    virtual BlabError SetCharacteristicHandlesForUUID(const ble_uuid_t *uuid,
                                                      const uint16_t &defHandle,
                                                      const uint16_t &valHandle) = 0;

    virtual uint16_t GetCharacteristicValHandleByUUIDs(const ble_uuid16_t &svcUuid,
                                                       const ble_uuid16_t &charUuid) const = 0;

    virtual bool IsConnectionHandleSet() const = 0;
    virtual void SetConnectionHandle(uint16_t handle) = 0;

    virtual uint16_t GetConnectionHandle() const = 0;
};


#endif // BLE_EMITTERHANDLER
