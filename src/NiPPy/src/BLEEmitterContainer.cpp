#include "BLEEmitterContainer.h"

#include "BLEEmitter.h"

BLEEmitterContainer::BLEEmitterContainer():
    _numImplicitServices(0),
    _connectionHandleSet(false),
    _currentServiceUUID(0) {
    // create the implicit ones
}

BLEEmitterContainer::~BLEEmitterContainer() {}

void BLEEmitterContainer::AddEmitter(BLEEmitter *emitter) {
    emitter->SetHandler(this);
    _emitters.push_back(emitter);
}

uint32_t BLEEmitterContainer::GetBroadcastInterval() const {
    uint32_t min = UINT32_MAX;
    for (const auto& emitter : _emitters) {
        if (emitter->GetEmissionInterval() < min) {
            min = emitter->GetEmissionInterval();
        }
    }
    return min;
}

void BLEEmitterContainer::Emit() const {
    for (const auto& emitter : _emitters) {
        emitter->Emit();
    }
}

int BLEEmitterContainer::GetNumServices(bool all) {
    int numEmitters = _emitters.size();
    if (all) {
      return numEmitters;
    } else {
        return numEmitters -_numImplicitServices;
    }
}

uint16_t BLEEmitterContainer::GetServiceHandleForUUID(const uint16_t& uuidValue) {
    return _serviceUUIDToHandles[uuidValue]; }

void BLEEmitterContainer::RegisterServiceHandle(const uint16_t& uuidValue,
                                                const uint16_t& handle) {
    _currentServiceUUID = uuidValue;
    _serviceUUIDToHandles[uuidValue] = handle;

    _handleToUUIDMap[handle] = uuidValue;
}

BlabError BLEEmitterContainer::SetCharacteristicHandlesForUUID(const ble_uuid_t *uuid,
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

uint16_t BLEEmitterContainer::GetCharacteristicValHandleByUUIDs(
                                                                const ble_uuid16_t &svcUuid,
                                                                const ble_uuid16_t &charUuid) const {
    const CharacteristicHandleMap& charHandles = _serviceCharsMap.at(svcUuid.value);
    auto devValPair = charHandles.at(charUuid.value);
    return devValPair.second;
}
