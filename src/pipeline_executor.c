#include "pipeline_executor.h"
#include <stdint.h>
#include "key_virtual_buffer.h"
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include "key_event_buffer.h"
#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"

#if defined(DEBUG)
    #define DEBUG_BUFFERS(caption) \
        DEBUG_PRINT("%s", caption); \
        print_key_press_buffer(pipeline_executor_state.key_event_buffer->key_press_buffer); \
        print_key_event_buffer(pipeline_executor_state.key_event_buffer);
#else
    #define DEBUG_BUFFERS(caption) ((void)0)
#endif

#if defined(DEBUG)
    #define DEBUG_RETURN_DATA() \
        DEBUG_PRINT("Return Data:"); \
        DEBUG_PRINT("| Capture: %s, Time: %hu, Buffer Changed: %s", pipeline_executor_state.return_data.capture_key_events ? "true" : "false", pipeline_executor_state.return_data.callback_time, pipeline_executor_state.return_data.key_buffer_changed ? "true" : "false");
#else
    #define DEBUG_RETURN_DATA() ((void)0)
#endif

pipeline_executor_state_t pipeline_executor_state;
pipeline_executor_config_t *pipeline_executor_config;
pipeline_physical_actions_t physical_actions;
pipeline_virtual_actions_t virtual_actions;

static void register_key(platform_keycode_t keycode) {
    platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, keycode);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}

static void unregister_key(platform_keycode_t keycode) {
    platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, keycode);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}

static void tap_key(platform_keycode_t keycode) {
    platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, keycode);
    platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, keycode);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}

static void remove_physical_press(uint8_t press_id) {
    platform_key_event_remove_physical_press_by_press_id(pipeline_executor_state.key_event_buffer, press_id);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}

static void remove_physical_release(uint8_t press_id) {
    platform_key_event_remove_physical_release_by_press_id(pipeline_executor_state.key_event_buffer, press_id);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}

static void remove_physical_tap(uint8_t press_id) {
    platform_key_event_remove_physical_tap_by_press_id(pipeline_executor_state.key_event_buffer, press_id);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}

static void update_layer_for_physical_events(uint8_t layer, uint8_t pos) {
    platform_key_event_update_layer_for_physical_events(pipeline_executor_state.key_event_buffer, layer, pos);
    pipeline_executor_state.return_data.key_buffer_changed = true;
}



static void reset_return_data(capture_pipeline_t* return_data) {
    return_data->callback_time = 0;
    return_data->capture_key_events = false;
    return_data->key_buffer_changed = false;
}

static void physical_event_triggered(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, platform_key_event_t* key_event) {
    reset_return_data(&pipeline_executor_state->return_data);

    pipeline_physical_callback_params_t callback_params;
    callback_params.key_event = key_event;
    callback_params.callback_type = PIPELINE_CALLBACK_KEY_EVENT;
    DEBUG_EXECUTOR("Executing pipeline %hhu with key event", pipeline_index);

    pipeline_executor_config->physical_pipelines[pipeline_index]->callback(&callback_params, &physical_actions, pipeline_executor_config->physical_pipelines[pipeline_index]->data);
}

static void physical_event_triggered_with_timer(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, uint16_t callback_time) {
    reset_return_data(&pipeline_executor_state->return_data);

    pipeline_physical_callback_params_t callback_params;
    callback_params.callback_type = PIPELINE_CALLBACK_TIMER;
    callback_params.callback_time = callback_time;
    DEBUG_EXECUTOR("Executing pipeline %hhu with callback time %hu", pipeline_index, callback_time);

    pipeline_executor_config->physical_pipelines[pipeline_index]->callback(&callback_params, &physical_actions, pipeline_executor_config->physical_pipelines[pipeline_index]->data);
}

static void virtual_event_triggered(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, platform_virtual_event_buffer_t* press_buffer_selected) {
    reset_return_data(&pipeline_executor_state->return_data);

    pipeline_virtual_callback_params_t callback_params;
    callback_params.key_events = press_buffer_selected;
    DEBUG_EXECUTOR("Executing virtual pipeline %hhu with key events", pipeline_index);

    pipeline_executor_config->virtual_pipelines[pipeline_index]->callback(&callback_params, &virtual_actions, pipeline_executor_config->virtual_pipelines[pipeline_index]->data);
}

