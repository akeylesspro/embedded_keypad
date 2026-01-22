# main.h

## Purpose
Header for the main application module. It currently exposes a single ISR-safe helper for event groups.

## Public API
- `send_event_from_isr(EventGroupHandle_t event, EventBits_t uxBitsToSet)`
  - Sets event bits from interrupt context and yields if a higher-priority task is woken.

## Usage notes
- Intended for ISR context only. It wraps `xEventGroupSetBitsFromISR` and `portYIELD_FROM_ISR`.
- Requires FreeRTOS event group types to be visible in any unit that includes this header.

## Dependencies
- Indirectly depends on FreeRTOS event group types via the calling C files.

## Notes
- The header is minimal by design to reduce coupling with the rest of `main.c`.
