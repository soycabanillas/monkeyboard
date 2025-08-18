#include "pipeline_key_replacer_initializer.h"
#include <stdlib.h>
#include "pipeline_key_replacer.h"
#include "platform_interface.h"

pipeline_key_replacer_pair_t* pipeline_key_replacer_create_pairs(platform_keycode_t keycode, platform_key_replacer_event_buffer_t* press_event_buffer, platform_key_replacer_event_buffer_t* release_event_buffer) {
    pipeline_key_replacer_pair_t* key_replacer_pairs = malloc(sizeof(pipeline_key_replacer_pair_t));
    key_replacer_pairs->keycode = keycode;
    key_replacer_pairs->press_event_buffer = press_event_buffer;
    key_replacer_pairs->release_event_buffer = release_event_buffer;
    return key_replacer_pairs;
}
