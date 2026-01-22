# Keypad Firmware Overview

This repository contains the Cloud-Wise BLE Keypad firmware that runs on a Nordic nRF52 SoC. The keypad behaves as a BLE peripheral plus a BLE central that maintains encrypted links with multiple “leach” peripherals and a Ruptela modem. A local 5-key matrix, LEDs, buzzer, relay output, ignition sense, and an external EEPROM form the hardware interface. The application targets FreeRTOS on top of Nordic’s SoftDevice S132 and a secure DFU bootloader.

The document explains what the project does and breaks down the modules so a full-stack developer can get oriented when stepping into embedded work.


---

## Platform And Build Stack
- **MCU & SDK** – nRF52 + SoftDevice S132. Nordic’s `nrf5-sdk/` supplies BLE stack headers, drivers, and middleware.
- **RTOS** – `FreeRTOS-Kernel/` is vendored to provide the scheduler.
- **Bootloader** – `bootloader/` contains Nordic’s secure DFU so firmware can be updated over BLE without a debugger.
- **Cloud-Wise SDK** – `cloud-wise-sdk/` is the vendor abstraction layer: HAL, drivers, and logic that multiple Cloud-Wise products reuse.
- **Application** – `leach/` is the actual keypad program that links the SDK pieces into a FreeRTOS application.
- **Build assets** – `build/keypad_build-{debug,release}.bat` wrap Segger Embedded Studio / nrfutil steps, while `Keypad.emProject` is the SES project file. `config/*.xml` define linker flash placement for the app and bootloader.

To produce firmware, run one of the batch scripts with a semantic version number (see `readme.txt`). The release build strips RTT logs; the debug build keeps them.

---

## Directory Map
| Path | Purpose |
| --- | --- |
| `leach/` | Application sources (tasks, BLE services, Ruptela integration). |
| `cloud-wise-sdk/hal` | Board support: GPIO, ADC, PWM, SPI, clock, and shared typedefs. |
| `cloud-wise-sdk/drivers` | Device drivers (buzzer PWM melodies, SPI EEPROM, etc.). |
| `cloud-wise-sdk/logic` | Feature logic shared across products (configuration, monitor, keypad scanner, BLE helpers, event definitions). |
| `bootloader/` | Secure DFU bootloader for over-the-air updates. |
| `FreeRTOS-Kernel/` | Third-party RTOS source. |
| `nrf5-sdk/` | Nordic SDK (SoftDevice headers, BLE profiles, peripherals). |
| `build/` | Build scripts, softdevice hex, output staging. |

---

## Runtime Flow (`leach/main.c`)
1. **Startup** – `main()` initializes logging, the LFCLK, and persists a non-initialized reset counter to tell cold boots from resets.
2. **System init** – `modules_init()` sets up `app_timer`, while `peripherals_init()` configures GPIO, the buzzer, and reads the silicon serial number.
3. **State machine services** – `state_machine_init()` enables the watchdog and a low-frequency RTC that drives 1 Hz system time (`cloud-wise-sdk/logic/state_machine.c`).
4. **Task creation** – FreeRTOS spawns:
   - `logger_thread` (flush deferred logs),
   - `monitor_thread` (core business logic),
   - `ble_thread` (connection and bonding control),
   - `keypad_thread` (matrix scanning).
5. **Task wiring** – `init_tasks()` allocates global semaphores/events, initializes BLE services, loads EEPROM configuration, and hands control to the scheduler.

The device then sleeps between FreeRTOS ticks while event groups and queues synchronize keypad, monitor, and BLE behavior.

---

## Task-Level Responsibilities

