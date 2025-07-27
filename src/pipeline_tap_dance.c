#include "pipeline_tap_dance.h"
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "key_event_buffer.h"
#include "pipeline_executor.h"
#include "platform_interface.h"
#include "platform_types.h"

// Global timing configuration
#define g_hold_timeout 200  // Hold timeout in milliseconds
#define g_tap_timeout 200   // Tap timeout in milliseconds

pipeline_tap_dance_global_status_t* global_status;

// Helper functions to find actions by tap count and type
pipeline_tap_dance_action_config_t* get_action_tap_key_sendkey(uint8_t tap_count, pipeline_tap_dance_behaviour_config_t* config) {
    for (size_t i = 0; i < config->actionslength; i++) {
        pipeline_tap_dance_action_config_t* action = config->actions[i];
        if (action->tap_count == tap_count && action->action == TDCL_TAP_KEY_SENDKEY) {
            return action;
        }
    }
    return NULL;
}

pipeline_tap_dance_action_config_t* get_action_hold_key_changelayertempo(uint8_t tap_count, pipeline_tap_dance_behaviour_config_t* config) {
    for (size_t i = 0; i < config->actionslength; i++) {
        pipeline_tap_dance_action_config_t* action = config->actions[i];
        if (action->tap_count == tap_count && action->action == TDCL_HOLD_KEY_CHANGELAYERTEMPO) {
            return action;
        }
    }
    return NULL;
}

bool has_subsequent_actions(pipeline_tap_dance_behaviour_config_t* config, uint8_t tap_count) {
    for (size_t i = 0; i < config->actionslength; i++) {
        pipeline_tap_dance_action_config_t* action = config->actions[i];
        if (action->tap_count > tap_count) {
            return true;
        }
    }
    return false;
}

void reset_behaviour_state(pipeline_tap_dance_behaviour_status_t *status) {
    status->state = TAP_DANCE_IDLE;
    status->tap_count = 0;
    status->original_layer = 0;
    status->selected_layer = 0;
    status->selected_keycode = 0;
    status->key_press_time = 0;
    status->last_release_time = 0;
    status->hold_action_discarded = false;
    // behaviour_index is set during initialization, don't reset it
    status->is_nested_active = false;
    // platform_key_event_reset(status->key_buffer);
}

// Helper function to check if we should ignore nesting for same keycode
bool should_ignore_same_keycode_nesting(pipeline_tap_dance_global_config_t* global_config, platform_keycode_t keycode) {
    // Check if any tap dance with this keycode is already active (not idle)
    for (size_t i = 0; i < global_config->length; i++) {
        pipeline_tap_dance_behaviour_t* behaviour = global_config->behaviours[i];
        if (behaviour->config->keycodemodifier == keycode && behaviour->status->is_nested_active) {
            return true; // Same keycode is already active, ignore this event
        }
    }
    return false;
}

bool should_activate_hold_action_on_interrupt(pipeline_tap_dance_action_config_t* hold_action,
                                             pipeline_tap_dance_behaviour_status_t* status,
                                             platform_time_t interrupt_time,
                                             bool is_key_release) {
    if (!hold_action) return false;

    platform_time_t time_since_press = interrupt_time - status->key_press_time;

    switch (hold_action->interrupt_config) {
        case -1:
            // Activate only on press+release or multiple keys
            return is_key_release;

        case 0:
            // Activate on any key press
            return true;

        default:
            // Positive value: activate only if interrupt occurs at or after the specified time
            if (time_since_press >= (platform_time_t)hold_action->interrupt_config) {
                return true;
            } else {
                // Before timeout - discard hold action completely
                status->hold_action_discarded = true;
                return false;
            }
    }
}

