#pragma once

#include "pipeline_key_replacer.h"

pipeline_key_replacer_pair_t* pipeline_key_replacer_create_pairs(platform_keycode_t keycode, replacer_callback callback);