| Task | Location | Responsibility |
| --- | --- | --- |
| `monitor_thread` | `cloud-wise-sdk/logic/monitor.c` | Initializes EEPROM, loads/sanitizes configuration & bonding list, orchestrates the keypad state machine, manages security states (armed/disarmed/garage/new password), schedules resets, plays buzzer patterns, and pushes logs. |
| `keypad_thread` | `cloud-wise-sdk/logic/keypad.c` | Scans the 2×3 matrix every 40 ms, debounces presses, distinguishes short vs. long presses, feeds a queue, and keeps the status LED blinking pattern in sync with the security state. |
| `ble_thread` | `leach/ble/ble_task.c` | Waits on BLE event groups, runs central scans/filters, manages bonding, saves MAC addresses + serials to EEPROM, mirrors ignition state characteristics, and relays Ruptela commands/logs. |
| `logger_thread` | `leach/main.c` | Flushes NRF_LOG buffers on idle hook so RTT/UART logging stays non-blocking. |
| SoftDevice task | `leach/ble/ble_services_manager.c` | Registered via `nrf_sdh_freertos`, handles BLE stack events, advertising, GATT, dfu transport, and connection parameter negotiation. |

FreeRTOS idle hooks feed the watchdog, and `state_machine.c`’s RTC tick drives global uptime counters.

---

## Module Breakdown

### Configuration & Persistence
- `cloud-wise-sdk/logic/configuration.*` loads/saves `EpromConfiguration` and `DeviceConfiguration` from SPI EEPROM, enforces CRC, resets to defaults, and maintains bonded device lists (addresses plus 16-byte serials). It also exposes helpers to clear bonding, persist garage mode flags, and update relay defaults.
- `cloud-wise-sdk/drivers/eprom.*` configures the SPI flash (M95 series) via `nrfx_spi`, wraps read/write helpers, and guards access with an RTOS semaphore.
- `cloud-wise-sdk/logic/flash_data.h` declares helper hooks for erasing internal flash regions used for larger files or parameter blobs (implementation is typically project-specific).

### Security State Machine & Monitoring
- `cloud-wise-sdk/logic/monitor.*` is the heart of the keypad. It:
  - Validates EEPROM CRC, tracks reset reasons, and blinks all LEDs on boot.
  - Maintains `KeyState` and `SecurityState` enums, applies timeouts (disarm, garage, locked), and reacts to keypad input.
  - Issues logs/commands to BLE peripherals or the Ruptela modem, schedules periodic keep-alive events, enforces bonding policies, and triggers resets if the connection roster looks unhealthy.
  - Drives buzzer melodies (`failure_buzzer`, `success_buzzer`, `lock_buzzer`) and LED animations via `peripherals.c`.
  - Tracks ignition/relay state bits and synchronizes them with BLE characteristics.
- `cloud-wise-sdk/logic/state_machine.*` sets up the watchdog and RTC tick source. It exposes `state_machine_feed_watchdog()` and increments `time_since_2020` so other modules (e.g., Ruptela logs) can timestamp events.

### Human Interface
- `cloud-wise-sdk/logic/keypad.*` implements the column/row drive logic, 2.5 second long-press detection, and queue-based message passing to `monitor_thread`. It also blinks the “asterisk” LED differently for each security state and checks ignition input to alter the pattern.
- `cloud-wise-sdk/logic/peripherals.*` centralizes GPIO helpers: reading ignition, toggling LEDs, measuring VDD via SAADC, and caching the silicon serial number.
- `cloud-wise-sdk/drivers/buzzer.*` uses `nrfx_pwm` to synthesize short melodies for success/error/lock/power-up events and exposes `buzzer_train`, `buzzer_long`, and melody tables.

### BLE Connectivity & Remote Control
- `leach/ble/ble_services_manager.*` configures the SoftDevice, advertising, scanning, GATT, Device Information Service client, and the custom “Leach” service. It filters by UUID and optionally by MAC/whitelist, launches bonding, and raises `event_ble_connection_changed` when connections change.
- `leach/ble/ble_task.*` coordinates central-side verification:
  - Accepts or rejects peripherals based on stored addresses and bonding mode.
  - Keeps track of `connections_array[]`, connection counts, and the difference between leach vs. modem peripherals.
  - Saves new devices into EEPROM when bonding succeeds (including serial numbers read over BLE DIS), then triggers resets to improve reconnection stability.
  - Bridges Ruptela commands/logs (`Ruptela.c`) via queues when `RUPTELA_SERVICE_SIMULATION` is enabled.