void handle_interrupting_key(pipeline_tap_dance_behaviour_config_t *config,
                             pipeline_tap_dance_behaviour_status_t *status,
                             pipeline_physical_actions_t* actions,
                             platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Interrupting Key Event: %d, state: %d", last_key_event->keycode, status->state);

    // Only handle interruptions during hold waiting states
    if (status->state != TAP_DANCE_WAITING_FOR_HOLD &&
        status->state != TAP_DANCE_INTERRUPT_CONFIG_ACTIVE) {
        return;
    }

    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (!hold_action) return;

    bool is_key_release = (last_key_event->is_press == false);

    // Check if this interrupt should activate the hold action
    if (should_activate_hold_action_on_interrupt(hold_action, status, last_key_event->time, is_key_release)) {
        status->state = TAP_DANCE_HOLDING;
        platform_layout_set_layer(status->selected_layer);

        // platform_keycode_t keycode = platform_layout_get_keycode_from_layer(status->selected_layer, first_key_event->keypos);
        // platform_key_event_add_event(status->key_buffer, first_key_event->time, status->selected_layer, first_key_event->keypos, keycode, true);

    }

    // For positive interrupt config, check if hold action should be discarded
    if (hold_action->interrupt_config > 0 && status->hold_action_discarded) {
        // Send the original trigger key press and the interrupting key
        actions->tap_key_fn(config->keycodemodifier);
        actions->tap_key_fn(last_key_event->keycode);
        reset_behaviour_state(status);
    }
}

void generic_key_press_handler(pipeline_tap_dance_behaviour_config_t *config,
                               pipeline_tap_dance_behaviour_status_t *status,
                               pipeline_physical_actions_t* actions,
                               platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Press Handler: %d", last_key_event->keycode);
    status->tap_count++;
    status->key_press_time = last_key_event->time;

    // Check if we have a hold action for this tap count
    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (hold_action != NULL) {
        status->state = TAP_DANCE_WAITING_FOR_HOLD;
        status->selected_layer = hold_action->layer;
        if (hold_action->interrupt_config > 0) {
            pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(hold_action->interrupt_config);
        } else {
            pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(g_hold_timeout);
        }
    } else {
        if (has_subsequent_actions(config, status->tap_count)) {
            status->state = TAP_DANCE_WAITING_FOR_RELEASE;
            pipeline_executor_end_with_capture_next_keys();
        } else {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->register_key_fn(tap_action->keycode);
                status->state = TAP_DANCE_WAITING_FOR_RELEASE;
            }
            pipeline_executor_end_with_capture_next_keys();
        }
    }
}

void generic_key_release_when_not_holding_handler(pipeline_tap_dance_behaviour_config_t *config,
                                                 pipeline_tap_dance_behaviour_status_t *status,
                                                 pipeline_physical_actions_t* actions,
                                                 platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Release Handler (Not Holding): %d", last_key_event->keycode);
    if (has_subsequent_actions(config, status->tap_count)) {
        status->state = TAP_DANCE_WAITING_FOR_TAP;
        pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(g_tap_timeout);
    } else {
        if (status->state == TAP_DANCE_WAITING_FOR_HOLD) {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->tap_key_fn(tap_action->keycode);
                actions->remove_physical_press_and_release_fn(last_key_event->keypos);
            }
            reset_behaviour_state(status);
        } else if (status->state == TAP_DANCE_WAITING_FOR_RELEASE) {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->remove_physical_press_and_release_fn(last_key_event->keypos);
                actions->unregister_key_fn(tap_action->keycode);
            }
            reset_behaviour_state(status);
        }
    }
}

void generic_key_release_when_holding_handler(pipeline_tap_dance_behaviour_config_t *config,
                                              pipeline_tap_dance_behaviour_status_t *status,
                                              pipeline_physical_actions_t* actions,
                                              platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Release Handler (Holding): %d", last_key_event->keycode);

    platform_layout_set_layer(status->original_layer);
    actions->remove_physical_press_and_release_fn(last_key_event->keypos);
    reset_behaviour_state(status);
}

void handle_key_press(pipeline_tap_dance_behaviour_config_t *config,
                     pipeline_tap_dance_behaviour_status_t *status,
                     pipeline_physical_actions_t* actions,
                     platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Main Key press: %d, state: %d", last_key_event->keycode, status->state);

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Main Key press: IDLE");
            // First press of a new sequence
            status->original_layer = platform_layout_get_current_layer(); // Use current layer from stack
            status->is_nested_active = true; // Mark as active
            generic_key_press_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_HOLD");
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_RELEASE");
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_TAP");
            generic_key_press_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_INTERRUPT_CONFIG_ACTIVE:
            DEBUG_TAP_DANCE("-- Main Key press: INTERRUPT_CONFIG_ACTIVE");
            break;
        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Main Key press: HOLDING");
            break;
    }
}

