#pragma once

#include "pipeline_key_replacer.h"
#include "platform_interface.h"

pipeline_key_replacer_pair_t* pipeline_key_replacer_create_pairs(platform_keycode_t keycode, platform_virtual_event_buffer_t* virtual_event_buffer_press, platform_virtual_event_buffer_t* virtual_event_buffer_release);
