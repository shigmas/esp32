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

private:
    std::string _name;
};

#endif //BLEBROADCASTER
