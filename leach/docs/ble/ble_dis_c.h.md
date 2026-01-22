# ble_dis_c.h

## Purpose
Public API and types for the Device Information Service (DIS) client.

## Key types
- `ble_dis_c_t`: client instance state (connection handle, handles array, callbacks, GATT queue).
- `ble_dis_c_evt_t`: event container for discovery, read responses, and errors.
- `ble_dis_c_char_type_t`: supported DIS characteristics (manufacturer, model, serial, hw/fw/sw rev).
- `ble_dis_c_init_t`: initialization parameters.

## Macros
- `BLE_DIS_C_DEF` / `BLE_DIS_C_ARRAY_DEF`: define client instances and register BLE observers.

## Public API
- `ble_dis_c_init`: initialize and register DIS discovery.
- `ble_dis_c_on_db_disc_evt`: handle DB discovery events.
- `ble_dis_c_on_ble_evt`: handle BLE events relevant to DIS.
- `ble_dis_c_read`: read a specific DIS characteristic.
- `ble_dis_c_handles_assign`: assign handles after discovery.

## Notes
- Some DIS characteristic types are commented out (System ID, PnP ID).
- Requires BLE and SoftDevice headers and the GATT queue module.
