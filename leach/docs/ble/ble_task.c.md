# ble_task.c

## Purpose
Implements the BLE task thread and connection/bonding state management for Leach devices. It orchestrates scanning, connection verification, bonding workflows, and ignition state updates.

## Main responsibilities
- Initialize BLE task state and connection tracking structures.
- Run the BLE FreeRTOS thread (`ble_thread`).
- Manage bonding state, connection list, and device verification.
- Handle Ruptela command messages in simulation mode.
- Update ignition state for connected peripherals.

## Task behavior (`ble_thread`)
- Waits for configuration to load before starting BLE services.
- Starts central BLE services and scanning.
- Reacts to connection and disconnection events via event groups.
- Verifies or removes peripherals based on connection events.
- Handles bonding LED feedback and pairing state.
- Processes Ruptela commands when simulation is enabled.

## Connection and bonding management
- `save_bondings`: persists bonded device list and sends logs.
- `verify_peripheral`: validates newly connected devices and accepts or rejects them.
- `remove_peripheral`: marks disconnected devices in the connection list.
- `disconnect_and_delete_all_peripheral`: disconnects and clears all bonded devices.
- `activate_bonding_state` / `de_activate_bonding_state`: enable/disable bonding.

## Data structures and globals
- `connections_array`: list of known peripherals.
- `bonded_count`, `connected_count`, `leach_bonded_count`, `modem_bonded_count`.
- `in_bonding_state`, `bonding_process_start_time`.
- `current_conn_info`: current connection metadata.
- `connection_list_semaphore`: protects shared connection data.

## Dependencies
- BLE services manager for scanning and connection updates.
- Configuration module for address list management.
- Ruptela simulation (optional) for command handling and logs.
- HAL/buzzer for user feedback.

## Notes
- Uses event groups `event_ble_connection_changed` and
  `event_ignition_characteristic_changed` for async updates.
- Several behaviors are project-specific (e.g., allow pairing based on app flag).
