#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pipeline_executor.h"
#include "platform_types.h"

typedef struct {
    platform_keycode_t keycode;
    uint8_t modifiers;
} pipeline_oneshot_modifier_pair_t;

typedef struct {
    size_t length;
    pipeline_oneshot_modifier_pair_t* modifier_pairs[];
} pipeline_oneshot_modifier_global_config_t;

typedef struct {
    uint8_t modifiers;
    bool modifiers_applied;
} pipeline_oneshot_modifier_global_status_t;

typedef struct {
    pipeline_oneshot_modifier_global_config_t *config; // Pointer to the global configuration
    pipeline_oneshot_modifier_global_status_t *status; // Status of the global tap dance
} pipeline_oneshot_modifier_global_t;

pipeline_oneshot_modifier_global_status_t* pipeline_oneshot_modifier_global_state_create(void);
void pipeline_oneshot_modifier_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* user_data);
void pipeline_oneshot_modifier_callback_reset(void* user_data);
