# custom_board.h

## Purpose
Board selection wrapper for the Leach custom hardware. It includes shared board definitions and validates the selected board revision.

## Behavior
- Includes `shared_custom_board.h`.
- Checks `BOARD_CUSTOM` and validates known revisions.
- Emits a preprocessor error if the board revision is unknown.

## Notes
- Current logic recognizes `LEACH_REV_1` as the valid custom board revision.
