# Cloud-Wise BLE Keypad – Project Summary

## What The Project Does
This repository contains the production firmware for the Cloud-Wise BLE Keypad that runs on a Nordic nRF52 SoC. The keypad operates as both a BLE peripheral (to advertise its own service) and a BLE central that manages secure, whitelisted links to “leach” accessories and a Ruptela cellular modem. Locally it supervises a five-key matrix, RGB/segment LEDs, a buzzer, relay output, ignition sense, and an external SPI EEPROM. The firmware layers FreeRTOS on top of Nordic’s SoftDevice S132, adds a secure DFU bootloader, and orchestrates keypad interaction, security states, telemetry, and remote control.

## Hardware & Firmware Platform
- **MCU & Stack**: Nordic nRF52 + SoftDevice S132 with FreeRTOS (`FreeRTOS-Kernel/`) providing the scheduler and low-power idle handling.
- **Bootloader**: Secure DFU implementation in `bootloader/` with an included public key (`bootloader/dfu_public_key.c`) so images can be updated wirelessly.
- **Board Support & Drivers**: `cloud-wise-sdk/hal` maps logical peripherals to GPIO, configures SAADC, PWM, SPI, and exposes HAL helpers. `cloud-wise-sdk/drivers` holds reusable EEPROM and buzzer drivers.
- **Application Layer**: `leach/` contains the keypad-specific FreeRTOS tasks, BLE services, Ruptela bridge, and state machine glue.
- **Application Layer**: `leach/` contains the keypad-specific FreeRTOS tasks, BLE services, Ruptela bridge, and state machine glue.
- **Vendor SDKs**: `nrf5-sdk/` provides BLE stack headers/profiles and low-level drivers. FreeRTOS is vendored for deterministic builds.
- **Build Assets**: `Keypad.emProject` (Segger Embedded Studio) plus `build/keypad_build-{debug,release}.bat` ensure reproducible, versioned artifacts that merge the SoftDevice hex.

## Software Architecture Overview
| Layer | Location | Key Responsibilities |
| --- | --- | --- |
| Bootloader | `bootloader/` | Secure BLE DFU, image validation, LED status during updates. |
| HAL & Drivers | `cloud-wise-sdk/hal`, `cloud-wise-sdk/drivers` | GPIO/power abstraction, PWM buzzer melodies, SPI EEPROM access, watchdog helpers. |
| Shared Logic | `cloud-wise-sdk/logic` | Configuration persistence, keypad scanner, BLE helpers, ignition control, monitoring/task watchdog, event enums. |
| Application | `leach/` | Main FreeRTOS tasks (`monitor`, `keypad`, `ble`, `logger`), BLE service manager, Ruptela protocol bridge, state machine wiring. |
| Third-Party SDK | `nrf5-sdk/`, `FreeRTOS-Kernel/` | Nordic BLE stack, peripheral libraries, and RTOS kernel sources. |

## Feature Set

### Security & Access Control
- Multi-state security engine (armed, disarmed, garage, blocked, password change) managed by `monitor.c`, with programmable disarm/lock timers (e.g., 2-minute disarm, 3-minute block).
- Password management through keypad input or remote Ruptela commands (change, fetch current, revert to defaults).
- Garage modes to clear bonded peripherals, enroll new ones, or clone the current connection list into EEPROM.
- Watchdog-backed task health tracking and automatic resets if BLE connections drift from the persisted roster.
- Relay output and ignition sense tied to security state, including ignition-on warnings in armed mode.

### User Interface & Feedback
- Five-key matrix scanning every 40 ms with debouncing, short/long press detection, and queue-based messaging.
- LED feedback patterns that reflect security modes, connection counts (long press on key 2), garage operations, and bonding progress (blink count matches devices bonded).
- Rich buzzer melodies for success, failure, lock, reset, power-up, test, and watchdog events via the PWM driver.
- Ignition-triggered alerts: periodic beeps and logs when ignition is high but required peripherals are missing.

### BLE Connectivity & Remote Services
- Dual-role behavior: advertises as `CW-RXXXDDDDDDD-AYY` while scanning as a central for Cloud-Wise peripherals and Ruptela modems.
- Custom 128-bit service UUID, ignition characteristic (read/write/notify), command/log characteristics, and Device Information Service usage for peer identification.
- Bonding list persisted in EEPROM with whitelist + UUID filters; firmware refuses to maintain links outside the approved roster unless garage mode enrollment runs.
- Supports up to three concurrent peripherals, differentiates between modem vs. leach peers, and resets after bonding to improve reconnection stability.
- File/log transfer hooks plus BLE UART-style command channel for diagnostics and field service commands.

### Configuration & Persistence
- EEPROM-backed configuration blocks with CRC validation, default restoration, and granular getters/setters (`cloud-wise-sdk/logic/configuration.c`).
- Storage for bonded device addresses, 16-byte serial numbers, relay defaults, garage flags, and lock state.
- Internal flash helpers prepared for larger file transfers or parameter blobs (`flash_data.h`).

### Monitoring, Logging & Diagnostics
- `monitor_thread` initializes hardware, verifies EEPROM health, enforces security state transitions, and schedules periodic keep-alive logs (including bursts after resets).
- Timekeeping via RTC tick (`state_machine.c`) enables timestamped logs and uptime counters.
- Log streaming over BLE log characteristic to Ruptela, covering watchdog resets, ignition events, password changes, and bonding outcomes.
- Remote command execution path (via Ruptela or BLE command characteristic) for arming/disarming, password maintenance, and factory-default requests.

### Bootloader & Update Workflow
- Secure DFU bootloader allows encrypted OTA updates; flash placement XML files coordinate memory layout between bootloader and application.
- DFU observer ties LED/buzzer cues to update status, while `readme.txt` documents version history from 1.0.8 onward.

### Build & Deployment
- Batch scripts accept semantic versions, invoke Segger Embedded Studio, and package SoftDevice + application for release or debug (with RTT logging enabled).
- Release builds strip logs for production stability; debug builds include RTT for lab diagnostics.
- `readme.txt` also captures BLE attribute layout, advertising format, and outstanding issues for quick reference.

## Operational Flow (High Level)
1. **Boot & Init**: `main.c` brings up clocks, logging, peripherals, and the task watchdog, then spawns the FreeRTOS tasks.
2. **Task Roles**:
   - `monitor_thread`: Heart of the system—handles state transitions, bonding workflows, ignition/relay control, and orchestrates resets/logs.
   - `keypad_thread`: Scans keys, maintains LED patterns, and reports user input events.
   - `ble_thread`: Manages scanning, filtering, bonding, data queues, and Ruptela bridging.
   - `logger_thread` & SoftDevice task: Drain log buffers and service BLE stack callbacks without blocking the app.
3. **Data Paths**: Keypad events flow to the monitor, which updates security state and may trigger BLE notifications or logs; BLE connections populate EEPROM lists, and remote commands travel from Ruptela through BLE queues into the monitor/state machine.

Armed with this document plus the detailed module descriptions in `PROJECT_OVERVIEW.md`, you can trace any feature—from keypad gestures to remote commands—through the Cloud-Wise BLE Keypad firmware.

