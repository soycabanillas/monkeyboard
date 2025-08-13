#pragma once

#include <stdint.h>
#include "pipeline_oneshot_modifier.h"
#include "platform_types.h"

pipeline_oneshot_modifier_pair_t* pipeline_oneshot_modifier_create_pairs(platform_keycode_t keycode, uint8_t modifiers);
