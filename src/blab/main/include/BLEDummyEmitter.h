#ifndef BLE_DUMMYEMITTER
#define BLE_DUMMYEMITTER

#include "BLEEmitter.h"

#include "esp_log.h"
#include "esp_random.h"

#include "host/ble_gap.h"

// for portTICK_PERIOD_MS
#include <freertos/FreeRTOS.h>


#include <string>

#define SUBTAG "REMOVE"

#define DUMMY_TASK_PERIOD (1000 / portTICK_PERIOD_MS)

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
        return DUMMY_TASK_PERIOD;
    }
    virtual void Emit() override {
        // wait for initialization
        if (IsConnectionHandleSet()) {
            ESP_LOGI(SUBTAG, "BLEDummyEmitter::Emit ble_gatts_indicate");
            ble_gatts_indicate(GetConnectionHandle(), *GetCharacteristicHandle());
        } else {
            ESP_LOGI(SUBTAG, "BLEDummyEmitter::Emit not ready to emit");
        }
    }

    virtual void SubscribeHandler(struct ble_gap_event *event) override{
        // is this my subscribe?
        if (event->subscribe.attr_handle == *GetCharacteristicHandle()) {
            SetConnectionHandle(event->subscribe.conn_handle);
            ESP_LOGI(SUBTAG, "BLEDummyEmitter::subscrive.cur_indicate: %d", event->subscribe.cur_indicate);
            //heart_rate_ind_status = event->subscribe.cur_indicate;
        } else {
          ESP_LOGI(SUBTAG,
                   "BLEDummyEmitter::subscribe - not my characteristic: %d",
                   event->subscribe.attr_handle);
        }
    }
    // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf
    //  environmental sensing service
    virtual ble_uuid16_t& _GetServiceUUID() override { return _serviceUUID ; }
    // temperature
    virtual ble_uuid16_t &_GetCharacteristicUUID() override {
      return _characteristicUUID;
    }

    // doesn't need to be public
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
