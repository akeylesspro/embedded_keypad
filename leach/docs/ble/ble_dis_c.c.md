# ble_dis_c.c

## Purpose
Implements the Device Information Service (DIS) client. It discovers DIS characteristics on a peer and reads their values.

## Main responsibilities
- Decode DIS characteristic values from read responses.
- Track handles for DIS characteristics discovered via DB discovery.
- Dispatch read responses and errors to the application handler.
- Integrate with the BLE GATT queue for read requests.

## Key behaviors
- `ble_dis_c_init` registers DIS with the DB discovery module.
- `ble_dis_c_on_db_disc_evt` maps discovered characteristics to handles.
- `ble_dis_c_read` sends GATT read requests via the GATT queue.
- `ble_dis_c_on_ble_evt` handles read responses and disconnects.

## Decoding logic
- `system_id_decode` and `pnp_id_decode` validate and parse DIS data.
- For string-based characteristics, the response is passed as raw bytes.
- Calls `decode_peripheral_device_by_dis(...)` to classify peripherals.

## Error handling
- Uses `gatt_error_handler` to forward errors to the application.
- Validates read response length and reports invalid length errors.

## Dependencies
- `ble_db_discovery` for discovery flow.
- `nrf_ble_gq` for queued GATT operations.
- `ble_services_manager` for peripheral decoding hook.

## Notes
- Several DIS characteristics (System ID, PnP ID) are currently disabled via comments.
