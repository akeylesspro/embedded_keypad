# nrf_ble_scan.h

## Purpose
Public API and types for the Nordic BLE scanning module.

## Key types
- `nrf_ble_scan_t`: scanning instance state and configuration.
- `nrf_ble_scan_init_t`: initialization parameters.
- `scan_evt_t`: scan event payload for app callbacks.
- `nrf_ble_scan_filters_t`: active filter configuration.

## Filter types and modes
- Filter types: name, short name, address, UUID, appearance.
- `NRF_BLE_SCAN_ALL_FILTER` enables multi-filter mode.
- Each filter has capacity limits defined by `NRF_BLE_SCAN_*_CNT`.

## Public API
- `nrf_ble_scan_init`
- `nrf_ble_scan_start`
- `nrf_ble_scan_stop`
- `nrf_ble_scan_params_set`
- `nrf_ble_scan_filter_set`
- `nrf_ble_scan_filters_enable`
- `nrf_ble_scan_filters_disable`
- `nrf_ble_scan_all_filter_remove`
- `nrf_ble_scan_filter_get`
- `nrf_ble_scan_on_ble_evt`
- `nrf_ble_scan_copy_addr_to_sd_gap_addr`

## Notes
- Exposes BLE observer macro `NRF_BLE_SCAN_DEF` for instance registration.
