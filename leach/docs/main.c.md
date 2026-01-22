# main.c

## Purpose
This file is the application entry point for the Leach keypad firmware. It initializes core modules (logging, clocks, timers), sets up tasks and synchronization objects, and starts the FreeRTOS scheduler.

## Main responsibilities
- Initialize logging and low-frequency clock sources.
- Track reset count across resets using `.non_init` memory.
- Initialize system timers and peripherals.
- Create core FreeRTOS tasks (logger, monitor, BLE, keypad).
- Initialize synchronization primitives (semaphores and event groups).
- Start the FreeRTOS scheduler and handle fatal states.

## Key flow in `main()`
- Initialize logging (`log_init`) and clock driver (`clock_init`).
- Use `test_value` in `.non_init` RAM to detect first power-up vs. warm reset.
- Seed time on first power-up (`SetTimeFromString`).
- Create the logger task to flush deferred logs.
- Enable deep sleep (`SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk`).
- Initialize modules and print firmware metadata.
- Initialize peripherals and state machine.
- Create the monitor, BLE, and keypad tasks.
- Call `init_tasks()` to create semaphores, event groups, and initialize BLE/keypad/config.
- Start the FreeRTOS scheduler (`vTaskStartScheduler`).

## Important functions
- `assert_nrf_callback`: SoftDevice assert handler, forwards to `app_error_handler`.
- `modules_init`: Initializes the app timer module.
- `sleep_mode_enter`: Enters system-off mode (no return).
- `log_init`: Initializes NRF logging backends.
- `logger_thread`: Processes deferred logs and suspends until resumed by idle hook.
- `vApplicationIdleHook`: Resumes the logger thread when idle.
- `init_tasks`: Creates semaphores/event groups and initializes BLE/config/keypad tasks.
- `send_event_from_isr`: Sets event group bits from ISR and yields if needed.

## Concurrency and synchronization
- Uses `xSemaphoreCreateBinary` for `clock_semaphore`, `connection_list_semaphore`,
  and `watchdog_monitor_semaphore`.
- Uses `xEventGroupCreate` for `event_ignition_characteristic_changed` and
  `event_ble_connection_changed`.

## Notable globals
- `reset_count` and `test_value` are placed in `.non_init` to survive resets.
- `power_up` indicates first boot after power-cycle.
- Task handles for logger, BLE, monitor, and keypad threads.

## Dependencies and interactions
- BLE initialization flows into `ble_task` and service manager.
- Peripheral initialization uses HAL (`hal_boards`, `buzzer`).
- State machine initialization sets up WDT/RTC tracking.

## Notes
- Some log calls are commented out or marked with placeholders.
- Task stack sizes are fixed at `256` for multiple tasks; ensure FreeRTOS config is
  aligned with this usage.
