#include "pipeline_key_replacer_initializer.h"
#include <stdlib.h>

pipeline_key_replacer_pair_t* pipeline_key_replacer_create_pairs(platform_keycode_t keycode, replacer_callback callback) {
    pipeline_key_replacer_pair_t* key_replacer_pairs = malloc(sizeof(pipeline_key_replacer_pair_t));
    key_replacer_pairs->keycode = keycode;
    key_replacer_pairs->callback = callback;
    key_replacer_pairs->activated = false;
    return key_replacer_pairs;
}
