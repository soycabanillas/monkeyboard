#pragma once

#include "pipeline_key_replacer.h"
#include "platform_interface.h"

pipeline_key_replacer_pair_t* pipeline_key_replacer_create_pairs(platform_keycode_t keycode, platform_key_replacer_event_buffer_t* press_event_buffer, platform_key_replacer_event_buffer_t* release_event_buffer);