void handle_key_release(pipeline_tap_dance_behaviour_config_t *config,
                        pipeline_tap_dance_behaviour_status_t *status,
                        pipeline_physical_actions_t* actions,
                        platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Main Key release: %d, state: %d", last_key_event->keycode, status->state);

    status->last_release_time = last_key_event->time;

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Main Key release: IDLE");
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_HOLD");
            generic_key_release_when_not_holding_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_RELEASE");
            generic_key_release_when_not_holding_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_TAP");
            break;
        case TAP_DANCE_INTERRUPT_CONFIG_ACTIVE:
            DEBUG_TAP_DANCE("-- Main Key release: INTERRUPT_CONFIG_ACTIVE");
            break;
        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Main Key release: HOLDING");
            generic_key_release_when_holding_handler(config, status, actions, last_key_event);
            break;
    }
}

void handle_timeout(pipeline_tap_dance_behaviour_config_t *config,
                    pipeline_tap_dance_behaviour_status_t *status,
                    pipeline_physical_actions_t* actions) {

    DEBUG_TAP_DANCE("-- Timer callback");

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Timer callback: IDLE");
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            {
                pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
                if (hold_action && hold_action->interrupt_config > 0) {
                    // This was the interrupt config timeout, now wait for hold timeout
                    status->state = TAP_DANCE_INTERRUPT_CONFIG_ACTIVE;
                    platform_time_t remaining_time = g_hold_timeout - hold_action->interrupt_config;
                    if (remaining_time > 0) {
                        pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(remaining_time);
                    } else {
                        // Hold timeout already reached
                        status->state = TAP_DANCE_HOLDING;
                    }
                } else {
                    // Regular hold timeout reached - activate hold action
                    status->state = TAP_DANCE_HOLDING;
                }
            }
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Timer callback: WAITING_FOR_RELEASE");
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Timer callback: WAITING_FOR_TAP");
            {
                // Tap timeout reached - execute tap action
                pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
                if (tap_action != NULL) {
                    // params->key_events->event_buffer_pos ++;
                    // params->key_events->event_buffer[0].keypos = status->key_buffer->event_buffer[0].keypos;
                    // params->key_events->event_buffer[0].keycode = tap_action->keycode;
                    // params->key_events->event_buffer[0].layer = status->selected_layer;
                    // params->key_events->event_buffer[0].is_press = true;
                    // params->key_events->event_buffer[0].time = params->key_events->event_buffer[0].time;

                    // pipeline_executor_end_with_buffer_swap();
                }

                reset_behaviour_state(status);
            }
            break;
        case TAP_DANCE_INTERRUPT_CONFIG_ACTIVE:
            DEBUG_TAP_DANCE("-- Timer callback: INTERRUPT_CONFIG_ACTIVE");
            // Hold timeout reached - activate hold action
            status->state = TAP_DANCE_HOLDING;
            break;

        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Timer callback: HOLDING");
            break;
    }
}

void custom_switch_layer_custom_function(pipeline_physical_callback_params_t* params,
                                        pipeline_tap_dance_behaviour_config_t *config,
                                        pipeline_tap_dance_behaviour_status_t *status,
                                        pipeline_physical_actions_t* actions,
                                        platform_key_event_t* last_key_event) {

    if (config->actionslength == 0) {
        return;
    }

    // Handle interrupting keys (keys other than our trigger key)
    if (last_key_event->keycode != config->keycodemodifier) {
        handle_interrupting_key(config, status, actions, last_key_event);
        return;
    }

    // Handle our trigger key events
    if (params->callback_type == PIPELINE_CALLBACK_KEY_EVENT) {
        if (last_key_event->is_press) {
            handle_key_press(config, status, actions, last_key_event);
        } else {
            handle_key_release(config, status, actions, last_key_event);
        }
    } else if (params->callback_type == PIPELINE_CALLBACK_TIMER) {
        handle_timeout(config, status, actions);
    }
}

void pipeline_tap_dance_global_state_create(void) {
    global_status = (pipeline_tap_dance_global_status_t*)malloc(sizeof(pipeline_tap_dance_global_status_t));
    global_status->last_behaviour = 0;
}

