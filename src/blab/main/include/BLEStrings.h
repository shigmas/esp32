#include "host/ble_gap.h"

#include <string>

struct ble_gap_event;
struct ble_gatt_register_ctxt;

std::string GetGAPEventType(struct ble_gap_event *event);
std::string GetGATTRegisterOp(struct ble_gatt_register_ctxt *ctx);
