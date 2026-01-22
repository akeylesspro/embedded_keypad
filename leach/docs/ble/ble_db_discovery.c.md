# ble_db_discovery.c

## Purpose
Implements Nordic's Database Discovery module for GATT service discovery on a peer device. It discovers services, characteristics, and descriptors, and forwards results to registered application handlers.

## Main responsibilities
- Maintain a registry of service UUIDs and a shared event handler.
- Start primary service discovery and iterate over services.
- Discover characteristics and descriptors within each service.
- Collect handles into a `ble_gatt_db_srv_t` structure.
- Notify the application when discovery is complete, failed, or unavailable.

## Core flow
- `ble_db_discovery_init` registers the event handler and GATT queue.
- `ble_db_discovery_evt_register` registers UUIDs to discover.
- `ble_db_discovery_start` begins discovery for a connection handle.
- GATT responses are handled in `ble_db_discovery_on_ble_evt`.
- Each response leads to characteristic and descriptor discovery steps.
- When all services are processed, an "available" event is raised.

## Key internal helpers
- `registered_handler_get/set`: manage UUID registration.
- `discovery_start`: queues the first primary service discovery request.
- `characteristics_discover`: discovers characteristics in current service.
- `descriptors_discover`: discovers descriptors in current characteristic range.
- `discovery_error_handler`: handles SoftDevice and queue errors.

## Event handling
Events raised to the app:
- `BLE_DB_DISCOVERY_COMPLETE`
- `BLE_DB_DISCOVERY_SRV_NOT_FOUND`
- `BLE_DB_DISCOVERY_ERROR`
- `BLE_DB_DISCOVERY_AVAILABLE`

## Dependencies and interactions
- Uses `nrf_ble_gq` to queue GATT operations.
- Depends on `ble_srv_common`, `ble_gattc`, and SoftDevice APIs.

## Notes
- `BLE_DB_DISCOVERY_MAX_SRV` and `BLE_GATT_DB_MAX_CHARS` limit discovery depth.
- This is standard Nordic SDK code with minor project integration.
