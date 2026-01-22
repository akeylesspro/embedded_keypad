# state_machine.h

## Purpose
Public interface for the state machine module that manages watchdog feeding and time tracking.

## Public API
- `state_machine_init`: initialize WDT and RTC.
- `state_machine_feed_watchdog`: feed the watchdog channel.
- `get_time_in_seconds`: return seconds since the local 2020 epoch.

## Notes
- Timekeeping is local; the epoch baseline is set elsewhere (e.g., via configuration).
