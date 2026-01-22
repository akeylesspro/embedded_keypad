# ble_services_manager.c

## Purpose
Central module that initializes and manages BLE services, scanning, discovery, and connections. It coordinates advertising, GAP/GATT setup, and multiple central links.

## Main responsibilities
- Initialize SoftDevice, BLE stack, GAP, GATT, and advertising.
- Manage scanning with UUID and optional MAC address filtering.
- Handle BLE events for connections, disconnections, and timeouts.
- Initialize and coordinate DB discovery, LBS client, DIS client.
- Dispatch connection state changes to the rest of the system.
- Provide helpers to send ignition state and logs to peripherals.

## Key initialization flow
- `ble_services_init_0`: enables SoftDevice and initializes BLE stack.
- `ble_services_init_central`: sets up GATT, discovery, clients, conn params, scanning, and FreeRTOS SDH integration.
- `ble_services_advertising_start`: starts advertising in fast mode.

## Connection handling
- `ble_evt_handler` handles:
  - `BLE_GAP_EVT_CONNECTED`: assigns handles, starts DB discovery, updates connection info.
  - `BLE_GAP_EVT_DISCONNECTED`: resets state and triggers connection event.
  - GATT client/server timeouts and PHY update requests.

## Scanning and filtering
- `scan_init_by_uuid`: configures scanning for the Leach service UUID.
- `scan_init_by_white_list`: uses address filters from configuration.
- `scan_start` / `scan_stop`: starts or stops scanning with optional force flag.
- `scan_evt_handler`: logs filter matches and handles scan errors.

## Discovery and client handling
- `db_discovery_init` / `db_disc_handler`: bridge DB discovery events to LBS/DIS clients.
- `lbs_c_init` / `lbs_c_evt_handler`: enable notifications and process button/command events.
- `dis_c_init` / `ble_dis_c_evt_handler`: read DIS characteristics and classify devices.

## Data and state
- `event_ble_connection_changed` event group is raised on connect/disconnect.
- `current_conn_info` tracks the active connection metadata.
- `use_mac_address_filter` controls filtering policy at runtime.

## Dependencies
- Nordic SDK modules: advertising, connection parameters, GATT, scanning, SDH FreeRTOS.
- Application logic: configuration, ignition, peripherals, tracking, serial.

## Notes
- This module mixes central and peripheral responsibilities (advertising + scanning).
- Several project-specific behaviors are included (Ruptela device detection, iPhone handling).
