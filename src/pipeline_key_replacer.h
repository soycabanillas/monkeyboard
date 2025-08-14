#pragma once

#include <stddef.h>
#include <stdint.h>
#include "key_virtual_buffer.h"
#include "pipeline_executor.h"
#include "platform_interface.h"

typedef struct {
    platform_keycode_t keycode;
    platform_virtual_event_buffer_t* virtual_event_buffer_press;
    platform_virtual_event_buffer_t* virtual_event_buffer_release;
} pipeline_key_replacer_pair_t;

typedef struct {
    size_t length;
    pipeline_key_replacer_pair_t* modifier_pairs[];
} pipeline_key_replacer_global_config_t;

typedef struct {
    pipeline_key_replacer_global_config_t* config;
} pipeline_key_replacer_global_t;

void pipeline_key_replacer_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* user_data);
void pipeline_key_replacer_modifier_callback_reset(void* user_data);
