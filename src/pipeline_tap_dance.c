#include "monkeyboard_debug.h"
#include "pipeline_tap_dance.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "key_event_buffer.h"
#include "pipeline_executor.h"
#include "platform_interface.h"
#include "platform_types.h"
#include "monkeyboard_layer_manager.h"

#if defined(MONKEYBOARD_DEBUG)
    #define PREFIX_DEBUG "TAP_DANCE: "
    #define DEBUG_TAP_DANCE(...) DEBUG_PRINT_PREFIX(PREFIX_DEBUG, __VA_ARGS__)
    #define DEBUG_TAP_DANCE_RAW(...) DEBUG_PRINT_RAW_PREFIX(PREFIX_DEBUG, __VA_ARGS__)
#else
    #define DEBUG_TAP_DANCE(...) ((void)0)
    #define DEBUG_TAP_DANCE_RAW(...) ((void)0)
#endif

pipeline_tap_dance_global_status_t* global_status;

typedef enum {
    NO_CAPTURE,
    CAPTURE_KEYS,
    CAPTURE_KEYS_AND_TIMEOUT
} capture_state_t;

static void update_layer(uint8_t layer, pipeline_physical_actions_t* actions) {
    uint8_t buffer_length = actions->get_physical_key_event_count_fn();
    for (uint8_t i = 0; i < buffer_length; i++) {
        platform_key_event_t* event = actions->get_physical_key_event_fn(i);
        if (event != NULL) {
            platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, event->keypos);
            actions->change_key_code_fn(i, keycode);
        }
    }
}

// Helper functions to find actions by tap count and type
static pipeline_tap_dance_action_config_t* get_action_tap_key_sendkey(uint8_t tap_count, pipeline_tap_dance_behaviour_config_t* config) {
    for (size_t i = 0; i < config->actionslength; i++) {
        pipeline_tap_dance_action_config_t* action = config->actions[i];
        if (action->tap_count == tap_count && action->action == TDCL_TAP_KEY_SENDKEY) {
            return action;
        }
    }
    return NULL;
}

static pipeline_tap_dance_action_config_t* get_action_hold_key_changelayertempo(uint8_t tap_count, pipeline_tap_dance_behaviour_config_t* config) {
    for (size_t i = 0; i < config->actionslength; i++) {
        pipeline_tap_dance_action_config_t* action = config->actions[i];
        if (action->tap_count == tap_count && action->action == TDCL_HOLD_KEY_CHANGELAYERTEMPO) {
            return action;
        }
    }
    return NULL;
}

static bool has_subsequent_actions(pipeline_tap_dance_behaviour_config_t* config, uint8_t tap_count) {
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
}

static void handle_interrupting_key(pipeline_tap_dance_behaviour_config_t *config,
                             pipeline_tap_dance_behaviour_status_t *status,
                             pipeline_physical_actions_t* actions,
                             pipeline_physical_return_actions_t* return_actions,
                             platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Interrupting Key Event: %d, state: %d", last_key_event->keycode, status->state);

    // Only handle interruptions during hold waiting states
    if (status->state != TAP_DANCE_WAITING_FOR_HOLD) {
        return_actions->no_capture_fn();
        return;
    }

    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (!hold_action) {
        return_actions->no_capture_fn();
        return;
    }

    if (hold_action->hold_strategy == TAP_DANCE_HOLD_PREFERRED) {
        if (last_key_event->is_press) {
            status->state = TAP_DANCE_HOLDING;
            uint8_t press_id = actions->get_physical_key_event_fn(0)->press_id;
            actions->remove_physical_press_fn(press_id);
            if (platform_layout_is_valid_layer(hold_action->layer)) {
                update_layer(hold_action->layer, actions);
                layout_manager_add_layer(status->trigger_keypos, press_id, hold_action->layer);
            }
            return_actions->no_capture_fn();
        } else {
            // uint8_t buffer_length = actions->get_physical_key_event_count_fn();
            // if (buffer_length == 2 && has_subsequent_actions(config, status->tap_count) == false) {
            //     DEBUG_EXECUTOR("----- Unregistering key: %d ------", last_key_event->keycode);
            //     actions->unregister_key_fn(last_key_event->keycode);
            //     actions->remove_physical_release_fn(last_key_event->press_id);
            // }
            // return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS, 0);
        }
        return;
    } else if (hold_action->hold_strategy == TAP_DANCE_TAP_PREFERRED) {
        return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS, 0);
        return;
    } else if (hold_action->hold_strategy == TAP_DANCE_BALANCED) {
        DEBUG_TAP_DANCE("Interrupting when TAP_DANCE_BALANCED");
        if (last_key_event->is_press) {
            return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS, 0);
            return;
        } else {
            bool press_found_on_buffer = false;
            uint8_t buffer_length = actions->get_physical_key_event_count_fn();
            // for (uint8_t i = 1; i < buffer_length - 1; i++) {
            for (uint8_t i = 1; i < buffer_length; i++) {
                platform_key_event_t* event = actions->get_physical_key_event_fn(i);
                DEBUG_TAP_DANCE("Buffer Event %d: %d-%d", i, event->keypos.row, event->keypos.col);
                if (event != NULL) {
                    press_found_on_buffer = event->is_press == true && platform_compare_keyposition(event->keypos, last_key_event->keypos);
                    if (press_found_on_buffer) break;
                }
            }
            if (press_found_on_buffer) {
                status->state = TAP_DANCE_HOLDING;
                uint8_t press_id = actions->get_physical_key_event_fn(0)->press_id;
                actions->remove_physical_press_fn(press_id);
                if (platform_layout_is_valid_layer(hold_action->layer)) {
                    update_layer(hold_action->layer, actions);
                    layout_manager_add_layer(status->trigger_keypos, press_id, hold_action->layer);
                }
                return_actions->no_capture_fn();
            } else {
                // if (buffer_length == 2 && has_subsequent_actions(config, status->tap_count) == false) {
                //     DEBUG_PRINT("----- Unregistering key: %d ------", last_key_event->keycode);
                //     actions->unregister_key_fn(last_key_event->keycode);
                //     actions->remove_physical_release_fn(last_key_event->press_id);
                // } else {
                // }
                // return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS, 0);
            }
            return;
        }
    }
}

