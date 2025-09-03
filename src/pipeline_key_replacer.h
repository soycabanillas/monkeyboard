#pragma once

#include <stddef.h>
#include <stdint.h>
#include "key_virtual_buffer.h"
#include "pipeline_executor.h"
#include "platform_interface.h"

typedef struct {
    platform_virtual_buffer_virtual_event_t buffer[PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS];
    uint8_t buffer_length;
} platform_key_replacer_event_buffer_t;

typedef struct {
    platform_keycode_t keycode;
    platform_key_replacer_event_buffer_t* press_event_buffer;
    platform_key_replacer_event_buffer_t* release_event_buffer;
} pipeline_key_replacer_pair_t;

typedef struct {
    size_t length;
    pipeline_key_replacer_pair_t** modifier_pairs;
} pipeline_key_replacer_global_config_t;

void pipeline_key_replacer_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, pipeline_key_replacer_global_config_t* config);
void pipeline_key_replacer_callback_process_data_executor(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* config);
void pipeline_key_replacer_callback_reset(pipeline_key_replacer_global_config_t* config);
void pipeline_key_replacer_callback_reset_executor(void* config);

