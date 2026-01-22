# ble_leach.h

## Purpose
Declares constants related to the Leach custom BLE service, including characteristic sizes and ignition bit flags.

## Characteristic size limits
- `COMMAND_CHAR_MAX_LEN` (64)
- `ACC_CHAR_MAX_LEN` (16)
- `EVENTS_CHAR_MAX_LEN` (64)
- `MEASURE_CHAR_MAX_LEN` (16)
- `STATUS_CHAR_MAX_LEN` (32)
- `IGNITION_CHAR_MAX_LEN` (4)

## Ignition bit positions
These bits are used within the ignition characteristic value:
- `IGNITION_BIT_SWITCH` (0)
- `IGNITION_BIT_SWITCH_EN` (1)
- `IGNITION_BIT_RELAY_STATE` (2)
- `IGNITION_BIT_RELAY_STATE_EN` (3)
- `IGNITION_BIT_CMD_CLEAR_ALL` (8)
- `IGNITION_BIT_CMD_DEFAULT_PARAMS` (9)
- `IGNITION_BIT_CMD_RESET` (10)

## Dependencies
- Includes BLE headers and SoftDevice configuration via `sdk_config.h`.

## Notes
- This header is currently constants-only; no function prototypes are defined.
