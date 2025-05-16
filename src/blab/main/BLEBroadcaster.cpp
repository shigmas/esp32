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

#include "BLEEmitter.h"

#define TAG "blab"

extern "C" {
    // declare it now. It's defined later
    void ble_store_config_init(void);
}

/* Defines */
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};

struct ble_hs_adv_fields;
struct ble_gap_adv_params;

// helper template for converting std::bind functions to C
template <typename T>
struct Callback;

template <typename Ret, typename... Params> struct Callback<Ret(Params...)> {
    template <typename... Args> 
        static Ret callback(Args... args) {                    
        return func(args...);  
    }
    static std::function<Ret(Params...)> func; 
};

template <typename Ret, typename... Params> std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}

BLEBroadcaster::BLEBroadcaster(const std::string &name) : _name(name) {}

BLEBroadcaster::~BLEBroadcaster()
{
    for (const auto& [key, value] : _emitterToHandle) {
        delete value;
    }
    
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

void BLEBroadcaster::AddEmitter(BLEEmitter *emitter) {
    _emitterToHandle[emitter] = new uint16_t;
}

uint32_t BLEBroadcaster::GetBroadcastInterval() const {
    uint32_t min = UINT32_MAX;
    for (const auto& [key, value] : _emitterToHandle) {
        if (key->GetEmissionInterval() < min) {
            min = key->GetEmissionInterval();
        }
    }
    return min;
}

void BLEBroadcaster::Broadcast() {}

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
    struct ble_gatt_svc_def gatt_svr_svcs[_emitterToHandle.size()];
  
    int i = 0;
    for (const auto& [key, value] : _emitterToHandle) {
        Callback<int(uint16_t, uint16_t, ble_gatt_access_ctxt *, void *)>::func = std::bind(&BLEEmitter::AccessData, key, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        gatt_svr_svcs[i].type = key->GetServiceType();
        gatt_svr_svcs[i].uuid = &key->GetServiceUUID().u;
        gatt_svr_svcs[i].characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &key->GetCharacteristicUUID().u,
                .access_cb =
                Callback<int(uint16_t, uint16_t, ble_gatt_access_ctxt *,
                             void *)>::callback,
                // can add a flags to the emitter
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
                .val_handle = value
            },
            {
                0, /* No more characteristics in this service. */
            },
        };
    }

    ret = ble_gatts_count_cfg(gatt_svr_svcs);
    if (ret != 0) {
        return ret;
    }
    ret = ble_gatts_add_svcs(gatt_svr_svcs);
    if (ret != 0) {
        return ret;
    }


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

void BLEBroadcaster::_OnStackSync() {
  ESP_LOGI(TAG, "StackSync. Starting Advertising");

  _InitAdvertising();
  _StartAdvertising();
}

int BLEBroadcaster::_GapEventHandler(struct ble_gap_event *event, void *arg)
{
    ESP_LOGI(TAG, "GAP Event Handler");

    return 0;
}

void BLEBroadcaster::_InitAdvertising() {
    /* Local variables */
    int rc = 0;
    char addr_str[18] = {0};

    /* Make sure we have proper BT identity address set (random preferred) */
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    /* Figure out BT address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &_ownAddrType);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    /* Printing ADDR */
    rc = ble_hs_id_copy_addr(_ownAddrType, _addrVal, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, _addrVal);
    ESP_LOGI(TAG, "device address: %s", addr_str);
}


void BLEBroadcaster::_StartAdvertising()
{
    int rc = 0;
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    /* Set advertising flags */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Set device name */
    adv_fields.name = (uint8_t *)_name.c_str();
    adv_fields.name_len = _name.length();
    adv_fields.name_is_complete = 1;

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    /* Set device appearance */
    adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
    adv_fields.appearance_is_present = 1;

    /* Set device LE role */
    adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    adv_fields.le_role_is_present = 1;

    /* Set advertiement fields */
    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    /* Set device address */
    rsp_fields.device_addr = _addrVal;
    rsp_fields.device_addr_type = _ownAddrType;
    rsp_fields.device_addr_is_present = 1;

    /* Set URI */
    rsp_fields.uri = esp_uri;
    rsp_fields.uri_len = sizeof(esp_uri);

    /* Set advertising interval */
    rsp_fields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500);
    rsp_fields.adv_itvl_is_present = 1;

    /* Set scan response fields */
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
        return;
    }

    /* Set non-connetable and general discoverable mode to be a beacon */
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    /* Set advertising interval */
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510);

    /* Start advertising */
    Callback<int(struct ble_gap_event *event, void *arg)>::func = std::bind(&BLEBroadcaster::_GapEventHandler, this, std::placeholders::_1, std::placeholders::_2);
    rc = ble_gap_adv_start(_ownAddrType, NULL, BLE_HS_FOREVER, &adv_params, Callback<int(struct ble_gap_event *event, void *arg)>::callback, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising started!");
}
