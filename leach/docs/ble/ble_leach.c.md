# ble_leach.c

## Purpose
Defines basic BLE identifiers for the Leach custom service. This file primarily declares UUID constants and logging configuration.

## Key contents
- Module log configuration (`NRF_LOG_MODULE_NAME`, `NRF_LOG_LEVEL`).
- 16-bit characteristic UUIDs:
  - `BLE_UUID_COMMAND_CHARACTERISTIC`
  - `BLE_UUID_IGNITION_CHARACTERISTIC`
- 128-bit base UUID for the Leach service (`ILEACH_BASE_UUID`).

## Notes
- This file is minimal and serves as a shared place for UUID constants.
