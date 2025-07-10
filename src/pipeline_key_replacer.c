#include "pipeline_key_replacer.h"
#include "keycodes.h"

void pipeline_key_replacer_callback(pipeline_callback_params_t* params, pipeline_actions_t* actions, void* user_data) {
    //platform_log_debug("pipeline_key_replacer_callback || up: %u || press: %u", params->up, params->callback_type);
    pipeline_key_replacer_global_t* data = (pipeline_key_replacer_global_t*)user_data;
    if (params->callback_type == PIPELINE_CALLBACK_KEY_PRESS) {
        for (size_t i = 0; i < data->config->length; i++)
        {
            if (data->config->modifier_pairs[i]->keycode == params->keycode) {
                replacer_callback_input input = {
                    .modifiers = 0
                };
                replacer_callback_result result = {
                    .keycode = 0,
                    .modifiers = 0
                };
                data->config->modifier_pairs[i]->callback(&input, &result);
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_SHIFT && params->info.is_pressed_fn(KC_LEFT_SHIFT) == false) {
                    platform_register_code(KC_LEFT_SHIFT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_SHIFT && params->info.is_pressed_fn(KC_RIGHT_SHIFT) == false) {
                    platform_register_code(KC_RIGHT_SHIFT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_CTRL && params->info.is_pressed_fn(KC_LEFT_CTRL) == false) {
                    platform_register_code(KC_LEFT_CTRL);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_CTRL && params->info.is_pressed_fn(KC_RIGHT_CTRL) == false) {
                    platform_register_code(KC_RIGHT_CTRL);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_ALT && params->info.is_pressed_fn(KC_LEFT_ALT) == false) {
                    platform_register_code(KC_LEFT_ALT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_ALT && params->info.is_pressed_fn(KC_RIGHT_ALT) == false) {
                    platform_register_code(KC_RIGHT_ALT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_GUI && params->info.is_pressed_fn(KC_LEFT_GUI) == false) {
                    platform_register_code(KC_LEFT_GUI);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_GUI && params->info.is_pressed_fn(KC_RIGHT_GUI) == false) {
                    platform_register_code(KC_RIGHT_GUI);
                    platform_wait_ms(10);
                }
                // tap_code(result.keycode);
                platform_register_code(result.keycode);
                data->config->modifier_pairs[i]->activated = true;
                break;
            }
        }
    } else if (params->callback_type == PIPELINE_CALLBACK_KEY_RELEASE) {
        for (size_t i = 0; i < data->config->length; i++)
        {
            if (data->config->modifier_pairs[i]->keycode == params->keycode && data->config->modifier_pairs[i]->activated) {
                replacer_callback_input input = {
                    .modifiers = 0
                };
                replacer_callback_result result = {
                    .keycode = 0,
                    .modifiers = 0
                };
                data->config->modifier_pairs[i]->callback(&input, &result);
                platform_unregister_code(result.keycode);
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_SHIFT && params->info.is_pressed_fn(KC_LEFT_SHIFT) == false) {
                    platform_unregister_code(KC_LEFT_SHIFT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_SHIFT && params->info.is_pressed_fn(KC_RIGHT_SHIFT) == false) {
                    platform_unregister_code(KC_RIGHT_SHIFT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_CTRL && params->info.is_pressed_fn(KC_LEFT_CTRL) == false) {
                    platform_unregister_code(KC_LEFT_CTRL);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_CTRL && params->info.is_pressed_fn(KC_RIGHT_CTRL) == false) {
                    platform_unregister_code(KC_RIGHT_CTRL);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_ALT && params->info.is_pressed_fn(KC_LEFT_ALT) == false) {
                    platform_unregister_code(KC_LEFT_ALT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_ALT && params->info.is_pressed_fn(KC_RIGHT_ALT) == false) {
                    platform_unregister_code(KC_RIGHT_ALT);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_LEFT_GUI && params->info.is_pressed_fn(KC_LEFT_GUI) == false) {
                    platform_unregister_code(KC_LEFT_GUI);
                    platform_wait_ms(10);
                }
                if (result.modifiers & MACRO_KEY_MODIFIER_RIGHT_GUI && params->info.is_pressed_fn(KC_RIGHT_GUI) == false) {
                    platform_unregister_code(KC_RIGHT_GUI);
                    platform_wait_ms(10);
                }
                data->config->modifier_pairs[i]->activated = false;
                break;
            }
        }
    }
}
