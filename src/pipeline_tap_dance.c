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

pipeline_tap_dance_global_status_t* global_status;

typedef enum {
    NO_CAPTURE,
    CAPTURE_KEYS,
    CAPTURE_KEYS_AND_TIMEOUT
} capture_state_t;

typedef struct {
    capture_state_t return_data;
    platform_time_t callback_time;
    bool capturing_keys;
} tap_dance_return_data_t;

static tap_dance_return_data_t end_with_no_capture(void) {
    tap_dance_return_data_t tap_dance_return_data;
    tap_dance_return_data.return_data = NO_CAPTURE;
    tap_dance_return_data.capturing_keys = false;
    return tap_dance_return_data;
}

static tap_dance_return_data_t end_with_capture_next_keys(void) {
    tap_dance_return_data_t tap_dance_return_data;
    tap_dance_return_data.return_data = CAPTURE_KEYS;
    tap_dance_return_data.capturing_keys = true;
    return tap_dance_return_data;
}

static tap_dance_return_data_t end_with_capture_next_keys_or_callback_on_timeout(platform_time_t callback_time) {
    tap_dance_return_data_t tap_dance_return_data;
    tap_dance_return_data.return_data = CAPTURE_KEYS_AND_TIMEOUT;
    tap_dance_return_data.capturing_keys = true;
    tap_dance_return_data.callback_time = callback_time;
    return tap_dance_return_data;
}

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
    // platform_key_event_reset(status->key_buffer);
}

tap_dance_return_data_t handle_interrupting_key(pipeline_tap_dance_behaviour_config_t *config,
                             pipeline_tap_dance_behaviour_status_t *status,
                             pipeline_physical_actions_t* actions,
                             platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Interrupting Key Event: %d, state: %d", last_key_event->keycode, status->state);

    // Only handle interruptions during hold waiting states
    if (status->state != TAP_DANCE_WAITING_FOR_HOLD) {
        return end_with_no_capture();
    }

    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (!hold_action) return end_with_no_capture();

    if (hold_action->hold_strategy == TAP_DANCE_HOLD_PREFERRED) {
        if (last_key_event->is_press) {
            status->state = TAP_DANCE_HOLDING;
            actions->update_layer_for_physical_events_fn(hold_action->layer, 0);
            platform_layout_set_layer(hold_action->layer);
            return end_with_no_capture();
        } else {
            return end_with_no_capture();
        }
    } else if (hold_action->hold_strategy == TAP_DANCE_TAP_PREFERRED) {
        if (last_key_event->is_press) {
            return end_with_no_capture();
        } else {
            status->state = TAP_DANCE_HOLDING;
            actions->update_layer_for_physical_events_fn(hold_action->layer, 0);
            platform_layout_set_layer(hold_action->layer);
            return end_with_no_capture();
        }
    } else if (hold_action->hold_strategy == TAP_DANCE_BALANCED) {
        return end_with_no_capture();
    }
    return end_with_no_capture();
}

tap_dance_return_data_t generic_key_press_handler(pipeline_tap_dance_behaviour_config_t *config,
                               pipeline_tap_dance_behaviour_status_t *status,
                               pipeline_physical_actions_t* actions,
                               platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Press Handler: %d", last_key_event->keycode);
    status->tap_count++;

    // Check if we have a hold action for this tap count
    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (hold_action != NULL) {
        status->state = TAP_DANCE_WAITING_FOR_HOLD;
        status->selected_layer = hold_action->layer;
        return end_with_capture_next_keys_or_callback_on_timeout(g_hold_timeout);
    } else {
        if (has_subsequent_actions(config, status->tap_count)) {
            status->state = TAP_DANCE_WAITING_FOR_RELEASE;
            return end_with_capture_next_keys();
        } else {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->remove_physical_press_fn(last_key_event->press_id);
                actions->register_key_fn(tap_action->keycode);
                status->state = TAP_DANCE_WAITING_FOR_RELEASE;
            }
            return end_with_no_capture();
        }
    }
    return end_with_no_capture();
}

tap_dance_return_data_t generic_key_release_when_not_holding_handler(pipeline_tap_dance_behaviour_config_t *config,
                                                 pipeline_tap_dance_behaviour_status_t *status,
                                                 pipeline_physical_actions_t* actions,
                                                 platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Release Handler (Not Holding): %d", last_key_event->keycode);
    if (has_subsequent_actions(config, status->tap_count)) {
        status->state = TAP_DANCE_WAITING_FOR_TAP;
        actions->remove_physical_tap_fn(last_key_event->press_id);
        return end_with_capture_next_keys_or_callback_on_timeout(g_tap_timeout);
    } else {
        if (status->state == TAP_DANCE_WAITING_FOR_HOLD) {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->remove_physical_tap_fn(last_key_event->press_id);
                actions->tap_key_fn(tap_action->keycode);
            }
            reset_behaviour_state(status);
            return end_with_no_capture();
        } else if (status->state == TAP_DANCE_WAITING_FOR_RELEASE) {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->remove_physical_release_fn(last_key_event->press_id);
                actions->unregister_key_fn(tap_action->keycode);
            }
            reset_behaviour_state(status);
            return end_with_no_capture();
        }
    }
    return end_with_no_capture();
}

