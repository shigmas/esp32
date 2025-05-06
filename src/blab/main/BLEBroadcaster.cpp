#include "BLEBroadcaster.h"

#include "esp_log.h"


#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

// for the _GAPInit() functionso
#include "host/ble_gap.h"
#include "services/gap/ble_svc_gap.h"

/* NimBLE GATT APIs */
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

// ble_store_config_init
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"


#include "nvs_flash.h"

#include <functional>

#define TAG "blab"

extern "C" {
// declare it now. It's defined later
void ble_store_config_init(void);
}

// helper template for converting std::bind functions to C
template <typename T>
struct Callback;

template <typename Ret, typename... Params>
    struct Callback<Ret(Params...)> {
    template <typename... Args> 
        static Ret callback(Args... args) {                    
        return func(args...);  
    }
    static std::function<Ret(Params...)> func; 
};

template <typename Ret, typename... Params> std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

BLEBroadcaster::BLEBroadcaster(const std::string &name) : _name(name) {}

BLEBroadcaster::~BLEBroadcaster()
{
    
}

void BLEBroadcaster::Init() {
    esp_err_t err;

    _NVSInit();
    
    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                 err);
        return;
    }

    _GAPInit();

    _GATTInit();

    _HostConfig();
    
    return;
}

int BLEBroadcaster::_NVSInit() {
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return 0;
    }

    return 1;
}

int BLEBroadcaster::_GAPInit()
{
  /* Call NimBLE GAP initialization API */
  ble_svc_gap_init();

  ESP_LOGI(TAG, "Broadcasting as [%s]",_name.c_str());
  int rc = ble_svc_gap_device_name_set(_name.c_str());
  if (rc != 0) {
    ESP_LOGE(TAG, "failed to set device name to %s, error code: %d",
             _name.c_str(), rc);
    return rc;
  }
  return 0;
}

int BLEBroadcaster::_GATTInit() {
  int ret = 0;

  ble_svc_gatt_init();

  /* 2. Update GATT services counter */
  // need to build this from our emitters
  // ret = ble_gatts_count_cfg(gatt_svr_svcs);

  return ret;
}

int BLEBroadcaster::_HostConfig() {

    // bind the member functions for callbacks
    Callback<void(int)>::func = std::bind(&BLEBroadcaster::_OnStackReset, this, std::placeholders::_1);
    ble_hs_cfg.reset_cb = Callback<void(int)>::callback;
    Callback<void()>::func = std::bind(&BLEBroadcaster::_OnStackSync, this);
    ble_hs_cfg.sync_cb = Callback<void()>::callback;
    // ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();

    return 0;
}

void BLEBroadcaster::_OnStackReset(int reason) {}
void BLEBroadcaster::_OnStackSync() {}
