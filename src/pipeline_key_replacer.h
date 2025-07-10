#pragma once

#include "pipeline_executor.h"

typedef struct {
    uint8_t modifiers;
} replacer_callback_input;

typedef struct {
    platform_keycode_t keycode;
    uint8_t modifiers;
} replacer_callback_result;

typedef void (*replacer_callback)(replacer_callback_input*, replacer_callback_result*);

typedef struct {
    platform_keycode_t keycode;
    replacer_callback callback;
    bool activated;
} pipeline_key_replacer_pair_t;

typedef struct {
    size_t length;
    pipeline_key_replacer_pair_t* modifier_pairs[];
} pipeline_key_replacer_global_config_t;

typedef struct {
    pipeline_key_replacer_global_config_t* config;
} pipeline_key_replacer_global_t;

void pipeline_key_replacer_callback(pipeline_callback_params_t* params, pipeline_actions_t* actions, void* user_data);
