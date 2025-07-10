#include "pipeline_oneshot_modifier_initializer.h"
#include <stdlib.h>

pipeline_oneshot_modifier_pair_t* pipeline_oneshot_modifier_create_pairs(platform_keycode_t keycode, uint8_t modifiers) {
    pipeline_oneshot_modifier_pair_t* oneshot_modifier_pairs = malloc(sizeof(pipeline_oneshot_modifier_pair_t));
    oneshot_modifier_pairs->keycode = keycode;
    oneshot_modifier_pairs->modifiers = modifiers;
    return oneshot_modifier_pairs;
}


