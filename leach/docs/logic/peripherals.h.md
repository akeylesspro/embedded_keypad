# peripherals.h

## Purpose
Public interface for peripheral helpers: LEDs, ignition control, battery/temperature readings, and device serial access.

## Key constants
- `SERIAL_NUMBER_WIDTH`: width of device serial number buffer.

## Public API
- `peripherals_init`
- `peripherals_read_battery_level`
- `peripherals_read_temperature`
- `peripherals_read_vdd`
- `peripherals_toggle_leds`
- `peripherals_ignition_on` / `peripherals_ignition_off`
- `peripheral_turn_on_all_led` / `peripheral_turn_off_all_led`
- `peripheral_set_led`
- `peripheral_toggle_led`
- `peripheral_is_switch_ignition_on`

## Notes
- Declares `device_serial_number` buffer used by the HAL to store serial ID.
