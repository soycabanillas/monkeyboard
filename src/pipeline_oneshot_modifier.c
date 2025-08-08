#include "pipeline_oneshot_modifier.h"
#include <stdint.h>
#include <stdlib.h>
#include "pipeline_executor.h"
#include "platform_types.h"

uint8_t modifier_state = 0;

void set_modifier(uint8_t modifier) {
    modifier_state |= modifier;
}

bool is_modifier_active(uint8_t modifier) {
    return (modifier_state & modifier) != 0;
}

pipeline_oneshot_modifier_global_status_t* pipeline_oneshot_modifier_global_state_create(void) {
    pipeline_oneshot_modifier_global_status_t* global_status = malloc(sizeof(pipeline_oneshot_modifier_global_status_t));
    global_status->modifiers = 0;
    global_status->modifiers_applied = false;
    return global_status;
}

void pipeline_oneshot_modifier_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* user_data) {
    // platform_log_debug("pipeline_oneshot_modifier_callback || up: %u || press: %u", params->up, params->callback_type);
    pipeline_oneshot_modifier_global_t* global = (pipeline_oneshot_modifier_global_t*)user_data;
    pipeline_oneshot_modifier_global_config_t* global_config = global->config;
    pipeline_oneshot_modifier_global_status_t* global_status = global->status;

    if (global_status->modifiers_applied == true) {
        if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_SHIFT) {
            actions->add_untap_fn(PLATFORM_KC_LEFT_SHIFT);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_SHIFT) {
            actions->add_untap_fn(PLATFORM_KC_RIGHT_SHIFT);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_CTRL) {
            actions->add_untap_fn(PLATFORM_KC_LEFT_CTRL);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_CTRL) {
            actions->add_untap_fn(PLATFORM_KC_RIGHT_CTRL);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_ALT) {
            actions->add_untap_fn(PLATFORM_KC_LEFT_ALT);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_ALT) {
            actions->add_untap_fn(PLATFORM_KC_RIGHT_ALT);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_GUI) {
            actions->add_untap_fn(PLATFORM_KC_LEFT_GUI);
        }
        if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_GUI) {
            actions->add_untap_fn(PLATFORM_KC_RIGHT_GUI);
        }
        global_status->modifiers = 0;
        global_status->modifiers_applied = false;
    }

    bool found_modifier = false;
    if (params->callback_type == PIPELINE_CALLBACK_KEY_PRESS) {
        for (size_t i = 0; i < global_config->length; i++)
        {
            if (global_config->modifier_pairs[i]->keycode == params->keycode) {
                global_status->modifiers = global_status->modifiers | global_config->modifier_pairs[i]->modifiers;
                found_modifier = true;
                break;
            }
        }
    }
    if (found_modifier == false) {
        if (global_status->modifiers != 0 && params->keycode <= 0xFF && params->callback_type == PIPELINE_CALLBACK_KEY_PRESS) {
            if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_SHIFT) {
                actions->add_tap_fn(PLATFORM_KC_LEFT_SHIFT, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_SHIFT) {
                actions->add_tap_fn(PLATFORM_KC_RIGHT_SHIFT, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_CTRL) {
                actions->add_tap_fn(PLATFORM_KC_LEFT_CTRL, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_CTRL) {
                actions->add_tap_fn(PLATFORM_KC_RIGHT_CTRL, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_ALT) {
                actions->add_tap_fn(PLATFORM_KC_LEFT_ALT, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_ALT) {
                actions->add_tap_fn(PLATFORM_KC_RIGHT_ALT, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_LEFT_GUI) {
                actions->add_tap_fn(PLATFORM_KC_LEFT_GUI, params->keypos);
            }
            if (global_status->modifiers & MACRO_KEY_MODIFIER_RIGHT_GUI) {
                actions->add_tap_fn(PLATFORM_KC_RIGHT_GUI, params->keypos);
            }
            global_status->modifiers_applied = true;
        }
    }
}

void pipeline_oneshot_modifier_callback_reset(void* user_data) {
    pipeline_oneshot_modifier_global_t* global = (pipeline_oneshot_modifier_global_t*)user_data;
    global->status->modifiers = 0;
    global->status->modifiers_applied = false;
}
