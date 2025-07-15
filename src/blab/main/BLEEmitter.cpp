#include "BLEEmitter.h"

#include "esp_log.h"

#include "CallbackDef.h"

#define TAG "BLEEmitter"
uint16_t BLEEmitter::GetServiceHandleForUUID(const uint16_t& uuidValue) {
    return _serviceUUIDToHandles[uuidValue]; }

void BLEEmitter::RegisterServiceHandle(const uint16_t& uuidValue,
                                           const uint16_t& handle) {
    _currentServiceUUID = uuidValue;
    _serviceUUIDToHandles[uuidValue] = handle;

    _handleToUUIDMap[handle] = uuidValue;
}

BlabError BLEEmitter::SetCharacteristicHandlesForUUID(const ble_uuid_t *uuid,
                                                      const uint16_t &defHandle,
                                                      const uint16_t &valHandle) {
    // we constantly look up the current service and then get the current handle
    // map. It would be better to build up a current handle map, then set it when
    // we get the next service registration.
    if (uuid->type != BLE_UUID_TYPE_16) {
        return BlabError("invalud UUID type");
    }
    if (_currentServiceUUID == 0) {
      return BlabError("no service being registered");
    }
    ble_uuid16_t * uuid16 = (ble_uuid16_t *)uuid;
    CharacteristicHandleMap& charHandles = _serviceCharsMap[_currentServiceUUID];
    charHandles[uuid16->value] = std::make_pair(defHandle, valHandle);
    // if not by value, need to set it again
    _handleToUUIDMap[defHandle] = uuid16->value;
    _handleToUUIDMap[valHandle] = uuid16->value;
  
    return BlabError();
}
