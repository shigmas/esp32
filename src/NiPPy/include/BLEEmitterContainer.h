#ifndef BLE_EMITTERCONTAINER
#define BLE_EMITTERCONTAINER

#include "esp_log.h"

#include "BLEEmitterHandler.h"
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#include <map>
#include <stdexcept>
#include <vector>

#define ECTAG "EmitterContainer"

class BLEEmitter;

// BLEEmitterContainer is just a container for the emitters as well as some registration
// information about the emitters. This will also create the implicit emitters.

class BLEEmitterContainer : public BLEEmitterHandler {
    // XXX it should probably handle *to* UUID, not UUID to handle, like we have here.
    // let's see if we need to look up too much.
    // pair of handles (def and val)
    typedef std::pair< uint16_t, uint16_t> CharacteristicHandlePair;
    // Type to map Characteristic UUID (16-bit) to the Def and Val handles
    typedef std::map< uint16_t, CharacteristicHandlePair > CharacteristicHandleMap;

    // Type to map the Service UUID (16-bit) to the handle.
    //typedef std::vector< uint16_t, uint16_t > ServiceHandleMap;
    typedef std::map<uint16_t, uint16_t> ServiceHandleMap;
    // Type to map a service UUID to the characteric handles
    typedef std::map< uint16_t, CharacteristicHandleMap > ServiceCharacteristicsMap;


public:
    struct Iterator {
        Iterator(std::vector<BLEEmitter*> es, size_t idx) : _es(es), _idx(idx) {}
        BLEEmitter* operator*() const {
            ESP_LOGI(ECTAG, "idx: %d, size: %d", _idx, _es.size());
            if (_idx >= _es.size()) {
                //throw std::out_of_range{"Iterator went out of bounds"};
                return NULL;
            }
            return _es[_idx];
        }

        Iterator &operator++() {
          _idx++;
          return *this;
        }

        bool operator==(const Iterator &b) const {
            return _es == b._es && _idx == b._idx;
        }

    private:
        std::vector<BLEEmitter*> _es;
        size_t _idx;
    };

    BLEEmitterContainer();
    virtual ~BLEEmitterContainer();

    // Add an Emitter
    void AddEmitter(BLEEmitter *emitter);

    // Get the interval that we should broadcast 
    uint32_t GetBroadcastInterval() const;

    uint16_t GetServiceHandleForUUID(const uint16_t& uuidValue) override;
    void RegisterServiceHandle(const uint16_t& uuidValue, const uint16_t& handle) override;

    BlabError SetCharacteristicHandlesForUUID(const ble_uuid_t *uuid,
                                              const uint16_t &defHandle,
                                              const uint16_t &valHandle) override;

    uint16_t GetCharacteristicValHandleByUUIDs(const ble_uuid16_t &svcUuid,
                                               const ble_uuid16_t &charUuid) const override;

    bool IsConnectionHandleSet() const override { return _connectionHandleSet; }
    void SetConnectionHandle(uint16_t handle) override {
        _connectionHandle = handle;
        _connectionHandleSet = true;
    }
    uint16_t GetConnectionHandle() const override { return _connectionHandle; }

    // Emit from all services
    void Emit() const;

    // Gets the number of services. By default, we don't include implicit services
    int GetNumServices(bool all=false);
    
    // make this iterable
    Iterator begin() {
        return Iterator(_emitters, _numImplicitServices);
    }
    Iterator end() {
        return Iterator(_emitters, GetNumServices());
    }

private:
    std::vector<BLEEmitter*> _emitters;
    int _numImplicitServices;

    uint16_t _connectionHandle;
    bool _connectionHandleSet;

    uint16_t _currentServiceUUID;
    ServiceHandleMap _serviceUUIDToHandles;
    ServiceCharacteristicsMap _serviceCharsMap;
    // mix of both worlds - keep handles
    std::map<uint16_t, uint16_t> _handleToUUIDMap;
};


#endif // BLE_EMITTERCONTAINER
