#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "pipeline_executor.h"
#include "platform_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TDCL_TAP_KEY_SENDKEY,
    // TDCL_HOLD_KEY_SENDKEY,
    TDCL_HOLD_KEY_CHANGELAYERTEMPO,
    // TDCL_TAP_KEY_CUSTOM,
    // TDCL_HOLD_KEY_CUSTOM,
} td_customlayer_action_t;

typedef enum {
  _HOLD_STATE_NOT_SET,
  _DURING_HOLD_DECISION,
  _HOLD_INTERRUPTION_DETECTED,
  _HOLD_INTERRUPTION_AND_TRESHOLD_DETECTED,
  _HOLD_TRESHOLD_DETECTED,
} hold_state;

typedef enum {
  _HOLD_OR_TAP_STATE_NOT_SET,
  _DECIDING,
  _TAP,
  _HOLD,
  _NONE
} hold_or_tap_sequence;

typedef enum {
    _WAITING_FOR_TAP,
    _AFTER_TAP,
    _HOLD_CONFIRMED,
    _AFTER_UNTAP
} pipeline_tap_dance_key_status_t;

typedef struct {
    uint8_t repetitions;
    td_customlayer_action_t action;
    platform_keycode_t keycode;
    uint8_t layer;
} pipeline_tap_dance_action_config_t;

typedef struct {
    uint8_t count;
    bool hasholdaction : 1;
    bool haskeyaction : 1;
    hold_or_tap_sequence sequence;
    hold_state state;
    uint8_t press_buffer_index;
    uint8_t keybuffer_length;
    platform_deferred_token hold_span_reached_token;
    platform_keycode_t selected_keycode;
    platform_deferred_token key_repetition_span_exceeded_token;

    uint8_t original_layer;
    uint8_t key_tap_repetition_count; // Number of times the key has been tapped
    pipeline_tap_dance_key_status_t key_status;
    uint8_t selected_layer;
} pipeline_tap_dance_behaviour_status_t;

typedef struct {
    platform_keycode_t keycodemodifier;
    size_t actionslength;
    pipeline_tap_dance_action_config_t *actions[];
} pipeline_tap_dance_behaviour_config_t;

typedef struct {
    pipeline_tap_dance_behaviour_config_t* config;
    pipeline_tap_dance_behaviour_status_t* status;
} pipeline_tap_dance_behaviour_t;

typedef struct {
    size_t last_behaviour; // The last layer that was processed
} pipeline_tap_dance_global_status_t;

typedef struct {
    size_t length; // Number of custom behaviour configurations
    pipeline_tap_dance_behaviour_t *behaviours[]; // Array of custom behaviour configurations
} pipeline_tap_dance_global_config_t;

void pipeline_tap_dance_global_state_create(void);
void reset_behaviour_state(pipeline_tap_dance_behaviour_status_t *layer_status);

pipeline_tap_dance_behaviour_status_t* pipeline_tap_dance_behaviour_state_create(void);

// bool macros_process_key(platform_keycode_t keycode, abskeyevent_t abskeyevent, custom_layers_struct *custom_layers);
void pipeline_tap_dance_callback(pipeline_callback_params_t* params, pipeline_actions_t* actions, void* user_data);

#ifdef __cplusplus
}
#endif

