#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pipeline_combo.h"
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

pipeline_combo_config_t* create_combo(uint8_t length, pipeline_combo_key_t** keys, pipeline_combo_key_translation_t key_on_press_combo, pipeline_combo_key_translation_t key_on_release_combo);
pipeline_combo_key_t* create_combo_key(platform_keypos_t keypos, pipeline_combo_key_translation_t key_on_press, pipeline_combo_key_translation_t key_on_release);
pipeline_combo_key_translation_t create_combo_key_action(pipeline_combo_key_action_t action, platform_keycode_t key);

#ifdef __cplusplus
}
#endif
