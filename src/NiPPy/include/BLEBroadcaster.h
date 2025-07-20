#ifndef BLEBROADCASTER
#define BLEBROADCASTER

// ble_gatt_svc_def
#include "host/ble_gatt.h"
//#include "services/gatt/ble_svc_gatt.h"

#include <string>

class BLEEmitter;
class BLEEmitterContainer;

class BLEBroadcaster {
public:
    BLEBroadcaster(const std::string& name);
    ~BLEBroadcaster();

    // sets the emitter stuff, so all the AddEmitter() calls should be complete by now
    void Init();
    void StartAdvertising();

    void AddEmitter(BLEEmitter *emitter);

    uint32_t GetBroadcastInterval() const;
    // fetches the data from emitters and sends it
    void Broadcast();
    
protected:
    int _NVSInit();
    int _GAPInit();
    int _GATTInit();
    int _HostConfig();

    void _OnStackReset(int reason);
    void _OnStackSync();

    void _InitAdvertising();
    void _StartAdvertising();

    int _GAPEventHandler(struct ble_gap_event *event, void *arg);
    void _GATTRegisterHandler(struct ble_gatt_register_ctxt *ctxt, void *arg);
    
private:
    std::string _name;

    BLEEmitterContainer *_emitters;

    // we need this to stick around in memory because NimBLE uses it by reference
    ble_gatt_svc_def  *_gatt_svr_svcs;
    uint16_t _connectionHandle;
    uint8_t _ownAddrType;
    uint8_t _addrVal[6];

};

#endif //BLEBROADCASTER
