#include "pipeline_key_replacer_initializer.h"
#include <stdlib.h>
#include "pipeline_key_replacer.h"
#include "platform_interface.h"

pipeline_key_replacer_pair_t* pipeline_key_replacer_create_pairs(platform_keycode_t keycode, platform_virtual_event_buffer_t* virtual_event_buffer_press, platform_virtual_event_buffer_t* virtual_event_buffer_release) {
    pipeline_key_replacer_pair_t* key_replacer_pairs = malloc(sizeof(pipeline_key_replacer_pair_t));
    key_replacer_pairs->keycode = keycode;
    key_replacer_pairs->virtual_event_buffer_press = virtual_event_buffer_press;
    key_replacer_pairs->virtual_event_buffer_release = virtual_event_buffer_release;
    return key_replacer_pairs;
}