tap_dance_return_data_t generic_key_release_when_holding_handler(pipeline_tap_dance_behaviour_config_t *config,
                                              pipeline_tap_dance_behaviour_status_t *status,
                                              pipeline_physical_actions_t* actions,
                                              platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Release Handler (Holding): %d", last_key_event->keycode);

    platform_layout_set_layer(status->original_layer);
    actions->remove_physical_release_fn(last_key_event->press_id);
    reset_behaviour_state(status);
    return end_with_no_capture();
}

tap_dance_return_data_t handle_key_press(pipeline_tap_dance_behaviour_config_t *config,
                     pipeline_tap_dance_behaviour_status_t *status,
                     pipeline_physical_actions_t* actions,
                     platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Main Key press: %d, state: %d", last_key_event->keycode, status->state);

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Main Key press: IDLE");
            // First press of a new sequence
            status->original_layer = platform_layout_get_current_layer(); // Use current layer from stack
            status->trigger_keypos = last_key_event->keypos; // Store the key position that triggered the tap dance
            return generic_key_press_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_HOLD");
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_RELEASE");
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_TAP");
            return generic_key_press_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Main Key press: HOLDING");
            break;
    }
    return end_with_no_capture();
}

tap_dance_return_data_t handle_key_release(pipeline_tap_dance_behaviour_config_t *config,
                        pipeline_tap_dance_behaviour_status_t *status,
                        pipeline_physical_actions_t* actions,
                        platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Main Key release: %d, state: %d", last_key_event->keycode, status->state);

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Main Key release: IDLE");
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_HOLD");
            return generic_key_release_when_not_holding_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_RELEASE");
            return generic_key_release_when_not_holding_handler(config, status, actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_TAP");
            break;
        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Main Key release: HOLDING");
            return generic_key_release_when_holding_handler(config, status, actions, last_key_event);
            break;
    }
    return end_with_no_capture();
}