- `cloud-wise-sdk/logic/ignition.*` decodes writes to the ignition GATT characteristic, sets event bits, and can request MCU resets when remote commands set the reset bit.
- `cloud-wise-sdk/logic/ble_file_transfer.*` defines the packet format for file transfers/log uploads over BLE (e.g., log streaming to a paired modem), including simulated transfer hooks.
- `cloud-wise-sdk/logic/commands.h` enumerates CLI/Test commands (beep, calibration, firmware build info, flash clearing, etc.) exposed over BLE UART or serial.

### HAL & Drivers
- `cloud-wise-sdk/hal/*.c` configures GPIO directions, initializes PWM, SAADC, SPI, and exposes helper APIs (`hal_read_vdd_raw`, `hal_get_pwm`, `hal_read_device_serial_number`). This isolates nRF specifics from the application logic.
- `cloud-wise-sdk/hal/hal_boards.h` maps logical pins (keys, LEDs, relay, ignition sense, buzzer) to nRF52 GPIO numbers.
- `cloud-wise-sdk/hal/hal_drivers.h` and `hal_data_types.h` define shared enums, event types, and structs so higher layers can stay hardware-agnostic.

### Bootloader & DFU
- `bootloader/main.c` is essentially Nordic’s secure BLE DFU template: it protects the bootloader region, initializes RTT logging for DFU events, and runs `nrf_bootloader_init()` with a simple LED observer. Public keys for DFU image validation live in `bootloader/config/dfu_public_key.c`.
- The bootloader cooperates with the application through `config/bootloader_flash_placement.xml` so both images share flash without overlap.

### Build & Release Automation
- `build/keypad_build-debug.bat` and `build/keypad_build-release.bat` call Segger Embedded Studio to compile the `.emProject`, merge the SoftDevice image (`build/s132_nrf52_7.2.0_softdevice.hex`), and package artifacts. They accept a version argument so the metadata matches `readme.txt`’s release log.

---

## Typical Data Paths
1. **Keypad input → Security state**
   - `keypad_thread` emits a `KeypadMessage`.
   - `monitor_thread` consumes it, updates `key_state`/`security_state`, logs any transitions, and may drive ignition output or send BLE commands.
2. **BLE bonding workflow**
   - `ble_services_manager` observes `BLE_GAP_EVT_CONNECTED`, discovers services, and populates `current_conn_info`.
   - `ble_thread` decides whether to accept, add to `connections_array`, and store addresses/serials via `configuration_save_address_list()`.
3. **Remote Ruptela command**
   - BLE log characteristic delivers a 16-byte command ⇒ `ble_thread` enqueues it ⇒ `Ruptela.c` decodes and updates configuration/time/password, emitting success/failure logs.
4. **Watchdog supervision**
   - `state_machine.c` feeds the WDT.
   - `monitor_thread` toggles `monitor_task_set()` bits; if a task stalls, `monitor_task_check()` detects it and can log or reset.

---

## Where To Learn More
- **Behavior definitions** – `cloud-wise-sdk/logic/monitor.c` is the best entry point; it ties together configuration, keypad events, logs, and resets.
- **BLE specifics** – Browse `leach/ble/ble_services_manager.c` for scanning, white/UUID filtering, custom service UUIDs, and GATT operations.
- **Hardware mapping** – See `cloud-wise-sdk/hal/hal_boards.h` to match pin names with the PCB.
- **Version history** – `readme.txt` enumerates firmware releases with feature bullets and bug fixes.

Armed with these references, you can trace any feature (e.g., bonding, garage mode, ignition logging) from hardware input, through the logic modules, out to BLE or Ruptela links.

