#pragma once

#include "pipeline_executor.h"
#include "platform_types.h"
#include <stddef.h>

typedef enum {
    COMBO_KEY_ACTION_NONE,
    COMBO_KEY_ACTION_TAP,
    COMBO_KEY_ACTION_REGISTER,
    COMBO_KEY_ACTION_UNREGISTER
} pipeline_combo_key_action_t;

typedef struct {
    pipeline_combo_key_action_t action;
    platform_keycode_t key;
} pipeline_combo_key_translation_t;

typedef struct {
    platform_keypos_t keypos; // Keypos for the combo
    uint8_t press_id;
    bool is_pressed;
    pipeline_combo_key_translation_t key_on_press;
    pipeline_combo_key_translation_t key_on_release;
} pipeline_combo_key_t;

typedef enum {
    COMBO_IDLE,
    COMBO_IDLE_WAITING_FOR_PRESSES,
    COMBO_IDLE_ALL_KEYS_PRESSED,
    COMBO_ACTIVE,
} pipeline_combo_state_t;

typedef struct {
    size_t keys_length; // Number of keys in the combo
    pipeline_combo_key_t* keys; // Array of keys in the combo
    pipeline_combo_key_translation_t key_on_press_combo;
    pipeline_combo_key_translation_t key_on_release_combo;

    pipeline_combo_state_t combo_status;
    bool first_key_event;
    platform_time_t time_from_first_key_event;
} pipeline_combo_config_t;

typedef struct {
    size_t length; // Number of combos
    pipeline_combo_config_t* combos; // Array of combo configurations
} pipeline_combo_global_config_t;

void pipeline_combo_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, void* user_data);
void pipeline_combo_callback_reset(void* user_data);

void pipeline_combo_global_state_create(void);
