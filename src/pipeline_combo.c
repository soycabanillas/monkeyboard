#include "pipeline_combo.h"
#include "monkeyboard_debug.h"
#include "pipeline_executor.h"
#include "platform_interface.h"
#include "platform_types.h"
#include "monkeyboard_time_manager.h"
#include <stdint.h>
#include <string.h>

#if defined(MONKEYBOARD_DEBUG)
    #define PREFIX_DEBUG "COMBO: "
    #define DEBUG_COMBO(...) DEBUG_PRINT_PREFIX(PREFIX_DEBUG, __VA_ARGS__)
    #define DEBUG_COMBO_RAW(...) DEBUG_PRINT_RAW_PREFIX(PREFIX_DEBUG, __VA_ARGS__)
#endif

#define g_interval_timeout 50

// Lookup structure
typedef struct {
    bool found;
    size_t index;
} pipeline_combo_element_found_t;

// Timer management
bool is_time_pending;
platform_time_t next_callback_timestamp;

// Execute combo actions
static void process_key_translation(pipeline_combo_key_translation_t* translation, pipeline_physical_actions_t* actions) {
        if (translation->action == COMBO_KEY_ACTION_NONE) {
        }
        else if (translation->action == COMBO_KEY_ACTION_REGISTER) {
            actions->register_key_fn(translation->key);
        } else if (translation->action == COMBO_KEY_ACTION_UNREGISTER) {
            actions->unregister_key_fn(translation->key);
        } else if (translation->action == COMBO_KEY_ACTION_TAP) {
            actions->tap_key_fn(translation->key);
        }
}

// Key lookup functions
static pipeline_combo_element_found_t find_key_in_combo(pipeline_combo_config_t* combo, platform_keypos_t keypos) {
    pipeline_combo_element_found_t result;
    result.found = false;
    result.index = 0;
    for (size_t i = 0; i < combo->keys_length; i++) {
        pipeline_combo_key_t* key = combo->keys[i];
        if (platform_compare_keyposition(key->keypos, keypos)) {
            result.found = true;
            result.index = i;
            return result;
        }
    }
    return result;
}

// Combo status functions

typedef enum {
    ADD_KEY_ACTIVE_WRONG_STATUS,
    ADD_KEY_ACTIVE_NOT_FOUND,
    ADD_KEY_ACTIVE_PRESSED,
    ADD_KEY_ACTIVE_RELEASED,
    ADD_KEY_ACTIVE_ALL_KEYS_RELEASED
} add_key_to_active_return_status_t;

typedef struct {
    add_key_to_active_return_status_t status;
    pipeline_combo_element_found_t key_info;
} add_key_to_active_return_t;

static bool all_keys_will_be_released(pipeline_combo_config_t* combo, platform_keypos_t keypos, bool is_pressed) {
    for (size_t i = 0; i < combo->keys_length; i++) {
        pipeline_combo_key_t* key = combo->keys[i];
        if ((platform_compare_keyposition(key->keypos, keypos))) {
            if (is_pressed == false) return false;
            if (key->is_pressed == false) return false;
        }
    }
    return true;
}

// Active combo state machine
// It only deals with keys, not any other state of the combo
static add_key_to_active_return_t add_key_to_active_combo(pipeline_combo_config_t* combo, platform_keypos_t keypos, bool is_press) {
    add_key_to_active_return_t result;
    if (combo->combo_status != COMBO_ACTIVE) {
        result.status = ADD_KEY_ACTIVE_WRONG_STATUS;
        return result;
    }
    pipeline_combo_element_found_t key_info = find_key_in_combo(combo, keypos);
    if (key_info.found) {
        pipeline_combo_key_t* combo_key = combo->keys[key_info.index];
        if (all_keys_will_be_released(combo, keypos, is_press)) {
            combo_key->is_pressed = false;

            result.status = ADD_KEY_ACTIVE_ALL_KEYS_RELEASED;
            result.key_info = key_info;
            return result;
        } else {
            if (is_press) {
                combo_key->is_pressed = true;
                result.status = ADD_KEY_ACTIVE_PRESSED;
                result.key_info = key_info;
                return result;
            } else {
                combo_key->is_pressed = false;
                result.status = ADD_KEY_ACTIVE_RELEASED;
                result.key_info = key_info;
                return result;
            }
        }
    }
    result.status = ADD_KEY_ACTIVE_NOT_FOUND;
    return result;
}

typedef enum {
    ADD_KEY_IDLE_WRONG_STATUS,
    ADD_KEY_IDLE_NOT_FOUND,
    ADD_KEY_IDLE_RESETED,
    ADD_KEY_IDLE_INITIALIZED,
    ADD_KEY_IDLE_PRESSED,
    ADD_KEY_IDLE_ALL_KEYS_PRESSED
} add_key_to_idle_return_status_t;

