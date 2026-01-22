# peripherals.c

## Purpose
Implements board-level peripheral helpers for the keypad: initialization, LEDs, ignition control, and basic sensor readings.

## Main responsibilities
- Initialize HAL and read device serial number.
- Control LED outputs and read ignition switch state.
- Read temperature and battery voltage.
- Expose helper functions for LED control.

## Key functions
- `peripherals_init`
  - Reads device serial number and initializes HAL.
  - Logs the serial number.
- `peripherals_read_temperature`
  - Reads internal temperature via `sd_temp_get` and converts to degrees.
- `peripherals_read_vdd`
  - Reads VDD via HAL ADC and converts to mV.
- `peripherals_read_battery_level`
  - Converts VDD to a battery percentage using `battery_level_in_percent`.
- `peripheral_turn_on_all_led` / `peripheral_turn_off_all_led`
  - Drive all keypad LEDs; optionally include status LED.
- `peripheral_is_switch_ignition_on`
  - Reads ignition input pin and returns boolean state.

## Dependencies
- HAL for GPIO, ADC, and serial number access.
- Configuration for battery conversion logic.
- BLE services manager for integration in higher-level flow.

## Notes
- Some HAL event handling and serial comm hooks are currently commented out.
- LED control uses active-low logic (`pin_clear` to turn on).
