#include "platform_types.h"

#if defined(AGNOSTIC_USE_1D_ARRAY)
    platform_keypos_t dummy_keypos = 0; // Default key position for 1D array
#elif defined(AGNOSTIC_USE_2D_ARRAY)
    platform_keypos_t dummy_keypos = {0, 0}; // Default key position for 2D array
#endif
