# app_config.h

## Purpose
Project-specific configuration overrides for the Leach firmware. It layers on top of shared configuration and defines BLE and scanning parameters used throughout the application.

## Key configuration areas
- BLE observer priority (`BLE_LEACH_BLE_OBSERVER_PRIO`).
- BLE module log level (`BLE_LEACH_BLE_LOG_LEVEL`).
- Link counts for central and peripheral roles.
- GATT Queue (GQ) enablement and sizing.
- Scanning parameters and filter capacities.

## Notable macros
- `NRF_SDH_BLE_CENTRAL_LINK_COUNT`, `NRF_SDH_BLE_PERIPHERAL_LINK_COUNT`.
- `NRF_BLE_GQ_*` sizing and max payload length.
- `NRF_BLE_SCAN_*` interval, window, duration, filters.
- `RUPTELA_SERVICE_SIMULATION` compile flag.
- `MAX_TARGET_ADDRESSES` derived from `CONSTANT_MAC_ADDRESS`.

## Notes
- Uses `shared_app_config.h` as the base.
- Designed to be edited via the "Configuration Wizard" in Nordic tools.
