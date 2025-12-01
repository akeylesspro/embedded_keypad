#pragma once

#include "shared_custom_board.h"

#ifdef BOARD_CUSTOM

#elif defined(BOARD_CUSTOM)

#if BOARD_CUSTOM == LEACH_REV_1

#else

#error "Cloud-Wise Leach Custom Board is unknown " ## BOARD_CUSTOM

#endif

#endif
