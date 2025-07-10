#pragma once

#include "pipeline_key_replacer.h"
#include "pipeline_oneshot_modifier.h"
#include "pipeline_tap_dance.h"

pipeline_key_replacer_global_config_t* pipeline_key_replacer_global_config_create(void);
pipeline_oneshot_modifier_global_config_t* pipeline_oneshot_modifier_global_config_create(void);
pipeline_tap_dance_global_config_t* pipeline_tap_dance_global_config_create(void);