typedef struct {
    add_key_to_idle_return_status_t status;
    platform_time_t timespan;
} add_key_to_idle_return_t;

// Idle combo state machine
// It only deals with keys, not any other state of the combo
static add_key_to_idle_return_t add_key_to_idle_combo(pipeline_combo_config_t* combo, platform_keypos_t keypos, uint8_t press_id, bool is_press, platform_time_t current_time) {
    add_key_to_idle_return_t result;


    if (combo->combo_status != COMBO_IDLE && combo->combo_status != COMBO_IDLE_WAITING_FOR_PRESSES && combo->combo_status != COMBO_IDLE_ALL_KEYS_PRESSED) {
        result.status = ADD_KEY_IDLE_WRONG_STATUS;
        result.timespan = 0;
        return result;
    }

    pipeline_combo_element_found_t key_info = find_key_in_combo(combo, keypos);
    if (key_info.found) {
        if (combo->combo_status == COMBO_IDLE && is_press == true) {
            combo->combo_status = COMBO_IDLE_WAITING_FOR_PRESSES;
            combo->time_from_first_key_event = current_time;
            combo->keys[key_info.index]->is_pressed = true;
            combo->keys[key_info.index]->press_id = press_id;

            result.status = ADD_KEY_IDLE_INITIALIZED;
            result.timespan = 0;
            return result;
        } else if (combo->combo_status == COMBO_IDLE_WAITING_FOR_PRESSES) {
            if (is_press == false) {
                combo->combo_status = COMBO_IDLE;
                for (size_t i = 0; i < combo->keys_length; i++) {
                    combo->keys[i]->is_pressed = false;
                }

                result.status = ADD_KEY_IDLE_RESETED;
                result.timespan = 0;
                return result;
            } else {
                platform_time_t timespan = calculate_time_span(combo->time_from_first_key_event, current_time);
                if (timespan <= g_interval_timeout) {
                    combo->keys[key_info.index]->is_pressed = is_press;
                    combo->keys[key_info.index]->press_id = press_id;
                    bool all_pressed = true;
                    for (size_t i = 0; i < combo->keys_length; i++) {
                        if (!combo->keys[i]->is_pressed) {
                            all_pressed = false;
                            break;
                        }
                    }
                    if (all_pressed) {
                        combo->combo_status = COMBO_IDLE_ALL_KEYS_PRESSED;
                        result.status = ADD_KEY_IDLE_ALL_KEYS_PRESSED;
                    } else {
                        result.status = ADD_KEY_IDLE_PRESSED;
                    }
                    result.timespan = timespan;
                    return result;
                } else {
                    result.status = ADD_KEY_IDLE_RESETED;
                    result.timespan = 0;
                    return result;
                }
            }
        } else if (combo->combo_status == COMBO_IDLE_ALL_KEYS_PRESSED) {
            result.status = ADD_KEY_IDLE_ALL_KEYS_PRESSED;
            result.timespan = 0;
            return result;
        }
    }
    result.status = ADD_KEY_IDLE_WRONG_STATUS;
    result.timespan = 0;
    return result;
}

typedef struct {
    bool combos_on_timer;
    platform_time_t timespan;
    platform_time_t timestamp;
} calculate_next_time_span_t;

/**
 * @brief Calculate the minimum timeout span for combo processing
 * 
 * Scans all combos in COMBO_IDLE_WAITING_FOR_PRESSES state to find the one
 * that will timeout soonest. Returns the time span from current time until
 * the next combo timeout should occur.
 * 
 * This function is used by the deferred callback system to schedule the next
 * timeout event for combo processing. It ensures that combos are properly
 * timed out when the interval expires.
 * 
 * @param global_config Pointer to global combo configuration containing all combos
 * @param current_time Current platform timestamp
 * @return calculate_next_time_span_t Structure containing:
 *         - combos_on_timer: true if any combos need timeout handling, false otherwise
 *         - timespan: Duration in platform time units until next timeout (0 for immediate)
 *         - timestamp: Absolute timestamp when next timeout should occur
 * 
 * @note Uses overflow-safe time calculations via time_is_after() and calculate_time_span()
 * @note Returns timespan=0 when timeout has already passed (immediate execution needed)
 * @note Returns combos_on_timer=false when no combos are waiting for timeout
 */
