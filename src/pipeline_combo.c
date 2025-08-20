#include "pipeline_combo.h"
#include "pipeline_executor.h"
#include "platform_interface.h"

#define g_hold_timeout 200  // Hold timeout in milliseconds

void pipeline_tap_dance_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, void* user_data) {
    pipeline_combo_global_config_t* global_config = (pipeline_combo_global_config_t*)user_data;

    if (params->callback_type == PIPELINE_CALLBACK_KEY_EVENT) {

        // Check if the key event is part of any active combo
        bool combo_processed = false;

        for (size_t i = 0; i < global_config->length; i++) {
            pipeline_combo_config_t* combo = &global_config->combos[i];
            if (combo->is_active == COMBO_ACTIVE) {
                bool key_found = false;
                for (size_t j = 0; j < combo->length; j++) {
                    pipeline_combo_key_t* key = &combo->keys[j];
                    if (platform_compare_keyposition(key->keypos, params->key_event->keypos)) {
                        key_found = true;
                        break;
                    }
                }
                if (key_found) {
                    process_active_combo(&combo, params->key_event);
                    combo_processed = true;
                    break;
                }
            }
        }
        if (combo_processed) {
            actions->mark_as_processed_fn();
            return_actions->no_capture_fn();
            return;
        }

        // Check if the key event is part of any waiting combo
        if (params->key_event->is_press) {
            size_t max_num_of_keys = 0;
            pipeline_combo_config_t* combo_with_max_keys = NULL;

            for (size_t i = 0; i < global_config->length; i++) {
                pipeline_combo_config_t* combo = &global_config->combos[i];
                if (combo->is_active == COMBO_WAITING_FOR_KEYS) {
                    bool key_found = false;
                    for (size_t j = 0; j < combo->length; j++) {
                        pipeline_combo_key_t* key = &combo->keys[j];
                        if (platform_compare_keyposition(key->keypos, params->key_event->keypos)) {
                            key->is_pressed = true;
                            key_found = true;
                            break;
                        }
                    }

                    if (key_found && combo->length > max_num_of_keys) {
                        max_num_of_keys = combo->length;
                        combo_with_max_keys = combo;
                    }
                }
            }
            if (combo_with_max_keys != NULL) {
                bool all_keys_pressed = true;
                for (size_t i = 0; i < combo_with_max_keys->length; i++) {
                    pipeline_combo_key_t* key = &combo_with_max_keys->keys[i];
                    bool same_key_pos_than_key_event = platform_compare_keyposition(key->keypos, params->key_event->keypos);
                    if (key->is_pressed == false && same_key_pos_than_key_event == false) {
                        // The combo key is not pressed and the new key is not part of the combo
                        all_keys_pressed = false;
                        break;
                    }
                }
                if (all_keys_pressed) {
                    activate_combo(&combo_with_max_keys, params->key_event);
                } else {
                    actions->mark_as_processed_fn();
                    return_actions->no_capture_with_deferred_callback_fn(); // Select one the rest of the combos that have all the keys pressed. Maybe use the COMBO_WAITING_FOR_CONFIRMATION and save the need to check for all the keys pressed
                }
            }
        } else {
            // Remove the key press from all the combos in waiting state
            for (size_t i = 0; i < global_config->length; i++) {
                pipeline_combo_config_t* combo = &global_config->combos[i];
                if (combo->is_active == COMBO_WAITING_FOR_KEYS) {
                    for (size_t j = 0; j < combo->length; j++) {
                        pipeline_combo_key_t* key = &combo->keys[j];
                        if (platform_compare_keyposition(key->keypos, params->key_event->keypos)) {
                            key->is_pressed = false;
                            break;
                        }
                    }
                }
            }
        }

    } else if (params->callback_type == PIPELINE_CALLBACK_TIMER) {
        // Handle hold events
    }
}

void pipeline_tap_dance_callback_reset(void* user_data) {

}