tap_dance_return_data_t handle_timeout(pipeline_tap_dance_behaviour_config_t *config,
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
                if (hold_action) {
                    if (hold_action->hold_strategy == TAP_DANCE_HOLD_PREFERRED) {
                        status->state = TAP_DANCE_HOLDING;
                        //actions->update_layer_for_physical_events_fn(hold_action->layer, 0);
                        uint8_t press_id = actions->get_physical_key_event_fn(actions->get_physical_key_event_count_fn() - 1)->press_id;
                        actions->remove_physical_press_fn(press_id);
                        platform_layout_set_layer(hold_action->layer);
                        return end_with_no_capture();
                    } else if (hold_action->hold_strategy == TAP_DANCE_TAP_PREFERRED) {
                        status->state = TAP_DANCE_HOLDING;
                        actions->update_layer_for_physical_events_fn(hold_action->layer, 0);
                        uint8_t press_id = actions->get_physical_key_event_fn(actions->get_physical_key_event_count_fn() - 1)->press_id;
                        actions->remove_physical_release_fn(press_id);
                        platform_layout_set_layer(hold_action->layer);
                        return end_with_no_capture();
                    } else if (hold_action->hold_strategy == TAP_DANCE_BALANCED) {
                        status->state = TAP_DANCE_HOLDING;
                        actions->update_layer_for_physical_events_fn(hold_action->layer, 0);
                        uint8_t press_id = actions->get_physical_key_event_fn(actions->get_physical_key_event_count_fn() - 1)->press_id;
                        actions->remove_physical_release_fn(press_id);
                        platform_layout_set_layer(hold_action->layer);
                        return end_with_no_capture();
                    }
                }
            }
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Timer callback: WAITING_FOR_RELEASE");
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Timer callback: WAITING_FOR_TAP");
            {
                pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
                if (tap_action != NULL) {
                    actions->tap_key_fn(tap_action->keycode);
                }
                reset_behaviour_state(status);
                return end_with_no_capture();
            }
            break;

        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Timer callback: HOLDING");
            break;
    }
    return end_with_no_capture();
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
        #if defined(AGNOSTIC_USE_1D_ARRAY)
                    DEBUG_PRINT_RAW(" # Behaviour %zu: Keycode %d, State %s, Tap Count %d, Layer %d, KP %d",
                    i, behaviour->config->keycodemodifier, tap_dance_state_to_string(behaviour->status->state),
                    behaviour->status->tap_count, behaviour->status->selected_layer,
                    behaviour->status->trigger_keypos);
        #elif defined(AGNOSTIC_USE_2D_ARRAY)
            DEBUG_PRINT_RAW(" # Behaviour %zu: Keycode %d, State %s, Tap Count %d, Layer %d, Col %d, Row %d",
                        i, behaviour->config->keycodemodifier, tap_dance_state_to_string(behaviour->status->state),
                        behaviour->status->tap_count, behaviour->status->selected_layer,
                        behaviour->status->trigger_keypos.row, behaviour->status->trigger_keypos.col);
        #endif
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

    tap_dance_return_data_t tap_dance_return_data = end_with_no_capture();

    if (params->callback_type == PIPELINE_CALLBACK_KEY_EVENT) {
        DEBUG_TAP_DANCE("PIPELINE_CALLBACK_KEY_EVENT: %d", last_key_event->keycode);
        if (global_status->is_capturing_keys) {
            pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[global_status->last_behaviour];
            pipeline_tap_dance_behaviour_config_t *config = behaviour->config;
            pipeline_tap_dance_behaviour_status_t *status = behaviour->status;
            if (last_key_event->keycode != config->keycodemodifier) {
                tap_dance_return_data = handle_interrupting_key(config, status, actions, last_key_event);
            } else {
                if (platform_compare_keyposition(last_key_event->keypos, status->trigger_keypos) == false) {
                    DEBUG_TAP_DANCE("Skipping behaviour %zu for key %d, not matching trigger keypos", global_status->last_behaviour, last_key_event->keycode);
                    actions->remove_physical_tap_fn(last_key_event->press_id);
                    tap_dance_return_data = end_with_capture_next_keys();
                } else {
                    if (last_key_event->is_press) {
                        tap_dance_return_data = handle_key_press(config, status, actions, last_key_event);
                    } else {
                        tap_dance_return_data = handle_key_release(config, status, actions, last_key_event);
                    }
                }
            }
        } else {
            // Process all tap dance behaviors for key events
            for (size_t i = 0; i < global_config->length; i++) {
                pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[i];
                pipeline_tap_dance_behaviour_config_t *config = behaviour->config;
                pipeline_tap_dance_behaviour_status_t *status = behaviour->status;

                if (config->actionslength > 0) {
                    if (last_key_event->keycode == config->keycodemodifier) {
                        if (platform_compare_keyposition(last_key_event->keypos, status->trigger_keypos) == false) {
                            DEBUG_TAP_DANCE("Skipping behaviour %zu for key %d, not matching trigger keypos", i, last_key_event->keycode);
                            actions->remove_physical_tap_fn(last_key_event->press_id);
                        } else {
                            if (last_key_event->is_press) {
                                tap_dance_return_data = handle_key_press(config, status, actions, last_key_event);
                            } else {
                                tap_dance_return_data = handle_key_release(config, status, actions, last_key_event);
                            }
                            global_status->last_behaviour = i;
                        }
                    }
                }
            }
        }
    } else if (params->callback_type == PIPELINE_CALLBACK_TIMER) {
        DEBUG_TAP_DANCE("PIPELINE_CALLBACK_TIMER");
        // Process timeout for the last active behavior
        if (global_status->last_behaviour < global_config->length) {
            pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[global_status->last_behaviour];
            pipeline_tap_dance_behaviour_config_t *config = behaviour->config;
            pipeline_tap_dance_behaviour_status_t *status = behaviour->status;

            if (config->actionslength > 0) {
                tap_dance_return_data = handle_timeout(config, status, actions);
            }
        }
    }

    if (tap_dance_return_data.capturing_keys) {
        global_status->is_capturing_keys = true;
        // DEBUG_TAP_DANCE("Capturing keys for tap dance behaviour %zu", global_status->last_behaviour);
    } else {
        global_status->is_capturing_keys = false;
    }
    switch (tap_dance_return_data.return_data) {
        case NO_CAPTURE:
            break;
        case CAPTURE_KEYS:
            pipeline_executor_end_with_capture_next_keys();
            break;
        case CAPTURE_KEYS_AND_TIMEOUT:
            pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(tap_dance_return_data.callback_time);
            break;
    }

    DEBUG_STATE("Finished processing tap dance event:");
}

void pipeline_tap_dance_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, void* user_data) {
    pipeline_tap_dance_global_config_t* global_config = (pipeline_tap_dance_global_config_t*)user_data;

    if (global_config == NULL) {
        DEBUG_PRINT_ERROR("Tap Dance: Global config is NULL");
        return;
    }
    pipeline_tap_dance_process(params, actions, global_config);
}

void pipeline_tap_dance_callback_reset(void* user_data) {
    pipeline_tap_dance_global_config_t* global_config = (pipeline_tap_dance_global_config_t*)user_data;

    if (global_config == NULL) {
        DEBUG_PRINT_ERROR("Tap Dance: Global config is NULL on reset");
        return;
    }
    DEBUG_TAP_DANCE("Resetting all behaviours");

    pipeline_tap_dance_global_state_reset(global_config);
}
