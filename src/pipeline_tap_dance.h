#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "key_event_buffer.h"
#include "pipeline_executor.h"
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TDCL_TAP_KEY_SENDKEY,
    TDCL_HOLD_KEY_CHANGELAYERTEMPO,
} td_customlayer_action_t;

// New state machine as per specification
typedef enum {
    TAP_DANCE_IDLE,
    TAP_DANCE_WAITING_FOR_HOLD,      // Key pressed, waiting for hold timeout
    TAP_DANCE_WAITING_FOR_TAP,       // Key released, waiting for tap timeout or next press
    TAP_DANCE_INTERRUPT_CONFIG_ACTIVE, // Interrupt config timeout expired, waiting for hold timeout
    TAP_DANCE_HOLDING,               // Hold action activated, key still pressed
    TAP_DANCE_COMPLETED              // Action executed, ready to reset
} tap_dance_state_t;

typedef struct {
    uint8_t tap_count;
    td_customlayer_action_t action;
    platform_keycode_t keycode;
    uint8_t layer;
    int16_t interrupt_config; // -1, 0, or positive milliseconds for interrupt behavior
} pipeline_tap_dance_action_config_t;

typedef struct {
    tap_dance_state_t state;
    uint8_t tap_count;               // Current tap count (1st tap, 2nd tap, etc.)
    uint8_t original_layer;          // Layer when sequence started
    uint8_t selected_layer;          // Layer selected by hold action
    platform_keycode_t selected_keycode; // Keycode selected by tap action
    platform_time_t key_press_time;      // When the trigger key was first pressed
    platform_time_t last_release_time;   // When the trigger key was last released
    bool hold_action_discarded;         // Flag for positive interrupt config when hold is discarded
    size_t behaviour_index;             // Index in global config for layer stack tracking
    bool is_nested_active;              // Whether this sequence is currently active (not idle)
    platform_key_event_buffer_t *key_buffer; // Key buffer for this tap dance sequence
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

// Global layer stack for nesting management
typedef struct {
    uint8_t layer;                   // The layer selected by this entry
    size_t behaviour_index;          // Which tap dance behaviour owns this layer selection
    bool marked_for_resolution;      // Whether this layer selection is waiting to be resolved
} layer_stack_entry_t;

typedef struct {
    layer_stack_entry_t stack[16];   // Fixed size stack - max nesting = max behaviours
    size_t top;                      // Index of top of stack (0 = empty)
    uint8_t current_layer;           // Currently active layer
} global_layer_stack_t;

typedef struct {
    size_t last_behaviour; // The last behaviour that was processed
    global_layer_stack_t layer_stack; // Global layer stack for nesting
} pipeline_tap_dance_global_status_t;

typedef struct {
    size_t length; // Number of tap dance behaviour configurations
    pipeline_tap_dance_behaviour_t *behaviours[]; // Array of tap dance behaviour configurations
} pipeline_tap_dance_global_config_t;

// Helper functions
bool is_key_repetition_exception(pipeline_tap_dance_behaviour_config_t* config);
bool should_ignore_same_keycode_nesting(pipeline_tap_dance_global_config_t* global_config, platform_keycode_t keycode);

// Layer stack management functions
void layer_stack_push(uint8_t layer, size_t behaviour_index);
void layer_stack_mark_for_resolution(size_t behaviour_index);
void layer_stack_resolve_dependencies(void);
uint8_t layer_stack_get_current_layer(void);

void pipeline_tap_dance_global_state_create(void);
void reset_behaviour_state(pipeline_tap_dance_behaviour_status_t *behaviour_status);

pipeline_tap_dance_behaviour_status_t* pipeline_tap_dance_behaviour_state_create(void);

void pipeline_tap_dance_callback_process_data(pipeline_callback_params_t* params, pipeline_actions_t* actions, void* user_data);
void pipeline_tap_dance_callback_reset(void* user_data);

#ifdef __cplusplus
}
#endif