static void flush_virtual_event_buffer(void) {
    for (size_t pos = 0; pos < pipeline_executor_state.virtual_event_buffer->press_buffer_pos; pos++) {
        if (pipeline_executor_state.virtual_event_buffer->press_buffer[pos].is_press) {
            DEBUG_EXECUTOR("Registering virtual keycode %d", pipeline_executor_state.virtual_event_buffer->press_buffer[pos].keycode);
            platform_register_keycode(pipeline_executor_state.virtual_event_buffer->press_buffer[pos].keycode);
        } else {
            DEBUG_EXECUTOR("Unregistering virtual keycode %d", pipeline_executor_state.virtual_event_buffer->press_buffer[pos].keycode);
            platform_unregister_keycode(pipeline_executor_state.virtual_event_buffer->press_buffer[pos].keycode);
        }
    }

    platform_virtual_event_reset(pipeline_executor_state.virtual_event_buffer);
}

// Executes the middleware when the timer callback is triggered
static void physical_event_deferred_exec_callback(void *cb_arg) {
    (void)cb_arg; // Unused parameter

    DEBUG_PRINT("=== TIMER ===");

    capture_pipeline_t last_execution = pipeline_executor_state.return_data;
    platform_time_t callback_time = last_execution.callback_time;

    uint8_t pipeline_index = pipeline_executor_state.physical_pipeline_index;

    bool key_digested = false;

    physical_event_triggered_with_timer(&pipeline_executor_state, pipeline_index, callback_time);
    last_execution = pipeline_executor_state.return_data;
    if (last_execution.key_buffer_changed == true || last_execution.capture_key_events == true) {
        key_digested = true;
    }

    bool pipeline_capturing_key_events = last_execution.capture_key_events;

    // Move the physical keys to the virtual event buffer
    if (pipeline_capturing_key_events == false && pipeline_executor_state.key_event_buffer->event_buffer_pos > 0) {
        for (size_t i = 0; i < pipeline_executor_state.key_event_buffer->event_buffer_pos; i++) {
            platform_key_event_t* event = &pipeline_executor_state.key_event_buffer->event_buffer[i];
            if (event->is_press) {
                platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, event->keycode);
            } else {
                platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, event->keycode);
            }
        }
        platform_key_event_remove_event_keys(pipeline_executor_state.key_event_buffer);
    }

    if (pipeline_executor_state.return_data.callback_time > 0) {
        DEBUG_EXECUTOR("Scheduling deferred execution callback for time %hu", pipeline_executor_state.return_data.callback_time);
        pipeline_executor_state.deferred_exec_callback_token = platform_defer_exec(pipeline_executor_state.return_data.callback_time, physical_event_deferred_exec_callback, NULL);
    }

    // Process the virtual pipelines
    for (size_t i = 0; i < pipeline_executor_config->virtual_pipelines_length; i++) {
        virtual_event_triggered(&pipeline_executor_state, i, pipeline_executor_state.virtual_event_buffer);
        last_execution = pipeline_executor_state.return_data;
        if (last_execution.key_buffer_changed == true) {
            key_digested = true;
        }
    }

    // Flush the virtual event buffer if no key events are captured and the buffer has changed
    if (pipeline_capturing_key_events == false && key_digested == true) {
        flush_virtual_event_buffer();
    }

    DEBUG_BUFFERS("Key event buffer after time out:");
    DEBUG_RETURN_DATA();
    DEBUG_PRINT("=================");
    DEBUG_PRINT_NL();
}

