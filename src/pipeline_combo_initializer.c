#include "pipeline_combo_initializer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pipeline_combo.h"
#include "platform_types.h"

pipeline_combo_config_t* create_combo(uint8_t length, pipeline_combo_key_t** keys, pipeline_combo_key_translation_t key_on_press_combo, pipeline_combo_key_translation_t key_on_release_combo) {
    pipeline_combo_config_t* combo = (pipeline_combo_config_t*)malloc(sizeof(*combo));
    if (!combo) return NULL;

    combo->keys_length = length;
    combo->keys = keys;
    combo->key_on_press_combo = key_on_press_combo;
    combo->key_on_release_combo = key_on_release_combo;
    combo->combo_status = COMBO_IDLE;
    combo->first_key_event = false;
    combo->time_from_first_key_event = 0;
    return combo;
}

pipeline_combo_key_t* create_combo_key(platform_keypos_t keypos, pipeline_combo_key_translation_t key_on_press, pipeline_combo_key_translation_t key_on_release) {
    pipeline_combo_key_t* key = (pipeline_combo_key_t*)malloc(sizeof(*key));
    if (!key) return NULL;

    key->keypos = keypos;
    key->key_on_press = key_on_press;
    key->key_on_release = key_on_release;
    key->press_id = 0;
    key->is_pressed = false;

    return key;
}

pipeline_combo_key_translation_t create_combo_key_action(pipeline_combo_key_action_t action, platform_keycode_t key) {
    pipeline_combo_key_translation_t translation = {
        .action = action,
        .key = key
    };
    return translation;
}
