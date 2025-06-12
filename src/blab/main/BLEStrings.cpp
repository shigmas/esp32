#include "BLEStrings.h"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"

std::string GetGAPEventType(struct ble_gap_event *event) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        return "GAPEventConnect";
    case BLE_GAP_EVENT_DISCONNECT:
        return "GAPEventDisConnect";
    case BLE_GAP_EVENT_CONN_UPDATE:
        return "GAPEventConnectUpdate";
    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        return "GAPEventConnectUpdateRequest";
    case BLE_GAP_EVENT_L2CAP_UPDATE_REQ:
        return "GAPEventL2CAPUpdateRequest";
    case BLE_GAP_EVENT_TERM_FAILURE:
        return "GAPEventTerm(ination?)Failure";
    case BLE_GAP_EVENT_DISC:
        return "GAPEventDiscovery";
    case BLE_GAP_EVENT_DISC_COMPLETE:
        return "GAPEventDiscoveryComplete";
    case BLE_GAP_EVENT_ADV_COMPLETE:
        return "GAPEventAdvertisingComplete";
    case BLE_GAP_EVENT_ENC_CHANGE:
        return "GAPEventEncChange";
    case BLE_GAP_EVENT_PASSKEY_ACTION:
        return "GAPEventPasskeyAction";
    case BLE_GAP_EVENT_NOTIFY_RX:
        return "GAPEventNotifyReceive";
    case BLE_GAP_EVENT_NOTIFY_TX:
        return "GAPEventNotifyTansmit";
    case BLE_GAP_EVENT_SUBSCRIBE:
        return "GAPEventSubscribe";
    case BLE_GAP_EVENT_MTU:
        return "GAPEventMTUUpdate";
    case BLE_GAP_EVENT_IDENTITY_RESOLVED:
        return "GAPEventIdentityResolved";
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        return "GAPEventRepeatPairing";
    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
        return "GAPEventPhyUpdateComplete";
    case BLE_GAP_EVENT_EXT_DISC:
        return "GAPEventExtDiscovery";
    case BLE_GAP_EVENT_PERIODIC_SYNC:
        return "GAPEventPeriodicSync";
    case BLE_GAP_EVENT_PERIODIC_REPORT:
        return "GAPEventPeriodicReport";
    case BLE_GAP_EVENT_PERIODIC_SYNC_LOST:
        return "GAPEventSyncLost";
    case BLE_GAP_EVENT_SCAN_REQ_RCVD:
        return "GAPEventScanRequestReceived";
    case BLE_GAP_EVENT_PERIODIC_TRANSFER:
        return "GAPEventPeriodicTransfer";
    case BLE_GAP_EVENT_PATHLOSS_THRESHOLD:
        return "GAPEventPathLossThreshold";
    case BLE_GAP_EVENT_TRANSMIT_POWER:
        return "GAPEventTransmitPower";
    case BLE_GAP_EVENT_PARING_COMPLETE:
        return "GAPEventPairingComplete";
    case BLE_GAP_EVENT_SUBRATE_CHANGE:
        return "GAPEventSubRateChanged";
    case BLE_GAP_EVENT_VS_HCI:
        return "GAPEventVsHCI";
    case BLE_GAP_EVENT_BIGINFO_REPORT:
       return "GAPEventBigInfoReport";
    case BLE_GAP_EVENT_REATTEMPT_COUNT:
        return "GAPEventReatTemptCount";
    case BLE_GAP_EVENT_AUTHORIZE:
        return "GAPEventAuthorize";
    case BLE_GAP_EVENT_TEST_UPDATE:
        return "GAPEventTestUpdate";
    case BLE_GAP_EVENT_DATA_LEN_CHG:
        return "GAPEventDataLengthChange";
    case BLE_GAP_EVENT_CONNLESS_IQ_REPORT:
        return "GAPEventConnLossIQReport";
    case BLE_GAP_EVENT_CONN_IQ_REPORT:
        return "GAPEventConnIQReport";
    case BLE_GAP_EVENT_CTE_REQ_FAILED:
        return "GAPEventCteReqFailed";
    case BLE_GAP_EVENT_LINK_ESTAB:
        return "GAPEventLinkEstablished";
    case BLE_GAP_EVENT_EATT
        return "GAPEventEAtt";
    case BLE_GAP_EVENT_PER_SUBEV_DATA_REQ:
        return "GAPEventPerSubDataRequest";
    case BLE_GAP_EVENT_PER_SUBEV_RESP:
        return "GAPEventPerSubEvResponse";
    case BLE_GAP_EVENT_PERIODIC_TRANSFER_V2:
        return "GAPEventPeriodicTransferV2";
    }
    return "";
}

std::string GetGATTRegisterOp(struct ble_gatt_register_ctxt *ctx) {
    switch (ctx->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        return "GATTRegisterOpService";
    case BLE_GATT_REGISTER_OP_CHR:
        return "GATTRegisterOpCharacteristic";
    case BLE_GATT_REGISTER_OP_DSC:
        return "GATTRegisterOpDescriptor";
    }
    return "";
}