static void generic_key_press_handler(pipeline_tap_dance_behaviour_config_t *config,
                               pipeline_tap_dance_behaviour_status_t *status,
                               pipeline_physical_actions_t* actions,
                               pipeline_physical_return_actions_t* return_actions,
                               platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Press Handler: %d", last_key_event->keycode);
    status->tap_count++;

    // Check if we have a hold action for this tap count
    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (hold_action != NULL) {
        status->state = TAP_DANCE_WAITING_FOR_HOLD;
        status->selected_layer = hold_action->layer;
        return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_NEW, config->hold_timeout);
        return;
    } else {
        if (has_subsequent_actions(config, status->tap_count)) {
            status->state = TAP_DANCE_WAITING_FOR_RELEASE;
            return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_NONE, 0);
            return;
        } else {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->change_key_code_fn(0, tap_action->keycode);
                reset_behaviour_state(status);
            } else {
                actions->remove_physical_tap_fn(last_key_event->press_id);
            }
            return_actions->no_capture_fn();
            return;
        }
    }
}

static void generic_key_release_when_not_holding_handler(pipeline_tap_dance_behaviour_config_t *config,
                                                 pipeline_tap_dance_behaviour_status_t *status,
                                                 pipeline_physical_actions_t* actions,
                                                 pipeline_physical_return_actions_t* return_actions,
                                                 platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Release Handler (Not Holding): %d", last_key_event->keycode);
    if (has_subsequent_actions(config, status->tap_count)) {
        status->state = TAP_DANCE_WAITING_FOR_TAP;
        actions->remove_physical_tap_fn(last_key_event->press_id);
        return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_NEW, config->tap_timeout);
        return;
    } else {
        if (status->state == TAP_DANCE_WAITING_FOR_HOLD) {
            pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            if (tap_action != NULL) {
                actions->change_key_code_fn(0, tap_action->keycode);
            } else {
                actions->remove_physical_tap_fn(last_key_event->press_id);
            }
            reset_behaviour_state(status);
            return_actions->no_capture_fn();
            return;
        } else if (status->state == TAP_DANCE_WAITING_FOR_RELEASE) {
            // pipeline_tap_dance_action_config_t* tap_action = get_action_tap_key_sendkey(status->tap_count, config);
            // if (tap_action != NULL) {
            //     actions->remove_physical_release_fn(last_key_event->press_id);
            //     actions->unregister_key_fn(tap_action->keycode);
            // }
            reset_behaviour_state(status);
            return_actions->no_capture_fn();
            return;
        }
    }
}