calculate_next_time_span_t calculate_minimum_time_span(pipeline_combo_global_config_t* global_config, platform_time_t current_time) {
    bool found = false;
    platform_time_t minimal_time = 0;
    
    for (size_t i = 0; i < global_config->length; i++) {
        pipeline_combo_config_t* combo = global_config->combos[i];
        if (combo->combo_status == COMBO_IDLE_WAITING_FOR_PRESSES) {
            if (!found) {
                found = true;
                minimal_time = combo->time_from_first_key_event;
            } else {
                if (time_is_before(combo->time_from_first_key_event, minimal_time)) {
                    minimal_time = combo->time_from_first_key_event;
                }
            }
        }
    }
    
    // Handle case when no combos are found
    if (!found) {
        calculate_next_time_span_t result = { false, 0, 0 };
        return result;
    }
    
    // Calculate when the earliest combo should timeout
    platform_time_t next_execution_time = minimal_time + g_interval_timeout;
    
    // Calculate the time span from current time to next execution
    platform_time_t time_span_to_execution;
    platform_time_t timestamp_to_execution;

    if (time_is_after(next_execution_time, current_time)) {
        // Normal case: timeout is in the future
        time_span_to_execution = calculate_time_span(current_time, next_execution_time);
        timestamp_to_execution = next_execution_time;
    } else {
        // Timeout has already passed, execute immediately
        time_span_to_execution = 0;
        timestamp_to_execution = current_time;
    }
    
    calculate_next_time_span_t result = { found, time_span_to_execution, timestamp_to_execution };
    return result;
}

 
#ifdef MONKEYBOARD_DEBUG
static const char* tap_combo_state_to_string(pipeline_combo_state_t state) {
    switch (state) {
        case COMBO_IDLE: return "IDLE";
        case COMBO_IDLE_WAITING_FOR_PRESSES: return "WAITING_FOR_PRESSES";
        case COMBO_IDLE_ALL_KEYS_PRESSED: return "ALL_KEYS_PRESSED";
        case COMBO_ACTIVE: return "ACTIVE";
        default: return "UNKNOWN";
    }
}

void print_combo_status(pipeline_combo_global_config_t* global_config) {
    if (global_config == NULL) {
        DEBUG_PRINT_ERROR("@ Combo: Global config is NULL");
        return;
    }

    DEBUG_COMBO_RAW("# %zu", global_config->length);
    for (size_t i = 0; i < global_config->length; i++) {
        pipeline_combo_config_t *combo = global_config->combos[i];
            DEBUG_PRINT_RAW(" # ACTIVE %zu: Status:%s First %d, Time %u",
                            i, tap_combo_state_to_string(combo->combo_status), combo->first_key_event, combo->time_from_first_key_event);
            for (size_t j = 0; j < combo->keys_length; j++) {
            #if defined(AGNOSTIC_USE_1D_ARRAY)
                DEBUG_PRINT_RAW(" # ACTIVE %zu: Keypos %d, IsPressed %d, PressId %d",
                    i, combo->keys[j]->keypos, combo->keys[j]->is_pressed, combo->keys[j]->press_id);
            #elif defined(AGNOSTIC_USE_2D_ARRAY)
                DEBUG_PRINT_RAW(" # ACTIVE %zu: Col %d, Row %d, IsPressed %d, PressId %d",
                    i, combo->keys[j]->keypos.col, combo->keys[j]->keypos.row, combo->keys[j]->is_pressed, combo->keys[j]->press_id);
            #endif
            }
    }
    DEBUG_PRINT_NL();
}
#endif

#if defined(MONKEYBOARD_DEBUG)
    #define DEBUG_STATE() \
        print_combo_status(global_config);
#else
    #define DEBUG_STATE() ((void)0)
#endif