static void pipeline_tap_dance_global_state_reset(pipeline_tap_dance_global_config_t* global_config ) {
    // Reset all behaviours
    for (size_t i = 0; i < global_config->length; i++) {
        pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[i];
        reset_behaviour_state(behaviour->status);
    }

    global_status->last_behaviour = 0;
}

#ifdef DEBUG

static const char* tap_dance_state_to_string(tap_dance_state_t state) {
    switch (state) {
        case TAP_DANCE_IDLE: return "IDLE";
        case TAP_DANCE_WAITING_FOR_HOLD: return "WAITING_FOR_HOLD";
        case TAP_DANCE_WAITING_FOR_RELEASE: return "WAITING_FOR_RELEASE";
        case TAP_DANCE_WAITING_FOR_TAP: return "WAITING_FOR_TAP";
        case TAP_DANCE_INTERRUPT_CONFIG_ACTIVE: return "INTERRUPT_CONFIG_ACTIVE";
        case TAP_DANCE_HOLDING: return "HOLDING";
        default: return "UNKNOWN";
    }
}

void print_tap_dance_status(pipeline_tap_dance_global_config_t* global_config) {
    if (global_config == NULL) {
        DEBUG_PRINT_ERROR("@ Tap Dance: Global config is NULL");
        return;
    }

    DEBUG_PRINT_RAW("# %zu", global_config->length);
    for (size_t i = 0; i < global_config->length; i++) {
        pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[i];
        DEBUG_PRINT_RAW(" # Behaviour %zu: Keycode %d, State %s, Tap Count %d, Layer %d",
                    i, behaviour->config->keycodemodifier, tap_dance_state_to_string(behaviour->status->state),
                    behaviour->status->tap_count, behaviour->status->selected_layer);
    }
    DEBUG_PRINT_NL();
}
#endif

#if defined(DEBUG)
    #define DEBUG_STATE(caption) \
        DEBUG_PRINT_RAW("%s\n", caption); \
        print_tap_dance_status(global_config);
#else
    #define DEBUG_STATE(caption) ((void)0)
#endif

static void pipeline_tap_dance_process(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_tap_dance_global_config_t* global_config) {
    platform_key_event_t* last_key_event = params->key_event;

    if (params->callback_type == PIPELINE_CALLBACK_KEY_EVENT) {
        DEBUG_TAP_DANCE("PIPELINE_CALLBACK_KEY_EVENT: %d", last_key_event->keycode);
        // Process all tap dance behaviors for key events
        for (uint8_t i = 0; i < global_config->length; i++) {
            global_status->last_behaviour = i;
            pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[i];

            behaviour->status->behaviour_index = i;

            custom_switch_layer_custom_function(params, behaviour->config, behaviour->status, actions, last_key_event);
        }
    } else if (params->callback_type == PIPELINE_CALLBACK_TIMER) {
        DEBUG_TAP_DANCE("PIPELINE_CALLBACK_TIMER");
        // Process timeout for the last active behavior
        if (global_status->last_behaviour < global_config->length) {
            pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[global_status->last_behaviour];
            custom_switch_layer_custom_function(params, behaviour->config, behaviour->status, actions, last_key_event);
        }
    }

    DEBUG_STATE("Finished processing tap dance event:");
}

void pipeline_tap_dance_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, void* user_data) {
    pipeline_tap_dance_global_config_t* global_config = (pipeline_tap_dance_global_config_t*)user_data;

    #ifdef DEBUG
    if (global_config == NULL) {
        DEBUG_PRINT_ERROR("Tap Dance: Global config is NULL");
        return;
    }
    DEBUG_TAP_DANCE("Callback called with type %d, row %d, col %d, keycode %d",
           params->callback_type, params->key_event->keypos.row, params->key_event->keypos.col, params->key_event->keycode);
    #endif
    pipeline_tap_dance_process(params, actions, global_config);
}

void pipeline_tap_dance_callback_reset(void* user_data) {
    pipeline_tap_dance_global_config_t* global_config = (pipeline_tap_dance_global_config_t*)user_data;

    #ifdef DEBUG
    if (global_config == NULL) {
        DEBUG_PRINT_ERROR("Tap Dance: Global config is NULL on reset");
        return;
    }
    DEBUG_TAP_DANCE("Resetting all behaviours");
    #endif

    pipeline_tap_dance_global_state_reset(global_config);
}
