#ifndef BLEBROADCASTER
#define BLEBROADCASTER

#include <string>

class BLEEmitter;

class BLEBroadcaster {
public:
    BLEBroadcaster(const std::string& name);
    ~BLEBroadcaster();
    void Init();
    void StartAdvertising();

    void AddEmitter();
    
protected:
    int _NVSInit();
    int _GAPInit();
    int _GATTInit();
    int _HostConfig();

    void _OnStackReset(int reason);
    void _OnStackSync();

    void _InitAdvertising();
    void _StartAdvertising();

private:
    std::string _name;

    static uint8_t _ownAddrType;
    static uint8_t _addrVal[6];

};

#endif //BLEBROADCASTER