void pipeline_combo_callback_process_data(pipeline_physical_callback_params_t* params, pipeline_physical_actions_t* actions, pipeline_physical_return_actions_t* return_actions, void* user_data) {
    pipeline_combo_global_config_t* global_config = (pipeline_combo_global_config_t*)user_data;

    if (params->callback_type == PIPELINE_CALLBACK_KEY_EVENT) {

        // Check if the key event is part of any active combo
        for (size_t i = 0; i < global_config->length; i++) {
            pipeline_combo_config_t* combo = global_config->combos[i];
            if (combo->combo_status == COMBO_ACTIVE) {
                add_key_to_active_return_t when_active_result = add_key_to_active_combo(combo, params->key_event->keypos, params->key_event->is_press);
                switch (when_active_result.status) {
                    case ADD_KEY_ACTIVE_WRONG_STATUS:
                        break;
                    case ADD_KEY_ACTIVE_NOT_FOUND:
                        break;
                    case ADD_KEY_ACTIVE_PRESSED:
                    {
                        pipeline_combo_key_t* combo_key = combo->keys[when_active_result.key_info.index];
                        process_key_translation(&combo_key->key_on_press, actions);
                        break;
                    }
                    case ADD_KEY_ACTIVE_RELEASED:
                    {
                        pipeline_combo_key_t* combo_key = combo->keys[when_active_result.key_info.index];
                        process_key_translation(&combo_key->key_on_release, actions);
                        break;
                    }
                    case ADD_KEY_ACTIVE_ALL_KEYS_RELEASED:
                    {
                        pipeline_combo_key_t* combo_key = combo->keys[when_active_result.key_info.index];
                        process_key_translation(&combo_key->key_on_release, actions);
                        process_key_translation(&combo->key_on_release_combo, actions);
                        combo->combo_status = COMBO_IDLE;
                        combo->first_key_event = false;
                        break;
                    }
                }
                switch (when_active_result.status) {
                    case ADD_KEY_ACTIVE_WRONG_STATUS:
                    case ADD_KEY_ACTIVE_NOT_FOUND:
                        break;
                    case ADD_KEY_ACTIVE_PRESSED:
                    case ADD_KEY_ACTIVE_RELEASED:
                    case ADD_KEY_ACTIVE_ALL_KEYS_RELEASED:
                        if (params->key_event->is_press == true) {
                            actions->remove_physical_press_fn(params->key_event->press_id);
                        } else {
                            actions->remove_physical_release_fn(params->key_event->press_id);
                        }
                        actions->mark_as_processed_fn();
                }
                if (is_time_pending == false) return_actions->no_capture_fn();
                else return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS, 0);
                DEBUG_STATE();
                return;
            }
        }

        pipeline_combo_config_t* first_combo_all_keys_pressed = NULL;
 
        for (size_t i = 0; i < global_config->length; i++) {
            pipeline_combo_config_t* combo = global_config->combos[i];

            add_key_to_idle_return_t result = add_key_to_idle_combo(combo, params->key_event->keypos, params->key_event->press_id, params->key_event->is_press, params->timespan);
            switch (result.status) {
                case ADD_KEY_IDLE_WRONG_STATUS:
                case ADD_KEY_IDLE_NOT_FOUND:
                case ADD_KEY_IDLE_RESETED:
                case ADD_KEY_IDLE_INITIALIZED:
                case ADD_KEY_IDLE_PRESSED:
                case ADD_KEY_IDLE_ALL_KEYS_PRESSED:
                    first_combo_all_keys_pressed = combo;

                    combo->combo_status = COMBO_ACTIVE;
                    combo->first_key_event = false;
            }
        }
        if (first_combo_all_keys_pressed != NULL) {
            // Remove the keys from the physical event buffer and from other idle combos
            for (uint8_t j = 0; j < first_combo_all_keys_pressed->keys_length; j++) {
                actions->remove_physical_press_fn(first_combo_all_keys_pressed->keys[j]->press_id);

                for (size_t k = 0; k < global_config->length; k++) {
                    if (global_config->combos[k]->combo_status != COMBO_ACTIVE) {
                        pipeline_combo_config_t* other_combo = global_config->combos[k];
                        for (size_t l = 0; l < other_combo->keys_length; l++) {
                            if (platform_compare_keyposition(other_combo->keys[l]->keypos, first_combo_all_keys_pressed->keys[j]->keypos)) {
                                other_combo->keys[l]->is_pressed = false;
                                other_combo->combo_status = COMBO_IDLE;
                                other_combo->first_key_event = false;
                            }
                        }
                    }
                }
            }
        }
        calculate_next_time_span_t next_time_span = calculate_minimum_time_span(global_config, params->timespan);
        if (next_time_span.combos_on_timer) {
            if (is_time_pending == true && next_callback_timestamp == next_time_span.timestamp) {
                return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS, 0);
                DEBUG_STATE();
                return;
            } else {
                is_time_pending = true;
                next_callback_timestamp = next_time_span.timestamp;
                return_actions->key_capture_fn(PIPELINE_EXECUTOR_TIMEOUT_NEW, next_time_span.timespan);
                DEBUG_STATE();
                return;
            }
        } else {
            is_time_pending = false;
            return_actions->no_capture_fn();
            DEBUG_STATE();
            return;
        }
    } else if (params->callback_type == PIPELINE_CALLBACK_TIMER) {
        // Handle hold events
    }
    DEBUG_STATE();
}

void pipeline_combo_callback_reset(void* user_data) {
    is_time_pending = false;
    next_callback_timestamp = 0;
}

void pipeline_combo_global_state_create(void) {
    is_time_pending = false;
    next_callback_timestamp = 0;
}