// Execute the middleware when a key event occurs
// Returns false if any key was digested by the middleware
// This function is called by the platform when a key event occurs
// It processes the key events and executes the pipelines accordingly
// It also cancels any pending deferred execution callbacks
// and resets the key event buffer for the next iteration
static bool process_key_pool(void) {
    if (pipeline_executor_state.return_data.callback_time > 0) {
        DEBUG_EXECUTOR("Cancelling deferred execution callback");
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }

    platform_key_event_t* key_event = &pipeline_executor_state.key_event_buffer->event_buffer[pipeline_executor_state.key_event_buffer->event_buffer_pos - 1];

    capture_pipeline_t last_execution = pipeline_executor_state.return_data;
    size_t pipeline_index = 0;
    if (last_execution.capture_key_events == true) {
        pipeline_index = pipeline_executor_state.physical_pipeline_index;
    }


    bool key_digested = false;

    // If the previous execution captured key events, we need to process them first
    if (last_execution.capture_key_events == true) {
        physical_event_triggered(&pipeline_executor_state, pipeline_index, key_event);
        last_execution = pipeline_executor_state.return_data;
        if (last_execution.key_buffer_changed == true || last_execution.capture_key_events == true) {
            key_digested = true;
        }
        pipeline_index = pipeline_executor_state.physical_pipeline_index + 1; // Move to the next pipeline
    }
    // Process the physical pipelines
    if (last_execution.capture_key_events == false)
    {
        for (size_t i = pipeline_index; i < pipeline_executor_config->physical_pipelines_length; i++) {
            pipeline_executor_state.physical_pipeline_index = i;
            for (size_t j = 0; j < pipeline_executor_state.key_event_buffer->event_buffer_pos; j++) {
                key_event = &pipeline_executor_state.key_event_buffer->event_buffer[j];
                physical_event_triggered(&pipeline_executor_state, i, key_event);
                last_execution = pipeline_executor_state.return_data;
                if (last_execution.key_buffer_changed == true || last_execution.capture_key_events == true) {
                    key_digested = true;
                }
                if (last_execution.capture_key_events == true) break;
            }
            if (last_execution.capture_key_events == true) break;
        }
    }

    bool pipeline_capturing_key_events = last_execution.capture_key_events;

    // Move the physical keys to the virtual event buffer
    if (pipeline_capturing_key_events == false && pipeline_executor_state.key_event_buffer->event_buffer_pos > 0) {
        for (size_t i = 0; i < pipeline_executor_state.key_event_buffer->event_buffer_pos; i++) {
            platform_key_event_t* event = &pipeline_executor_state.key_event_buffer->event_buffer[i];
            if (event->is_press) {
                platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, event->keycode);
            } else {
                platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, event->keycode);
            }
        }
        platform_key_event_remove_event_keys(pipeline_executor_state.key_event_buffer);
    }

    if (pipeline_executor_state.return_data.callback_time > 0) {
        DEBUG_EXECUTOR("Scheduling deferred execution callback for time %hu", pipeline_executor_state.return_data.callback_time);
        pipeline_executor_state.deferred_exec_callback_token = platform_defer_exec(pipeline_executor_state.return_data.callback_time, physical_event_deferred_exec_callback, NULL);
    }

    // Process the virtual pipelines
    for (size_t i = 0; i < pipeline_executor_config->virtual_pipelines_length; i++) {
        virtual_event_triggered(&pipeline_executor_state, i, pipeline_executor_state.virtual_event_buffer);
        last_execution = pipeline_executor_state.return_data;
        if (last_execution.key_buffer_changed == true) {
            key_digested = true;
        }
    }

    // Flush the virtual event buffer if no key events are captured and the buffer has changed
    if (pipeline_capturing_key_events == false && key_digested == true) {
        flush_virtual_event_buffer();
    }

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

static void pipeline_executor_create_state(void) {
    pipeline_executor_state.key_event_buffer = platform_key_event_create();
    pipeline_executor_state.virtual_event_buffer = platform_virtual_event_create();
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.return_data.key_buffer_changed = false;
    pipeline_executor_state.physical_pipeline_index = 0; // Initialize the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Initialize the deferred execution callback token
}

/**
 * Resets the global state of the pipeline executor, including all buffers, pipeline states,
 * and configuration.
 * This function should be called to reinitialize the pipeline executor to a clean state.
 */
void pipeline_executor_reset_state(void) {
    platform_key_event_reset(pipeline_executor_state.key_event_buffer);
    platform_virtual_event_reset(pipeline_executor_state.virtual_event_buffer);
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.return_data.key_buffer_changed = false;
    pipeline_executor_state.physical_pipeline_index = 0; // Reset the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Reset the deferred execution callback token
    for (uint8_t i = 0; i < pipeline_executor_config->physical_pipelines_length; i++) {
        physical_pipeline_t* pipeline = pipeline_executor_config->physical_pipelines[i];
        if (pipeline) {
            pipeline->callback_reset(pipeline->data);
        }
    }
    if (pipeline_executor_state.return_data.callback_time > 0) {
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }
}