static void generic_key_release_when_holding_handler(pipeline_tap_dance_behaviour_config_t *config,
                                              pipeline_tap_dance_behaviour_status_t *status,
                                              pipeline_physical_actions_t* actions,
                                              pipeline_physical_return_actions_t* return_actions,
                                              platform_key_event_t* last_key_event) {
    DEBUG_TAP_DANCE("Generic Key Release Handler (Holding): %d", last_key_event->keycode);

    pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
    if (!hold_action) {
        return_actions->no_capture_fn();
        return;
    }

    if (hold_action->hold_strategy == TAP_DANCE_HOLD_PREFERRED) {
        layout_manager_remove_layer_by_keypos(status->trigger_keypos);
        actions->remove_physical_release_fn(last_key_event->press_id);
        reset_behaviour_state(status);
        return_actions->no_capture_fn();
        return;
    } else if (hold_action->hold_strategy == TAP_DANCE_TAP_PREFERRED) {
        layout_manager_remove_layer_by_keypos(status->trigger_keypos);
        actions->remove_physical_release_fn(last_key_event->press_id);
        reset_behaviour_state(status);
        return_actions->no_capture_fn();
        return;
    } else if (hold_action->hold_strategy == TAP_DANCE_BALANCED) {
        layout_manager_remove_layer_by_keypos(status->trigger_keypos);
        actions->remove_physical_release_fn(last_key_event->press_id);
        reset_behaviour_state(status);
        return_actions->no_capture_fn();
        return;
    }
}

static void handle_key_press(pipeline_tap_dance_behaviour_config_t *config,
                     pipeline_tap_dance_behaviour_status_t *status,
                     pipeline_physical_actions_t* actions,
                     pipeline_physical_return_actions_t* return_actions,
                     platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Main Key press: %d, state: %d", last_key_event->keycode, status->state);

    if (config->actionslength == 0) {
        // No actions configured, just return no capture
        actions->remove_physical_tap_fn(last_key_event->press_id);
        return_actions->no_capture_fn();
        return;
    }

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Main Key press: IDLE");
            // First press of a new sequence
            status->original_layer = platform_layout_get_current_layer(); // Use current layer from stack
            status->trigger_keypos = last_key_event->keypos; // Store the key position that triggered the tap dance
            generic_key_press_handler(config, status, actions, return_actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_HOLD");
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_RELEASE");
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Main Key press: WAITING_FOR_TAP");
            generic_key_press_handler(config, status, actions, return_actions, last_key_event);
            break;
        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Main Key press: HOLDING");
            break;
    }
}

static void handle_key_release(pipeline_tap_dance_behaviour_config_t *config,
                        pipeline_tap_dance_behaviour_status_t *status,
                        pipeline_physical_actions_t* actions,
                        pipeline_physical_return_actions_t* return_actions,
                        platform_key_event_t* last_key_event) {

    DEBUG_TAP_DANCE("-- Main Key release: %d, state: %d", last_key_event->keycode, status->state);

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Main Key release: IDLE");
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_HOLD");
            generic_key_release_when_not_holding_handler(config, status, actions, return_actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_RELEASE:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_RELEASE");
            generic_key_release_when_not_holding_handler(config, status, actions, return_actions, last_key_event);
            break;
        case TAP_DANCE_WAITING_FOR_TAP:
            DEBUG_TAP_DANCE("-- Main Key release: WAITING_FOR_TAP");
            break;
        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Main Key release: HOLDING");
            generic_key_release_when_holding_handler(config, status, actions, return_actions, last_key_event);
            break;
    }
}

