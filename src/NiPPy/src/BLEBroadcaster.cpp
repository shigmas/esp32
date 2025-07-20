
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

// ble_store_config_init
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "CallbackDef.h"

#include "nvs_flash.h"

#include "BLEEmitter.h"
#include "BLEStrings.h"

#define TAG "Broadcaster"

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


inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}

BLEBroadcaster::BLEBroadcaster(const std::string &name) : _name(name) {}

BLEBroadcaster::~BLEBroadcaster()
{
    ESP_LOGI(TAG,"BLEBroadcaster::~BLEBroadcaster()");
    delete [] _gatt_svr_svcs;
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

    ESP_ERROR_CHECK(_GAPInit());
    ESP_ERROR_CHECK(_GATTInit());
    ESP_ERROR_CHECK(_HostConfig());
    
    return;
}

void BLEBroadcaster::AddEmitter(BLEEmitter *emitter) {
    _emitters.push_back(emitter);
}

uint32_t BLEBroadcaster::GetBroadcastInterval() const {
    uint32_t min = UINT32_MAX;
    for (const auto& emitter : _emitters) {
        if (emitter->GetEmissionInterval() < min) {
            min = emitter->GetEmissionInterval();
        }
    }
    return min;
}

void BLEBroadcaster::Broadcast() {
    for (const auto& emitter : _emitters) {
        emitter->Emit();
    }
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
    // the functions that we pass this variable to check for the end by looking for a 0 trailing
    // element
    _gatt_svr_svcs = new ble_gatt_svc_def[_emitters.size()+1];
    
    int i = 0;
    for (const auto& emitter : _emitters) {
        _gatt_svr_svcs[i] = emitter->GetService();
        i++;
    }
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    _gatt_svr_svcs[i] = {0};

    ret = ble_gatts_count_cfg(_gatt_svr_svcs);
    if (ret != 0) {
        return ret;
    }

    ret = ble_gatts_add_svcs(_gatt_svr_svcs);
    if (ret != 0) {
        return ret;
    }
    for (const auto& emitter : _emitters) {
        ESP_LOGI(TAG,"GATTInit: emitter val handle: %d", *emitter->GetCharacteristicHandle());
        i++;
    }
    
    return ret;
}

int BLEBroadcaster::_HostConfig() {

    // bind the member functions for callbacks
    Callback<void(int)>::func = std::bind(&BLEBroadcaster::_OnStackReset, this, std::placeholders::_1);
    ble_hs_cfg.reset_cb = Callback<void(int)>::callback;
    Callback<void()>::func = std::bind(&BLEBroadcaster::_OnStackSync, this);
    ble_hs_cfg.sync_cb = Callback<void()>::callback;
    Callback<void(struct ble_gatt_register_ctxt *ctxt, void *arg)>::func = std::bind(&BLEBroadcaster::_GATTRegisterHandler, this, std::placeholders::_1, std::placeholders::_2);
    ble_hs_cfg.gatts_register_cb = Callback<void(struct ble_gatt_register_ctxt *ctxt, void *arg)>::callback;
    //ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
    return 0;
}

void BLEBroadcaster::_OnStackReset(int reason) {
    ESP_LOGI(TAG, "_OnStackReset: %d", reason);
}

void BLEBroadcaster::_OnStackSync() {
    ESP_LOGI(TAG, "StackSync. Starting Advertising");

    _InitAdvertising();
    _StartAdvertising();
}

int BLEBroadcaster::_GAPEventHandler(struct ble_gap_event *event, void *arg)
{
    ESP_LOGI(TAG, "GAP Event Handler");
    int rc = 0;
    //struct ble_gap_conn_desc desc;

    /* Handle different GAP event */
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:

        ESP_LOGI(TAG, "connection %s; status=%d, handle: %d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status, event->connect.conn_handle);
        _connectionHandle = event->connect.conn_handle;
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        /* A connection was terminated, print connection descriptor */
        ESP_LOGI(TAG, "disconnected from peer; reason=%d",
                 event->disconnect.reason);
        _StartAdvertising();

        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        ESP_LOGI(TAG, "connection updated; status=%d",
                 event->conn_update.status);
        _StartAdvertising();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        /* Advertising completed, restart advertising */
        ESP_LOGI(TAG, "advertise complete; reason=%d",
                 event->adv_complete.reason);
        _StartAdvertising();
        break;
    case BLE_GAP_EVENT_NOTIFY_TX:
        ESP_LOGI(TAG, "Notify");
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        /* Print subscription info to log */
        ESP_LOGI(TAG,
                 "subscribe event; conn_handle=%d attr_handle=%d "
                 "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle,
                 event->subscribe.reason, event->subscribe.prev_notify,
                 event->subscribe.cur_notify, event->subscribe.prev_indicate,
                 event->subscribe.cur_indicate);

        /* GATT subscribe event callback */
        for (const auto& emitter : _emitters) {
            emitter->SubscribeHandler(event);
        }

        break;
     case BLE_GAP_EVENT_MTU:
        /* Print MTU update info to log */
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                 event->mtu.conn_handle, event->mtu.channel_id,
                 event->mtu.value);
        break;
    default:
        ESP_LOGI(TAG, "unhandled GAP event: %d (%s)",event->type,GetGAPEventType(event).c_str());
    }
    return rc;
}

void BLEBroadcaster::_GATTRegisterHandler(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[128];
    ble_uuid16_t *uuid16 = NULL;

    // This is kind of like a state machine. The service is registered, followed by the
    // characteristics, then the next service and its characteristics.
    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        /* Service register event */
        // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf
        // pg 69
        ESP_LOGI(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
        if (ctxt->svc.svc_def->uuid->type == BLE_UUID_TYPE_16) {
            uuid16 = (ble_uuid16_t *)ctxt->svc.svc_def->uuid;
        }
        for (auto emitter : _emitters) {
            //            if ((uuid16 != NULL) && (emitter->GetServiceUUID().value == uuid16->value)) {
                emitter->RegisterServiceHandle(uuid16->value, ctxt->svc.handle);
                //            }
        }
        break;
    case BLE_GATT_REGISTER_OP_CHR:
        /* Characteristic register event */
        // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf
        // pg 96
        ESP_LOGI(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        for (auto emitter : _emitters) {
            BlabError err = emitter->SetCharacteristicHandlesForUUID(ctxt->chr.chr_def->uuid,
                                                                     ctxt->chr.def_handle,
                                                                     ctxt->chr.val_handle);
            if (err) {
                ESP_LOGI(TAG, "unable to store handles: %s", err->String().c_str());
            }
        }

        break;
    case BLE_GATT_REGISTER_OP_DSC:
        /* Descriptor register event */
        ESP_LOGI(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf), ctxt->dsc.handle);
        break;
    default:
        ESP_LOGI(TAG, "Unhandled registration event: %s", GetGATTRegisterOp(ctxt).c_str());
        /* Unknown event */
        break;
    }
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
    ESP_LOGI(TAG, "InitAdvertising: device address: %s", addr_str);
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

    /* Set advertisement fields */
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
    Callback<int(struct ble_gap_event *event, void *arg)>::func = std::bind(&BLEBroadcaster::_GAPEventHandler, this, std::placeholders::_1, std::placeholders::_2);
    rc = ble_gap_adv_start(_ownAddrType, NULL, BLE_HS_FOREVER, &adv_params, Callback<int(struct ble_gap_event *event, void *arg)>::callback, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "StartAdvertising: advertising started!");
}
