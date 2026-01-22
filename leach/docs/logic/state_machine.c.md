# state_machine.c

## Purpose
Implements a lightweight state/timekeeper module using the watchdog timer (WDT) and RTC. It tracks time since 2020 and triggers periodic monitoring.

## Main responsibilities
- Initialize WDT and RTC for system reliability and timing.
- Feed the watchdog from application logic.
- Maintain a `time_since_2020` counter (seconds).
- Update `monitor_timer` and call `monitor_task_check` periodically.

## Key functions
- `state_machine_init`: initializes WDT and RTC.
- `feed_watchdog` / `state_machine_feed_watchdog`: feed the WDT channel.
- `get_time_in_seconds`: returns seconds since 2020 epoch (local counter).

## RTC handler
- Uses RTC2 with prescaler 4095.
- Increments `time_since_2020` every 8 ticks.
- Logs time every 10 seconds.
- Updates `monitor_timer` and triggers monitor checks at `MONITOR_TEST_TIME`.

## Dependencies
- `nrfx_wdt`, `nrfx_rtc` for hardware timers.
- `logic/monitor.h` for monitoring hooks.
- NRF logging for diagnostics.

## Notes
- The time counter is local and not synchronized to real time unless set elsewhere.
