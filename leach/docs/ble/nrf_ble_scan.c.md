# nrf_ble_scan.c

## Purpose
Implements the Nordic BLE scanning module. It provides scanning, filtering, and optional auto-connect behavior.

## Main responsibilities
- Initialize scanning parameters and connection parameters.
- Maintain scan filters (name, address, UUID, appearance, short name).
- Parse advertisement reports and determine filter matches.
- Trigger auto-connect when a filter matches and auto-connect is enabled.
- Raise scan events to the application.

## Key behaviors
- `nrf_ble_scan_init`: sets defaults or uses provided scan/conn params.
- `nrf_ble_scan_start` / `nrf_ble_scan_stop`: start/stop scanning.
- `nrf_ble_scan_filter_set`: add filters by type.
- `nrf_ble_scan_filters_enable` / `nrf_ble_scan_filters_disable`: toggle filters.
- `nrf_ble_scan_on_ble_evt`: entry point for BLE events.

## Filter logic
- Supports multiple filters and "match all" mode.
- Address filtering can compare against a whitelist of addresses.
- UUID matching can be combined with MAC filtering (project-specific).

## Project-specific behavior
- Uses `configuration_get_address` and `use_mac_address_filter` to enforce a MAC address filter when enabled.

## Notes
- This file is largely standard Nordic SDK code with added MAC filtering logic.
