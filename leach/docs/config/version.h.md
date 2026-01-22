# version.h

## Purpose
Defines firmware identity information used in BLE advertising and Device Information Service (DIS) characteristics.

## Key macros
- `DEVICE_NAME`: Device name used in advertising.
- `MANUFACTURER_NAME`: DIS manufacturer string.
- `MODEL_NUM` and `HARDWARE_REV`: Model and hardware revision strings.
- `FIRMWARE_REV`: Firmware revision string (current active version).

## Version history
The file contains a commented history of firmware revisions, documenting changes
across releases. The active revision is the single uncommented `FIRMWARE_REV`.

## Notes
- Board selection affects the model/hardware revision definitions.
- `version[]` is declared externally for runtime use.