void pipeline_executor_create_config(uint8_t physical_pipeline_count, uint8_t virtual_pipeline_count) {
    DEBUG_PRINT_NL();
    pipeline_executor_create_state();
    pipeline_executor_config = malloc(sizeof(pipeline_executor_config_t));
    pipeline_executor_config->physical_pipelines_length = physical_pipeline_count;
    pipeline_executor_config->virtual_pipelines_length = virtual_pipeline_count;
    pipeline_executor_config->physical_pipelines = malloc(sizeof(physical_pipeline_t*) * physical_pipeline_count);
    pipeline_executor_config->virtual_pipelines = malloc(sizeof(virtual_pipeline_t*) * virtual_pipeline_count);

    physical_actions.register_key_fn = &register_key;
    physical_actions.unregister_key_fn = &unregister_key;
    physical_actions.tap_key_fn = &tap_key;
    physical_actions.remove_physical_press_fn = &remove_physical_press;
    physical_actions.remove_physical_release_fn = &remove_physical_release;
    physical_actions.remove_physical_tap_fn = &remove_physical_tap;
    physical_actions.update_layer_for_physical_events_fn = &update_layer_for_physical_events;

    virtual_actions.register_key_fn = &register_key;
    virtual_actions.unregister_key_fn = &unregister_key;
    virtual_actions.tap_key_fn = &tap_key;
}

void pipeline_executor_add_physical_pipeline(uint8_t pipeline_position, pipeline_physical_callback callback, pipeline_callback_reset callback_reset, void* user_data) {
    if (pipeline_position >= pipeline_executor_config->physical_pipelines_length) {
        // Handle error: pipeline position out of bounds
        return;
    }
    physical_pipeline_t* pipeline;
    pipeline = malloc(sizeof(physical_pipeline_t));
    pipeline->callback = callback;
    pipeline->callback_reset = callback_reset;
    pipeline->data = user_data;

    pipeline_executor_config->physical_pipelines[pipeline_position] = pipeline;
}

void pipeline_executor_add_virtual_pipeline(uint8_t pipeline_position, pipeline_virtual_callback callback, pipeline_callback_reset callback_reset, void* user_data) {
    if (pipeline_position >= pipeline_executor_config->virtual_pipelines_length) {
        // Handle error: pipeline position out of bounds
        return;
    }
    virtual_pipeline_t* pipeline;
    pipeline = malloc(sizeof(virtual_pipeline_t));
    pipeline->callback = callback;
    pipeline->callback_reset = callback_reset;
    pipeline->data = user_data;

    pipeline_executor_config->virtual_pipelines[pipeline_position] = pipeline;
}

bool pipeline_process_key(abskeyevent_t abskeyevent) {
    DEBUG_PRINT("=== ITERATION ===");

    bool further_process_required = false;

    bool buffer_full = false;
    bool event_added = false;
    if (abskeyevent.pressed) {
        uint8_t press_id = platform_key_event_add_physical_press(pipeline_executor_state.key_event_buffer, abskeyevent.time, abskeyevent.keypos, &buffer_full);
        if (press_id > 0) {
            event_added = true;
        }
        DEBUG_BUFFERS(event_added ? "Key event buffer after adding press key:" : "Key event buffer not modified after trying to add press key:");
    } else {
        if (platform_key_event_add_physical_release(pipeline_executor_state.key_event_buffer, abskeyevent.time, abskeyevent.keypos, &buffer_full)) {
            event_added = true;
        }
        DEBUG_BUFFERS(event_added ? "Key event buffer after adding release key:" : "Key event buffer not modified after trying to add release key:");
    }

    if (buffer_full == false && event_added) {
        further_process_required = process_key_pool();
    } else if (buffer_full) {
        DEBUG_EXECUTOR("Error: Key event buffer is full, cannot add event");
        // Reset the global state
        pipeline_executor_reset_state();
        further_process_required = false;
        return further_process_required;
    }
    DEBUG_BUFFERS(event_added ? "Key event buffer after processing:" : "Key event buffer not modified:");
    DEBUG_RETURN_DATA();
    DEBUG_PRINT("=================");
    DEBUG_PRINT_NL();
    return further_process_required;
}
