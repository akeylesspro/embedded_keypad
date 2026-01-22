# FreeRTOSConfig.h

## Purpose
FreeRTOS kernel configuration for the Leach firmware. It defines scheduler behavior, memory sizes, interrupt priorities, and optional features.

## Key scheduler settings
- Preemption enabled (`configUSE_PREEMPTION`).
- Tickless idle enabled (`configUSE_TICKLESS_IDLE`).
- Tick source configured to RTC (`configTICK_SOURCE = FREERTOS_USE_RTC`).
- Tick rate set to 1024 Hz.

## Memory and task settings
- Heap size: `configTOTAL_HEAP_SIZE` (2 * 8196 bytes).
- Minimal stack size: `configMINIMAL_STACK_SIZE`.
- Timer task stack and queue sizes configured.

## Synchronization features
Enables mutexes, recursive mutexes, and counting semaphores.

## Interrupt configuration
- `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` mapped to `_PRIO_APP_HIGH`.
- `configKERNEL_INTERRUPT_PRIORITY` set to the lowest priority.

## Hooks and diagnostics
- Idle hook enabled (`configUSE_IDLE_HOOK`).
- Stack overflow and malloc failed hooks disabled.
- Optional debug assert mapping via `configASSERT`.

## Notes
- RTC is used as the system tick (RTC1 IRQ).
- Settings are tuned for low-power operation and embedded constraints.
