# ble_task.h

## Purpose
Public interface for the BLE task and connection management utilities.

## Key types
- `PeripheralType`: classification of connected peripherals (Leach, modem, iSticker, etc.).

## Public API
- Task lifecycle:
  - `void ble_thread(void *pvParameters)`
  - `void init_ble_task(void)`
- Connection and bonding:
  - `void activate_bonding_state(bool bonding_state)`
  - `void de_activate_bonding_state(void)`
  - `void disconnect_and_delete_all_peripheral(bool including_address_list_in_eprom)`
  - `void save_bondings(void)`
- Utilities:
  - `void print_address(char* text, ble_gap_addr_t *address)`
  - `bool connections_state_lock(uint32_t wait_ms)`
  - `void connections_state_unlock(void)`
  - `void set_ignition_state(uint32_t ignition_state, uint16_t connection_handle)`
  - `uint8_t blink_bonded_device_count(bool show_bonded_count, bool display_key_led)`
  - `void setLeachFound(uint8_t found)`

## Notes
- `is_peripheral_recognized` is declared but not defined in this file set.
