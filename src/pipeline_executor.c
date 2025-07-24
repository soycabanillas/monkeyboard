#include "pipeline_executor.h"
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include "key_event_buffer.h"
#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"

pipeline_executor_state_t pipeline_executor_state;
pipeline_executor_config_t *pipeline_executor_config;

//Functions available in the pipeline_info_t struct

static void register_key(platform_keycode_t keycode, platform_keypos_t keypos) {
    platform_register_keycode(keycode);
}

static void unregister_key(platform_keycode_t keycode) {
    platform_unregister_keycode(keycode);
}

static void tap_key(platform_keycode_t keycode, platform_keypos_t keypos) {
    platform_tap_keycode(keycode);
}

static void remove_physical_press_and_release(platform_keypos_t keypos) {
    platform_key_event_remove_physical_press_and_release(pipeline_executor_state.key_event_buffer, keypos);
}

static void update_layer_for_physical_events(uint8_t layer, uint8_t pos) {
    platform_key_event_update_layer_for_physical_events(pipeline_executor_state.key_event_buffer, layer, pos);
}

// End of functions available in the pipeline_info_t struct

/**
 * Capture mechanism: Allows a pipeline to "capture" subsequent key events
 * and optionally set a timeout callback. When captured:
 * - All following key events go to the capturing pipeline only
 * - Other pipelines are bypassed until capture is released
 * - If timeout is set, callback is triggered after specified time
 * - Used for multi-key sequences, tap dance timing, etc.
 */
static void execute_pipeline(uint16_t callback_time, uint8_t pipeline_index, platform_key_event_buffer_t* key_events, platform_key_event_t* press_buffer_selected, capture_pipeline_t* return_data) {
    pipeline_callback_params_t callback_params;
    if (callback_time == 0) {
        callback_params.key_events = key_events;
        callback_params.callback_type = PIPELINE_CALLBACK_KEY_EVENT;
    } else {
        callback_params.callback_type = PIPELINE_CALLBACK_TIMER;
        callback_params.callback_time = callback_time;
    }

    pipeline_actions_t actions;
    actions.register_key_fn = &register_key;
    actions.unregister_key_fn = &unregister_key;
    actions.tap_key_fn = &tap_key;
    actions.remove_physical_press_and_release_fn = &remove_physical_press_and_release;
    actions.update_layer_for_physical_events_fn = &update_layer_for_physical_events;
    pipeline_executor_config->pipelines[pipeline_index]->callback(&callback_params, &actions, pipeline_executor_config->pipelines[pipeline_index]->data);
}

static void execute_middleware(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, platform_key_event_t* press_buffer_selected) {
    execute_pipeline(0, pipeline_index, pipeline_executor_state->key_event_buffer, press_buffer_selected, &pipeline_executor_state->return_data);
}

// Executes the middleware when the timer callback is triggered
static void deferred_exec_callback(void *cb_arg) {
    (void)cb_arg; // Unused parameter
    execute_pipeline(pipeline_executor_state.return_data.callback_time, pipeline_executor_state.pipeline_index, pipeline_executor_state.key_event_buffer, NULL, &pipeline_executor_state.return_data);
}

// Execute the middleware when a key event occurs
// Returns false if any key was digested by the middleware
// This function is called by the platform when a key event occurs
// It processes the key events and executes the pipelines accordingly
// It also cancels any pending deferred execution callbacks
// and resets the key event buffer for the next iteration
static bool process_key_pool(void) {

    if (pipeline_executor_state.return_data.callback_time > 0) {
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }
    size_t pipeline_index = 0;
    if (pipeline_executor_state.return_data.capture_key_events == true) {
        pipeline_index = pipeline_executor_state.pipeline_index;
    }

    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.return_data.key_buffer_changed = false; // Reset swap buffer flag

    platform_key_event_t* press_buffer_selected = &pipeline_executor_state.key_event_buffer->event_buffer[0];

    capture_pipeline_t last_execution = pipeline_executor_state.return_data;
    if (last_execution.capture_key_events == true) {
        execute_middleware(&pipeline_executor_state, pipeline_index, press_buffer_selected);
        last_execution = pipeline_executor_state.return_data;
    }
    if (last_execution.capture_key_events == false)
    {
        for (size_t i = pipeline_index + 1; i < pipeline_executor_config->length; i++) {
            pipeline_executor_state.pipeline_index = i;
            execute_middleware(&pipeline_executor_state, i, press_buffer_selected);
            last_execution = pipeline_executor_state.return_data;
            if (last_execution.capture_key_events == true) break;
        }
    }

    if (pipeline_executor_state.return_data.callback_time > 0) {
        pipeline_executor_state.deferred_exec_callback_token = platform_defer_exec(pipeline_executor_state.return_data.callback_time, deferred_exec_callback, NULL);
    }

    if (last_execution.capture_key_events == false && last_execution.callback_time == 0) {
        for (size_t pos = 0; pos < pipeline_executor_state.key_event_buffer->event_buffer_pos; pos++) {
            if (pipeline_executor_state.key_event_buffer->event_buffer[pos].is_press) {
                platform_register_keycode(pipeline_executor_state.key_event_buffer->event_buffer[pos].keycode);
            } else {
                platform_unregister_keycode(pipeline_executor_state.key_event_buffer->event_buffer[pos].keycode);
            }
        }
    }

    bool key_digested = last_execution.key_buffer_changed || last_execution.capture_key_events;
    return (key_digested == false);
}

void pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(platform_time_t callback_time) {
    pipeline_executor_state.return_data.callback_time = callback_time;
    pipeline_executor_state.return_data.capture_key_events = true;
}

void pipeline_executor_end_with_capture_next_keys(void) {
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = true;
}

void pipeline_executor_end_with_buffer_swap(void) {
    pipeline_executor_state.return_data.capture_key_events = true;
}

static void pipeline_executor_create_state(void) {
    pipeline_executor_state.key_event_buffer = platform_key_event_create();
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.return_data.key_buffer_changed = false;
    pipeline_executor_state.return_data.key_buffer = pipeline_executor_state.key_event_buffer;
    pipeline_executor_state.pipeline_index = 0; // Initialize the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Initialize the deferred execution callback token
}

/**
 * Resets the global state of the pipeline executor, including all buffers, pipeline states,
 * and configuration.
 * This function should be called to reinitialize the pipeline executor to a clean state.
 */
void pipeline_executor_reset_state(void) {
    platform_key_event_reset(pipeline_executor_state.key_event_buffer);
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.return_data.key_buffer_changed = false;
    pipeline_executor_state.return_data.key_buffer = pipeline_executor_state.key_event_buffer;
    pipeline_executor_state.pipeline_index = 0; // Reset the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Reset the deferred execution callback token
    for (uint8_t i = 0; i < pipeline_executor_config->length; i++) {
        pipeline_t* pipeline = pipeline_executor_config->pipelines[i];
        if (pipeline) {
            pipeline->callback_reset(pipeline->data);
        }
    }
    if (pipeline_executor_state.return_data.callback_time > 0) {
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }
}

void pipeline_executor_create_config(uint8_t pipeline_count) {
    pipeline_executor_create_state();
    pipeline_executor_config = (pipeline_executor_config_t*)malloc(sizeof(pipeline_executor_config_t) + sizeof(pipeline_t*) * pipeline_count);
    pipeline_executor_config->length = pipeline_count; // Initialize the length of the pipeline config
}

void pipeline_executor_add_pipeline(uint8_t pipeline_position, pipeline_callback callback, pipeline_callback_reset callback_reset, void* user_data) {
    if (pipeline_position >= pipeline_executor_config->length) {
        // Handle error: pipeline position out of bounds
        return;
    }
    pipeline_t* pipeline;
    pipeline = malloc(sizeof(pipeline_t));
    pipeline->callback = callback;
    pipeline->callback_reset = callback_reset;
    pipeline->data = user_data;

    pipeline_executor_config->pipelines[pipeline_position] = pipeline;
}




bool pipeline_process_key(abskeyevent_t abskeyevent) {
    #ifdef DEBUG
    // Print the abskeyevent for debugging
    printf("Processing key event: Row: %u, Col: %u, Pressed: %d, Time: %u\n",
            abskeyevent.keypos.row, abskeyevent.keypos.col, abskeyevent.pressed, abskeyevent.time);
    #endif

    bool further_process_required = false;

    bool event_added = false;
    if (abskeyevent.pressed) {
        uint8_t press_id = platform_key_event_add_physical_press(pipeline_executor_state.key_event_buffer, abskeyevent.time, abskeyevent.keypos);
        if (press_id > 0) {
            event_added = true;
        }
    } else {
        if (platform_key_event_add_physical_release(pipeline_executor_state.key_event_buffer, abskeyevent.time, abskeyevent.keypos)) {
            event_added = true;
        }
    }
    #ifdef DEBUG
    print_key_press_buffer(pipeline_executor_state.key_event_buffer->key_press_buffer, PLATFORM_KEY_BUFFER_MAX_ELEMENTS);
    print_key_event_buffer(pipeline_executor_state.key_event_buffer, PLATFORM_KEY_EVENT_MAX_ELEMENTS);
    #endif
    if (event_added) {
        further_process_required = process_key_pool();
    } else {
        #ifdef DEBUG
        printf("Error: Key event buffer is full, cannot add event\n");
        #endif
        // Reset the global state
        pipeline_executor_reset_state();
        further_process_required = false;
    }
    return further_process_required;
}
