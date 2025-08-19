#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "pipeline_executor.h"
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global timing configuration
#define g_hold_timeout 200  // Hold timeout in milliseconds
#define g_tap_timeout 200   // Tap timeout in milliseconds

typedef enum {
    TDCL_TAP_KEY_SENDKEY,           // Tap Action
    TDCL_HOLD_KEY_CHANGELAYERTEMPO, // Hold layer while key pressed
} td_customlayer_action_t;

// New state machine as per specification
typedef enum {
    TAP_DANCE_IDLE,
    TAP_DANCE_WAITING_FOR_HOLD,      // Key pressed, waiting for hold timeout
    TAP_DANCE_WAITING_FOR_RELEASE,   // Key pressed, no hold action, waiting for release
    TAP_DANCE_WAITING_FOR_TAP,       // Key released, waiting for tap timeout or next press
    TAP_DANCE_HOLDING,               // Hold action activated, key still pressed
} tap_dance_state_t;

typedef enum {
    TAP_DANCE_HOLD_PREFERRED,  // Prefer hold action over tap
    TAP_DANCE_TAP_PREFERRED,   // Prefer tap action over hold
    TAP_DANCE_BALANCED
} tap_dance_hold_strategy_t;

typedef struct {
    uint8_t tap_count;
    td_customlayer_action_t action;
    platform_keycode_t keycode;
    uint8_t layer;
    tap_dance_hold_strategy_t hold_strategy;
} pipeline_tap_dance_action_config_t;

typedef struct {
    tap_dance_state_t state;
    uint8_t tap_count;               // Current tap count (1st tap, 2nd tap, etc.)
    uint8_t original_layer;          // Layer when sequence started
    uint8_t selected_layer;          // Layer selected by hold action
    platform_keypos_t trigger_keypos; // Key position that triggered the tap dance
} pipeline_tap_dance_behaviour_status_t;

typedef struct {
    platform_keycode_t keycodemodifier;
    uint16_t hold_timeout; // Timeout for hold action
    uint16_t tap_timeout;  // Timeout for tap action
    size_t actionslength;
    pipeline_tap_dance_action_config_t **actions;
} pipeline_tap_dance_behaviour_config_t;

typedef struct {
    pipeline_tap_dance_behaviour_config_t* config;
    pipeline_tap_dance_behaviour_status_t* status;
} pipeline_tap_dance_behaviour_t;


typedef struct {
    size_t last_behaviour; // The last behaviour that was processed
} pipeline_tap_dance_global_status_t;

typedef struct {
    size_t length; // Number of tap dance behaviour configurations
    pipeline_tap_dance_behaviour_t **behaviours; // Array of tap dance behaviour configurations
} pipeline_tap_dance_global_config_t;

void pipeline_tap_dance_global_state_create(void);
void reset_behaviour_state(pipeline_tap_dance_behaviour_status_t *behaviour_status);

pipeline_tap_dance_behaviour_status_t* pipeline_tap_dance_behaviour_state_create(void);

void pipeline_tap_dance_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, void* user_data);
void pipeline_tap_dance_callback_reset(void* user_data);

#ifdef __cplusplus
}
#endif

