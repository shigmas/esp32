#ifndef BLE_DUMMYEMITTER
#define BLE_DUMMYEMITTER

#include "BLEEmitter.h"

#include "esp_log.h"
#include "esp_random.h"

// for portTICK_PERIOD_MS
#include <freertos/FreeRTOS.h>


#include <string>

#define SUBTAG "REMOVE"

#define HEART_RATE_TASK_PERIOD (1000 / portTICK_PERIOD_MS)

class BLEDummyEmitter : public BLEEmitter {

    typedef BLEEmitter super;

public:
    BLEDummyEmitter(const std::string &name)
        // NimBLE macros to convert raw UUIDs into the variables
        : super(name),
          _serviceUUID(BLE_UUID16_INIT(0x181A)),
          _characteristicUUID(BLE_UUID16_INIT(0x2A6E)) {}
    virtual ~BLEDummyEmitter() {}

    virtual uint32_t GetEmissionInterval() const override {
        return HEART_RATE_TASK_PERIOD;
    }

    // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf
    //  environmental sensing service
    virtual ble_uuid16_t& _GetServiceUUID() override { return _serviceUUID ; }
    // temperature
    virtual ble_uuid16_t &_GetCharacteristicUUID() override {
      return _characteristicUUID;
    }

    virtual int AccessData(uint16_t conn_handle, uint16_t attr_handle,
                           ble_gatt_access_ctxt *ctxt, void *arg) override {
        ESP_LOGI(SUBTAG, "BLEDummyEmitter::AccessData() callback");
        _value= 60 + (uint8_t)(esp_random() % 21);
        ESP_LOGI(SUBTAG, "BLEDummyEmitter::AccessData() - %d", _value);
        return _value;
    }

  private:
    float _value;
    // typedef struct
    // {
    //    ble_uuid_t u;
    //    uint16_t value;
    // } ble_uuid16_t;
    ble_uuid16_t _serviceUUID;
    ble_uuid16_t _characteristicUUID;
};

#endif //BLE_DUMMYEMITTER
