# ble_services_manager.h

## Purpose
Public interface for BLE services management. Exposes initialization, scanning controls, and connection helpers.

## Public API
- Initialization:
  - `ble_services_init_0`
  - `ble_services_init_peripheral`
  - `ble_services_init_central`
  - `ble_services_advertising_start`
- Scanning controls:
  - `scan_start`, `scan_stop`
  - `scan_init_by_white_list`, `scan_init_by_uuid`
- Connection status:
  - `ble_services_is_connected`
- Utility:
  - `decode_peripheral_device_by_dis`

## Data structures
- `ble_peripheral_info_t`
  - Tracks ignition state, connection time, peer address, connection handle,
    peripheral type, and flags for connection state and ignition changes.

## Dependencies
- BLE GAP types (`ble_gap_addr_t`) and DIS types for device decoding.

## Notes
- `ble_services_init_peripheral` is declared but implemented elsewhere.
