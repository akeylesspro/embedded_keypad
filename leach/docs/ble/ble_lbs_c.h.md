# ble_lbs_c.h

## Purpose
Public API and type definitions for the LED Button Service (LBS) client.

## UUID and characteristic IDs
- Base UUID: `LBS_UUID_BASE`
- Service UUID: `LBS_UUID_SERVICE`
- Button characteristic: `LBS_UUID_BUTTON_CHAR`
- LED characteristic: `LBS_UUID_LED_CHAR`
- Simulation-only:
  - Command characteristic: `LBS_UUID_COMMAND_CHAR`
  - Log characteristic: `LBS_UUID_LOG_CHAR`

## Key types
- `ble_lbs_c_t`: client instance state (handles, UUID type, callbacks).
- `lbs_db_t`: peer handles for button/LED (and command/log in simulation).
- `ble_lbs_c_evt_t`: event payload for discovery and notifications.

## Public API
- `ble_lbs_c_init`
- `ble_lbs_c_on_ble_evt`
- `ble_lbs_on_db_disc_evt`
- `ble_lbs_c_button_notif_enable`
- `ble_lbs_c_handles_assign`
- `ble_lbs_led_status_send`
- `ble_lbs_log_send`

## Notes
- Includes `NRF_SDH_BLE_OBSERVER` macros for instance registration.
- Simulation-specific fields are guarded by `RUPTELA_SERVICE_SIMULATION`.
