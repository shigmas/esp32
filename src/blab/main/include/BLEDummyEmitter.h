#ifndef BLE_DUMMYEMITTER
#define BLE_DUMMYEMITTER

#include "BLEEmitter.h"

#include "esp_random.h"

// for portTICK_PERIOD_MS
#include <freertos/FreeRTOS.h>


#include <string>


#define HEART_RATE_TASK_PERIOD (1000 / portTICK_PERIOD_MS)

class BLEDummyEmitter : public BLEEmitter {

    typedef BLEEmitter super;

public:
  BLEDummyEmitter(const std::string &name)
      : super(name), _serviceUUID(BLE_UUID16_INIT(0x181A)),
        _characteristicUUID(BLE_UUID16_INIT(0x2A6E)) {}
    virtual ~BLEDummyEmitter() {}

    virtual uint32_t GetEmissionInterval() const override {
      return HEART_RATE_TASK_PERIOD;
    }

    // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf
    //  environmental sensing service
    virtual const ble_uuid16_t& GetServiceUUID() const override { return _serviceUUID ; }
    // temperature
    virtual const ble_uuid16_t &GetCharacteristicUUID() const {
      return _characteristicUUID;
    }

    virtual int AccessData(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg) override {
        _value= 60 + (uint8_t)(esp_random() % 21);
        return _value;
    }

  private:
    float _value;
    ble_uuid16_t _serviceUUID;
    ble_uuid16_t _characteristicUUID;
};

#endif //BLE_DUMMYEMITTER
