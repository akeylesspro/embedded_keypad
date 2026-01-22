# Glossary

## Platform & Firmware Stack
- **MCU (nRF52)**  
  Nordic’s ARM Cortex-M4 SoC that runs every firmware layer, from FreeRTOS through the keypad application.
- **SoftDevice S132**  
  Proprietary BLE stack that supplies GAP, GATT, advertising, security, and link-layer services so the keypad can be both a peripheral and a central.
  - *GAP (Generic Access Profile)* – Defines how devices advertise, scan, and connect; the keypad uses it to control central/peripheral behavior.
  - *GATT (Generic Attribute Profile)* – Defines services/characteristics for data exchange; the keypad exposes ignition, logs, and DFU characteristics via GATT.
- **FreeRTOS**  
  Real-time operating system handling the scheduler, timers, queues, and synchronization that keep `monitor`, `keypad`, `ble`, and `logger` tasks coordinated.
- **Secure DFU Bootloader**  
  OTA-capable bootloader that authenticates firmware packages before swapping images, enabling field updates over BLE without opening the enclosure.
- **Cloud-Wise SDK**  
  Internal SDK composed of HAL, drivers, and reusable logic that multiple Cloud-Wise products share; `leach/` links these modules into the keypad firmware.

## Runtime Tasks & Logic
- **Logic Layer**  
  Collection of shared modules (`monitor`, `keypad`, `configuration`, `state_machine`, etc.) that implement security states, keypad handling, bonding policies, and timekeeping.
- **Monitor Thread**  
  Primary FreeRTOS task that loads configuration, validates EEPROM CRC, manages security states, drives buzzer/LED patterns, logs events, and supervises task health.
- **Keypad Thread**  
  Scans the 2×3 key matrix every 40 ms, debounces presses, differentiates short/long presses, and emits `KeypadMessage` events to the monitor.
- **BLE Thread**  
  Oversees central scans, bonding, connection records, and EEPROM persistence of addresses/serial numbers while coordinating with Ruptela peripherals.
- **Logger Thread**  
  Flushes deferred RTT logs from `leach/main.c`, ensuring diagnostic output remains non-blocking even under heavy system load.
- **SoftDevice Task**  
  Context spawned via `nrf_sdh_freertos` that receives BLE stack callbacks, manages advertising sets, negotiates connection parameters, and services DFU transports.

## Connectivity & Data Paths
- **BLE Peripheral & Central Roles**  
  The keypad advertises custom services (peripheral role) while simultaneously scanning for leach devices and the Ruptela modem (central role) to maintain encrypted links.
- **Ruptela Modem**  
  Remote BLE/GPRS bridge that receives log streams and command packets; `ble_task` forwards frames and `Ruptela.c` applies configuration/time/password updates.
- **SPI EEPROM**  
  External flash (M95 series) accessed over SPI to store configuration structures, bonding lists, security flags, and CRC values guarded by semaphores in `eprom.c`.

## Hardware Interface & HAL
- **HAL (Hardware Abstraction Layer)**  
  Board-support layer that translates logical peripherals into nRF52 pin assignments and driver handles.
  - *GPIO (General Purpose I/O)* – Controls LEDs, relay, ignition sense, and keypad rows/columns through standardized pin helpers.
  - *PWM (Pulse Width Modulation)* – Drives the buzzer melodies via `nrfx_pwm`.
  - *SAADC (Successive Approximation ADC)* – Measures battery voltage (VDD) and other analog signals for health monitoring.
  - *SPI (Serial Peripheral Interface)* – Communicates with the external EEPROM and any additional serial peripherals.
- **Drivers**  
  Concrete implementations (EEPROM, buzzer, ADC, etc.) that expose thread-safe APIs for higher-level logic to read sensors, play tones, and persist data.
- **Ignition Sense & Relay**  
  Hardware IO pair that detects vehicle ignition state and toggles the relay output; mirrored to BLE characteristics so remote clients see real-time status.
- **Watchdog & RTC Tick**  
  `state_machine.c` feeds the watchdog timer to reboot on hangs and derives a 1 Hz RTC tick (`time_since_2020`) used for scheduling and timestamping logs.

## Tooling & Diagnostics
- **Segger Embedded Studio (SES)**  
  Nordic’s supported IDE/toolchain; batch scripts in `build/` invoke SES to compile the project, merge the SoftDevice hex, and package debug/release artifacts.
- **RTT Logging**  
  Segger Real-Time Transfer transport that streams logs without blocking the CPU; release images strip most RTT output to conserve flash and runtime overhead.


