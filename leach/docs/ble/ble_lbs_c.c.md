# ble_lbs_c.c

## Purpose
Implements the LED Button Service (LBS) client. It discovers the LBS service on a peer and handles notifications and writes to LED/command/log characteristics.

## Main responsibilities
- Handle notifications for button state (and command state in simulation).
- Configure CCCDs to enable notifications.
- Send LED status and log messages to the peer device.
- Initialize the LBS client and process DB discovery events.

## Key behaviors
- `ble_lbs_c_init`: registers LBS UUID and initializes client state.
- `ble_lbs_on_db_disc_evt`: maps discovered handles to local client DB.
- `ble_lbs_c_on_ble_evt`: handles HVX (notification) and disconnect events.
- `ble_lbs_c_button_notif_enable`: enables notifications via CCCD write.
- `ble_lbs_led_status_send`: writes LED state to peer characteristic.
- `ble_lbs_log_send`: sends 16-byte log messages (simulation mode).

## Notification handling
- `on_hvx` decodes:
  - Button characteristic (expects 4 bytes, maps to `button_state`).
  - Command characteristic (expects 16 bytes) when `RUPTELA_SERVICE_SIMULATION` is enabled.

## Dependencies
- `ble_db_discovery` for discovery flow.
- `nrf_ble_gq` for queued writes.
- LBS UUID base and characteristic IDs in `ble_lbs_c.h`.

## Notes
- Additional characteristics (command/log) are compiled only in simulation mode.
