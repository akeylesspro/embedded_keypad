# Ruptela.h

## Purpose
Declares the Ruptela service simulation types, constants, and public API. This header is only active when `RUPTELA_SERVICE_SIMULATION` is enabled.

## Data types
- `Ruptela_Log`
  - Structured log record with timestamp, log code, sub-code, and two values.
- `Ruptela_Command`
  - Parsed command payload with command code, sub-code, and three values.

## Log code constants
Defines log codes such as:
- `RUPTELA_LOG_DISARMED`
- `RUPTELA_LOG_PASSWORD_CHANGED`
- `RUPTELA_LOG_LEACH_CONNECTED`
- `RUPTELA_LOG_ENDED_PAIRING`

These are encoded as `0xAAAABBBB` where `AAAA` is the log code and `BBBB` is the sub-code.

## Command code constants
Defines command codes such as:
- `RUPTELA_COMMAND_DISARM`, `RUPTELA_COMMAND_ARM`
- `RUPTELA_COMMAND_GET_PASSWORD`, `RUPTELA_COMMAND_NEW_PASSWORD`
- `RUPTELA_COMMAND_SET_TIME`, `RUPTELA_COMMAND_ENABLE_PAIRING`

These are encoded as `0xAAAABBBB` where `AAAA` is the command code and `BBBB` is the sub-code.

## Public API
- `SecurityState read_new_security_state(void)`
- `void ruptela_send_log(uint32_t log_code, uint8_t* value1, uint8_t* value2, uint16_t sub_code)`
- `void ruptela_decode_command(uint8_t* command_message, Ruptela_Command* command)`
- `bool getEnablePairing(void)`
- `void resetEnablePairing(void)`

## Notes
- The header assumes `SecurityState` is defined in the included configuration headers.
- All declarations are guarded by `#ifdef RUPTELA_SERVICE_SIMULATION`.
