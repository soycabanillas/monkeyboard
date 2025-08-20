#pragma once

#include "pipeline_executor.h"
#include "platform_types.h"
#include <stddef.h>

typedef struct {
    platform_keypos_t keypos; // Keypos for the combo
    bool is_pressed;
    platform_keycode_t key_on_press;
    platform_keycode_t key_on_release;
} pipeline_combo_key_t;

typedef enum {
    // COMBO_INACTIVE,
    COMBO_WAITING_FOR_KEYS,
    COMBO_WAITING_FOR_CONFIRMATION,
    COMBO_ACTIVE,
} pipeline_combo_state_t;

typedef struct {
    size_t length; // Number of keys in the combo
    pipeline_combo_key_t* keys; // Array of keys in the combo

    platform_time_t last_pressed_timestamp;
    pipeline_combo_state_t is_active;
} pipeline_combo_config_t;

typedef struct {
    size_t length; // Number of combos
    pipeline_combo_config_t* combos; // Array of combo configurations
} pipeline_combo_global_config_t;

void pipeline_tap_dance_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, void* user_data);
void pipeline_tap_dance_callback_reset(void* user_data);
