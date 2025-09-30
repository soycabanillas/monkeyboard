#include "pipeline_oneshot_modifier.h"
#include <stdint.h>
#include <stdlib.h>
#include "key_virtual_buffer.h"
#include "monkeyboard_keycodes.h"
#include "pipeline_executor.h"

uint8_t modifier_state = 0;

pipeline_oneshot_modifier_global_status_t* pipeline_oneshot_modifier_global_state_create(void) {
    pipeline_oneshot_modifier_global_status_t* global_status = malloc(sizeof(pipeline_oneshot_modifier_global_status_t));
    global_status->modifiers = 0;
    return global_status;
}

void pipeline_oneshot_modifier_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, pipeline_oneshot_modifier_global_t* config) {
    platform_virtual_buffer_virtual_event_t* key_event = params->key_event;

    // platform_log_debug("pipeline_oneshot_modifier_callback || up: %u || press: %u", params->up, params->callback_type);
    pipeline_oneshot_modifier_global_config_t* global_config = config->config;
    pipeline_oneshot_modifier_global_status_t* global_status = config->status;

    for (size_t i = 0; i < global_config->length; i++)
    {
        if (global_config->modifier_pairs[i]->keycode == key_event->keycode) {
            if (key_event->is_press) {
                global_status->modifiers = global_status->modifiers | global_config->modifier_pairs[i]->modifiers;
            }
            actions->mark_as_processed_fn(); // Mark the key event as processed
            return;
        }
    }
    if (global_status->modifiers != 0 && key_event->keycode <= 0xFF && key_event->is_press == true) {
        actions->mark_as_processed_fn();
        uint8_t modifiers = global_status->modifiers;
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_SHIFT) {
            actions->report_press_fn(PLATFORM_KC_LEFT_SHIFT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_SHIFT) {
            actions->report_press_fn(PLATFORM_KC_RIGHT_SHIFT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_CTRL) {
            actions->report_press_fn(PLATFORM_KC_LEFT_CTRL);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_CTRL) {
            actions->report_press_fn(PLATFORM_KC_RIGHT_CTRL);
        }
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_ALT) {
            actions->report_press_fn(PLATFORM_KC_LEFT_ALT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_ALT) {
            actions->report_press_fn(PLATFORM_KC_RIGHT_ALT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_GUI) {
            actions->report_press_fn(PLATFORM_KC_LEFT_GUI);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_GUI) {
            actions->report_press_fn(PLATFORM_KC_RIGHT_GUI);
        }
        actions->report_press_fn(key_event->keycode);
        actions->report_send_fn();
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_SHIFT) {
            actions->report_release_fn(PLATFORM_KC_LEFT_SHIFT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_SHIFT) {
            actions->report_release_fn(PLATFORM_KC_RIGHT_SHIFT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_CTRL) {
            actions->report_release_fn(PLATFORM_KC_LEFT_CTRL);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_CTRL) {
            actions->report_release_fn(PLATFORM_KC_RIGHT_CTRL);
        }
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_ALT) {
            actions->report_release_fn(PLATFORM_KC_LEFT_ALT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_ALT) {
            actions->report_release_fn(PLATFORM_KC_RIGHT_ALT);
        }
        if (modifiers & MACRO_KEY_MODIFIER_LEFT_GUI) {
            actions->report_release_fn(PLATFORM_KC_LEFT_GUI);
        }
        if (modifiers & MACRO_KEY_MODIFIER_RIGHT_GUI) {
            actions->report_release_fn(PLATFORM_KC_RIGHT_GUI);
        }
        actions->report_send_fn();
        global_status->modifiers = 0; // Reset the modifier state after processing
    }
}

void pipeline_oneshot_modifier_callback_process_data_executor(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* config) {
    pipeline_oneshot_modifier_callback_process_data(params, actions, config);
}

void pipeline_oneshot_modifier_callback_reset(pipeline_oneshot_modifier_global_t* config) {
    config->status->modifiers = 0;
}

void pipeline_oneshot_modifier_callback_reset_executor(void* config) {
    pipeline_oneshot_modifier_callback_reset(config);
}
