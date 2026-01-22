# Ruptela.c

## Purpose
Implements the Ruptela service simulation logic used when `RUPTELA_SERVICE_SIMULATION` is enabled. It parses inbound commands, manages a security state, and sends structured log records back to the simulated peer.

## Main responsibilities
- Build and send Ruptela log records (`Ruptela_Log`) to the transport layer.
- Decode command payloads into `Ruptela_Command` and dispatch actions.
- Maintain a security state machine (`SECURITY_STATE_*`) protected by a semaphore.
- Validate and update keypad password configuration.
- Support pairing enable/disable flow via `enablePairing`.

## Key functions
- `ruptela_send_log(...)`
  - Builds a log record with timestamp and data values.
  - Splits `log_code` into major code/subcode fields.
  - Sends the log through `send_log`.
- `ruptela_decode_command(...)`
  - Parses a 16-byte command message into fields.
  - Handles command codes for arm/disarm, password get/set, default reset, time set.
  - Updates configuration and emits log responses.
- `read_new_security_state()`
  - Reads and clears the current security state (thread-safe).
- `getEnablePairing()` / `resetEnablePairing()`
  - Expose and reset the pairing enable flag.
- `is_valid_password(...)`
  - Verifies password digits are in the allowed ASCII range `0x31-0x35` (digits 1-5).

## Important data and synchronization
- `security_state_semaphore` guards `security_state`.
- `enablePairing` toggles pairing flow for Leach connections.
- `time_since_2020` is used for log timestamps.

## Dependencies and interactions
- Uses `configuration_save_with_crc()` and `configuration_set_default()` to persist config changes.
- Calls `success_buzzer()` and other feedback functions for user signals.
- Reads/writes `eprom_configuration.password`.
- Reports to the monitoring/logging path using `NRF_LOG_*`.

## Notes
- The module is compiled only when `RUPTELA_SERVICE_SIMULATION` is defined.
- Command payload sizes and parsing offsets are fixed and must match sender format.
