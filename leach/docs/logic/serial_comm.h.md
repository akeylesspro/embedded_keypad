# serial_comm.h

## Purpose
Declares the serial communication interface for text and display messages.

## Public API
- `serial_comm_init`
- `serial_comm_send_text`
- `serial_comm_process_rx`
- `DisplayMessage`
- `DisplayMessageWithTime`
- `DisplayMessageWithNoLock`
- `terminal_buffer_lock`
- `terminal_buffer_release`

## Notes
- The implementation is expected elsewhere in the codebase.
- `alert_str` is declared as an external buffer.