static void handle_timeout(pipeline_tap_dance_behaviour_config_t *config,
                    pipeline_tap_dance_behaviour_status_t *status,
                    pipeline_physical_actions_t* actions,
                    pipeline_physical_return_actions_t* return_actions) {

    DEBUG_TAP_DANCE("-- Timer callback");

    switch (status->state) {
        case TAP_DANCE_IDLE:
            DEBUG_TAP_DANCE("-- Timer callback: IDLE");
            break;
        case TAP_DANCE_WAITING_FOR_HOLD:
            {
                pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(status->tap_count, config);
                if (hold_action != NULL) {
                    if (hold_action->hold_strategy == TAP_DANCE_HOLD_PREFERRED) {
                        status->state = TAP_DANCE_HOLDING;
                        uint8_t press_id = actions->get_physical_key_event_fn(0)->press_id;
                        actions->remove_physical_press_fn(press_id);
                        if (platform_layout_is_valid_layer(hold_action->layer)) {
                            layout_manager_add_layer(status->trigger_keypos, press_id, hold_action->layer);
                        }
                        return_actions->no_capture_fn();
                        return;
                    } else if (hold_action->hold_strategy == TAP_DANCE_TAP_PREFERRED) {
                        status->state = TAP_DANCE_HOLDING;
                        uint8_t press_id = actions->get_physical_key_event_fn(0)->press_id;
                        actions->remove_physical_press_fn(press_id);
                        update_layer(hold_action->layer, actions);
                        if (platform_layout_is_valid_layer(hold_action->layer)) {
                            layout_manager_add_layer(status->trigger_keypos, press_id, hold_action->layer);
                        }
                        return_actions->no_capture_fn();
                        return;
                    } else if (hold_action->hold_strategy == TAP_DANCE_BALANCED) {
                        status->state = TAP_DANCE_HOLDING;
                        uint8_t press_id = actions->get_physical_key_event_fn(0)->press_id;
                        actions->remove_physical_press_fn(press_id);
                        update_layer(hold_action->layer, actions);
                        if (platform_layout_is_valid_layer(hold_action->layer)) {
                            layout_manager_add_layer(status->trigger_keypos, press_id, hold_action->layer);
                        }
                        return_actions->no_capture_fn();
                        return;
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
                    reset_behaviour_state(status);
                    return_actions->no_capture_fn();
                    return;
                } else {
                    reset_behaviour_state(status);
                    return_actions->no_capture_fn();
                    return;
                }
            }
            break;

        case TAP_DANCE_HOLDING:
            DEBUG_TAP_DANCE("-- Timer callback: HOLDING");
            break;
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

#ifdef MONKEYBOARD_DEBUG

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

    DEBUG_TAP_DANCE_RAW("# %zu", global_config->length);
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

#if defined(MONKEYBOARD_DEBUG)
    #define DEBUG_STATE() \
        print_tap_dance_status(global_config);
#else
    #define DEBUG_STATE() ((void)0)
#endif

static void pipeline_tap_dance_process(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, pipeline_tap_dance_global_config_t* global_config) {
    platform_key_event_t* last_key_event = params->key_event;

    if (params->callback_type == PIPELINE_CALLBACK_KEY_EVENT) {
        DEBUG_TAP_DANCE("PIPELINE_CALLBACK_KEY_EVENT: %d", last_key_event->keycode);
        if (params->is_capturing_keys) {
            DEBUG_TAP_DANCE("IS CAPTURING");

            pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[global_status->last_behaviour];
            pipeline_tap_dance_behaviour_config_t *config = behaviour->config;
            pipeline_tap_dance_behaviour_status_t *status = behaviour->status;
            if (last_key_event->keycode != config->keycodemodifier) {
                handle_interrupting_key(config, status, actions, return_actions, last_key_event);
            } else {
                if (platform_compare_keyposition(last_key_event->keypos, status->trigger_keypos) == false) {
                    DEBUG_TAP_DANCE("Skipping behaviour %zu for key %d, not matching trigger keypos", global_status->last_behaviour, last_key_event->keycode);
                    actions->remove_physical_tap_fn(last_key_event->press_id);
                    return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_NONE, 0);
                } else {
                    if (last_key_event->is_press) {
                        handle_key_press(config, status, actions, return_actions, last_key_event);
                    } else {
                        handle_key_release(config, status, actions, return_actions, last_key_event);
                    }
                }
            }
        } else {
            DEBUG_TAP_DANCE("IS NOT CAPTURING");
            // Process all tap dance behaviors for key events
            for (size_t i = 0; i < global_config->length; i++) {
                pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[i];
                pipeline_tap_dance_behaviour_config_t *config = behaviour->config;
                pipeline_tap_dance_behaviour_status_t *status = behaviour->status;

                if (last_key_event->keycode == config->keycodemodifier) {
                    if (status->state != TAP_DANCE_IDLE && platform_compare_keyposition(last_key_event->keypos, status->trigger_keypos) == false) {
                        DEBUG_TAP_DANCE("Skipping behaviour %zu for key %d, not matching trigger keypos", i, last_key_event->keycode);
                        actions->remove_physical_tap_fn(last_key_event->press_id);
                    } else {
                        if (last_key_event->is_press) {
                            handle_key_press(config, status, actions, return_actions, last_key_event);
                        } else {
                            handle_key_release(config, status, actions, return_actions, last_key_event);
                        }
                        global_status->last_behaviour = i;
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
                handle_timeout(config, status, actions, return_actions);
            }
        }
    }
    DEBUG_STATE();
}

void pipeline_tap_dance_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, pipeline_tap_dance_global_config_t* config) {
    if (config == NULL) {
        DEBUG_PRINT_ERROR("Tap Dance: Global config is NULL");
        return;
    }
    pipeline_tap_dance_process(params, actions, return_actions, config);
}

void pipeline_tap_dance_callback_process_data_executor(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, void* config) {
    pipeline_tap_dance_callback_process_data(params, actions, return_actions, config);
}

void pipeline_tap_dance_callback_reset(pipeline_tap_dance_global_config_t* config) {

    if (config == NULL) {
        DEBUG_PRINT_ERROR("Tap Dance: Global config is NULL on reset");
        return;
    }
    DEBUG_TAP_DANCE("Resetting all behaviours");

    pipeline_tap_dance_global_state_reset(config);
}

void pipeline_tap_dance_callback_reset_executor(void* config) {

}
