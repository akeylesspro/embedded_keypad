# nrf_ble_gq.c

## Purpose
Implements the Nordic BLE GATT Queue (GQ). It queues and serializes GATT client/server requests to avoid SoftDevice busy errors and manage memory for request payloads.

## Main responsibilities
- Provide a queue for GATT operations per connection.
- Allocate and free memory for write and HVX payloads.
- Retry or defer requests when SoftDevice is busy.
- Process queued requests on relevant BLE events.

## Key behaviors
- `nrf_ble_gq_item_add`: queues a request or sends immediately if possible.
- `queue_process`: dispatches the next request in the queue.
- `nrf_ble_gq_conn_handle_register`: registers a connection and initializes pools.
- `nrf_ble_gq_on_ble_evt`: processes queues on GATT events and purges on disconnect.

## Request types handled
- GATTC read/write
- Primary service discovery
- Characteristic discovery
- Descriptor discovery
- GATTS HVX (notification/indication)

## Memory management
- Uses `nrf_memobj_pool` to store queued payloads.
- Allocators:
  - `gattc_write_alloc` for writes.
  - `gatts_hvx_alloc` for notifications/indications.

## Notes
- Standard Nordic SDK implementation with logging enabled.
