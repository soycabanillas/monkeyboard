#include "pipeline_oneshot_modifier.h"
#include <stdint.h>
#include <stdlib.h>
#include "pipeline_executor.h"
#include "platform_types.h"

uint8_t modifier_state = 0;

pipeline_oneshot_modifier_global_status_t* pipeline_oneshot_modifier_global_state_create(void) {
    pipeline_oneshot_modifier_global_status_t* global_status = malloc(sizeof(pipeline_oneshot_modifier_global_status_t));
    global_status->modifiers = 0;
    return global_status;
}

void pipeline_oneshot_modifier_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* user_data) {
    platform_virtual_buffer_virtual_event_t* key_event = params->key_event;
    
    // platform_log_debug("pipeline_oneshot_modifier_callback || up: %u || press: %u", params->up, params->callback_type);
    pipeline_oneshot_modifier_global_t* global = (pipeline_oneshot_modifier_global_t*)user_data;
    pipeline_oneshot_modifier_global_config_t* global_config = global->config;
    pipeline_oneshot_modifier_global_status_t* global_status = global->status;

    for (size_t i = 0; i < global_config->length; i++)
    {
        if (global_config->modifier_pairs[i]->keycode == key_event->keycode) {
            global_status->modifiers = global_status->modifiers | global_config->modifier_pairs[i]->modifiers;
            return;
        }
    }
    if (global_status->modifiers != 0 && key_event->keycode <= 0xFF) {
        key_event->keycode = key_event->keycode | (global_status->modifiers << 8);
    }
}

void pipeline_oneshot_modifier_callback_reset(void* user_data) {
    pipeline_oneshot_modifier_global_t* global = (pipeline_oneshot_modifier_global_t*)user_data;
    global->status->modifiers = 0;
}
