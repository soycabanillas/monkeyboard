#include "monkeyboard_debug.h"
#include "pipeline_executor.h"
#include <stdint.h>
#include "key_virtual_buffer.h"
#include <stdlib.h>
#include "key_event_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"
#include "monkeyboard_layer_manager.h"

#if defined(MONKEYBOARD_DEBUG)
    #include "key_press_buffer.h"
    #define DEBUG_BUFFERS(caption) \
        DEBUG_PRINT("%s", caption); \
        print_key_press_buffer(pipeline_executor_state.key_event_buffer->key_press_buffer); \
        print_key_event_buffer(pipeline_executor_state.key_event_buffer);
#else
    #define DEBUG_BUFFERS(caption) ((void)0)
#endif

#if defined(MONKEYBOARD_DEBUG)
static const char* timer_behavior_to_string(pipeline_executor_timer_behavior_t timer_behavior) {
    switch (timer_behavior) {
        case PIPELINE_EXECUTOR_TIMEOUT_NEW: return "NEW";
        case PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS: return "PREVIOUS";
        case PIPELINE_EXECUTOR_TIMEOUT_NONE: return "NONE";
        default: return "UNKNOWN";
    }
}
    #define DEBUG_RETURN_DATA() \
        DEBUG_PRINT("Return Data:"); \
        DEBUG_PRINT("| Capture: %s, Behavior: %s, Time: %u", pipeline_executor_state.return_data.capture_key_events ? "true" : "false", timer_behavior_to_string(pipeline_executor_state.return_data.timer_behavior), pipeline_executor_state.return_data.callback_time);
#else
    #define DEBUG_RETURN_DATA() ((void)0)
#endif

pipeline_executor_state_t pipeline_executor_state;
pipeline_executor_config_t *pipeline_executor_config;
pipeline_physical_actions_t physical_actions;
pipeline_virtual_actions_t virtual_actions;
pipeline_physical_return_actions_t physical_return_actions;

static void register_virtual_key(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, keycode);
}

static void unregister_virtual_key(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, keycode);
}

static void tap_virtual_key(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, keycode);
    platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, keycode);
}

static void register_key(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_register_keycode(keycode);
}
static void unregister_key(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_unregister_keycode(keycode);
}

static void tap_key(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_register_keycode(keycode);
    platform_unregister_keycode(keycode);
}

static void report_press(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_add_key(keycode);
}
static void report_release(platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_del_key(keycode);
}
static void report_send(void) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    platform_send_report();
}

static uint8_t get_physical_key_event_count(void) {
    return pipeline_executor_state.event_length;
}

static platform_key_event_t* get_physical_key_event(uint8_t index) {
    if (index < pipeline_executor_state.event_length) {
        return &pipeline_executor_state.key_event_buffer->event_buffer[index];
    }
    return NULL; // Out of bounds
}

static uint8_t get_virtual_key_event_count(void) {
    return pipeline_executor_state.event_length;
}

static platform_virtual_buffer_virtual_event_t* get_virtual_key_event(uint8_t index) {
    if (index < pipeline_executor_state.virtual_event_buffer->press_buffer_pos) {
        return &pipeline_executor_state.virtual_event_buffer->press_buffer[index];
    }
    return NULL; // Out of bounds
}

static void remove_physical_press(uint8_t press_id) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    uint8_t current_length = pipeline_executor_state.key_event_buffer->event_buffer_pos;
    platform_key_event_remove_physical_press_by_press_id(pipeline_executor_state.key_event_buffer, press_id);
    uint8_t new_length = pipeline_executor_state.key_event_buffer->event_buffer_pos;
    if (new_length < current_length) {
        pipeline_executor_state.event_length = pipeline_executor_state.event_length - (current_length - new_length);
    }
}

static void remove_physical_release(uint8_t press_id) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    uint8_t current_length = pipeline_executor_state.key_event_buffer->event_buffer_pos;
    platform_key_event_remove_physical_release_by_press_id(pipeline_executor_state.key_event_buffer, press_id);
    uint8_t new_length = pipeline_executor_state.key_event_buffer->event_buffer_pos;
    if (new_length < current_length) {
        pipeline_executor_state.event_length = pipeline_executor_state.event_length - (current_length - new_length);
    }
}

static void remove_physical_tap(uint8_t press_id) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    uint8_t current_length = pipeline_executor_state.key_event_buffer->event_buffer_pos;
    platform_key_event_remove_physical_tap_by_press_id(pipeline_executor_state.key_event_buffer, press_id);
    uint8_t new_length = pipeline_executor_state.key_event_buffer->event_buffer_pos;
    if (new_length < current_length) {
        pipeline_executor_state.event_length = pipeline_executor_state.event_length - (current_length - new_length);
    }
}

static void change_key_code(uint8_t pos, platform_keycode_t keycode) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
    uint8_t total_events = get_physical_key_event_count();
    if (pos < total_events) {
        platform_key_event_t* event = get_physical_key_event(pos);
        if (event != NULL) {
            platform_key_event_change_keycode(pipeline_executor_state.key_event_buffer, event->press_id, keycode);
        }
    } else {
        DEBUG_PRINT_ERROR("Position out of bounds: %d", pos);
    }
}

static void mark_as_processed(void) {
    pipeline_executor_state.return_data.processed = true; // Mark the key event as processed
}

static void end_with_capture_next_keys(pipeline_executor_timer_behavior_t timer_behavior, platform_time_t time) {
    pipeline_executor_state.return_data.timer_behavior = timer_behavior;
    switch (timer_behavior) {
        case PIPELINE_EXECUTOR_TIMEOUT_NEW:
            pipeline_executor_state.return_data.callback_time = time;
            break;
        case PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS:
            pipeline_executor_state.return_data.callback_time = pipeline_executor_state.return_data.callback_time;
            break;
        case PIPELINE_EXECUTOR_TIMEOUT_NONE:
            pipeline_executor_state.return_data.callback_time = 0;
            break;
    }
    pipeline_executor_state.return_data.callback_time = time;
    pipeline_executor_state.return_data.capture_key_events = true;
}

static void no_capture(void) {
    pipeline_executor_state.return_data.timer_behavior = PIPELINE_EXECUTOR_TIMEOUT_NONE;
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
}

static void reset_return_data(capture_pipeline_t* return_data) {
    return_data->processed = false; // Reset processed state
    return_data->timer_behavior = PIPELINE_EXECUTOR_TIMEOUT_NONE;
    return_data->callback_time = 0;
    return_data->capture_key_events = false;
}

static void physical_event_triggered(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, platform_key_event_t* key_event, bool is_capturing_keys) {
    reset_return_data(&pipeline_executor_state->return_data);

    pipeline_physical_callback_params_t callback_params;
    callback_params.callback_type = PIPELINE_CALLBACK_KEY_EVENT;
    callback_params.key_event = key_event;
    callback_params.is_capturing_keys = is_capturing_keys;
    DEBUG_EXECUTOR("Executing pipeline %hhu with key event", pipeline_index);

    pipeline_executor_config->physical_pipelines[pipeline_index]->callback(&callback_params, &physical_actions, &physical_return_actions, pipeline_executor_config->physical_pipelines[pipeline_index]->data);
}

static void physical_event_triggered_with_timer(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, bool is_capturing_keys) {
    reset_return_data(&pipeline_executor_state->return_data);

    pipeline_physical_callback_params_t callback_params;
    callback_params.callback_type = PIPELINE_CALLBACK_TIMER;
    callback_params.is_capturing_keys = is_capturing_keys;
    DEBUG_EXECUTOR("Executing pipeline %hhu", pipeline_index);

    pipeline_executor_config->physical_pipelines[pipeline_index]->callback(&callback_params, &physical_actions, &physical_return_actions, pipeline_executor_config->physical_pipelines[pipeline_index]->data);
}

static void virtual_event_triggered(pipeline_executor_state_t* pipeline_executor_state, uint8_t pipeline_index, platform_virtual_buffer_virtual_event_t* key_event) {
    reset_return_data(&pipeline_executor_state->return_data);

    pipeline_virtual_callback_params_t callback_params;
    callback_params.key_event = key_event;
    DEBUG_EXECUTOR("Executing virtual pipeline %hhu with key events", pipeline_index);

    pipeline_executor_config->virtual_pipelines[pipeline_index]->callback(&callback_params, &virtual_actions, pipeline_executor_config->virtual_pipelines[pipeline_index]->data);
}

static void process_virtual_event_buffer(void) {
    capture_pipeline_t last_execution;

    // Process the virtual pipelines
    for (size_t pos = 0; pos < pipeline_executor_state.virtual_event_buffer->press_buffer_pos; pos++) {
        bool processed = false;
        for (size_t i = 0; i < pipeline_executor_config->virtual_pipelines_length; i++) {
            virtual_event_triggered(&pipeline_executor_state, i, &pipeline_executor_state.virtual_event_buffer->press_buffer[pos]);
            last_execution = pipeline_executor_state.return_data;
            processed = last_execution.processed;
            if (processed == true) break;
        }
        if (processed == false) {
            if (pipeline_executor_state.virtual_event_buffer->press_buffer[pos].is_press) {
                platform_register_keycode(pipeline_executor_state.virtual_event_buffer->press_buffer[pos].keycode);
            } else {
                platform_unregister_keycode(pipeline_executor_state.virtual_event_buffer->press_buffer[pos].keycode);
            }
        }
    }

    platform_virtual_event_reset(pipeline_executor_state.virtual_event_buffer);
}

// Executes the middleware when the timer callback is triggered
static void physical_event_deferred_exec_callback(void *cb_arg) {
    (void)cb_arg; // Unused parameter

    DEBUG_PRINT("=== TIMER ===");

    capture_pipeline_t last_execution = pipeline_executor_state.return_data;

    uint8_t pipeline_index = pipeline_executor_state.physical_pipeline_index;

    physical_event_triggered_with_timer(&pipeline_executor_state, pipeline_index, last_execution.capture_key_events);
    last_execution = pipeline_executor_state.return_data;

    bool pipeline_capturing_key_events = last_execution.capture_key_events;

    // Move the physical keys to the virtual event buffer
    if (pipeline_capturing_key_events == false && pipeline_executor_state.event_length > 0) {
        for (size_t i = 0; i < pipeline_executor_state.event_length; i++) {
            platform_key_event_t* event = &pipeline_executor_state.key_event_buffer->event_buffer[i];
            if (event->is_press) {
                platform_virtual_event_add_press(pipeline_executor_state.virtual_event_buffer, event->keycode);
            } else {
                platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, event->keycode);
            }
        }
        platform_key_event_remove_event_keys(pipeline_executor_state.key_event_buffer);
    }

    last_execution = pipeline_executor_state.return_data;

    if (last_execution.timer_behavior == PIPELINE_EXECUTOR_TIMEOUT_NEW && last_execution.callback_time > 0) {
        DEBUG_EXECUTOR("Scheduling deferred execution callback for time %u", last_execution.callback_time);
        pipeline_executor_state.deferred_exec_callback_token = platform_defer_exec(last_execution.callback_time, physical_event_deferred_exec_callback, NULL);
        pipeline_executor_state.is_callback_set = true; // Set the callback set flag
    }

    // Process the virtual pipelines
    process_virtual_event_buffer();

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
static void process_key_pool(void) {

    capture_pipeline_t last_execution = pipeline_executor_state.return_data;
    platform_key_event_t* key_event = &pipeline_executor_state.key_event_buffer->event_buffer[pipeline_executor_state.key_event_buffer->event_buffer_pos - 1];

    DEBUG_EXECUTOR("Capture key events %d", last_execution.capture_key_events);

    if (last_execution.capture_key_events == true && key_event->is_press == false) {
        uint8_t event_size = pipeline_executor_state.key_event_buffer->event_buffer_pos;
        bool found_previous_press = false;
        for (uint8_t i = 0; i < event_size - 1; i++) {
            platform_key_event_t* event = &pipeline_executor_state.key_event_buffer->event_buffer[i];
            if (event->is_press && event->press_id == key_event->press_id) {
                found_previous_press = true;
                break;
            }
        }
        if (found_previous_press == false){
            DEBUG_EXECUTOR("Skipping release for press_id %d", key_event->press_id);
            platform_virtual_event_add_release(pipeline_executor_state.virtual_event_buffer, key_event->keycode);
            platform_key_event_remove_physical_release_by_press_id(pipeline_executor_state.key_event_buffer, key_event->press_id);
            pipeline_executor_state.event_length--;
        }
    }

    size_t pipeline_index = 0;
    if (last_execution.capture_key_events == true) {
        pipeline_index = pipeline_executor_state.physical_pipeline_index;
    }

    // If the previous execution captured key events, we need to process them first
    if (last_execution.capture_key_events == true) {
        pipeline_executor_state.event_length = pipeline_executor_state.key_event_buffer->event_buffer_pos; // Set the event length to the current buffer size
        physical_event_triggered(&pipeline_executor_state, pipeline_index, key_event, true);
        last_execution = pipeline_executor_state.return_data;
        pipeline_index = pipeline_executor_state.physical_pipeline_index + 1; // Move to the next pipeline
    }
    // Process the physical pipelines
    if (last_execution.capture_key_events == false)
    {
        for (size_t i = pipeline_index; i < pipeline_executor_config->physical_pipelines_length; i++) {
            pipeline_executor_state.physical_pipeline_index = i;
            for (size_t j = 0; j < pipeline_executor_state.key_event_buffer->event_buffer_pos; j++) {
                key_event = &pipeline_executor_state.key_event_buffer->event_buffer[j];
                pipeline_executor_state.event_length = j + 1;
                physical_event_triggered(&pipeline_executor_state, i, key_event, false);
                last_execution = pipeline_executor_state.return_data;
                if (last_execution.capture_key_events == true && last_execution.callback_time > 0 && pipeline_executor_state.event_length < pipeline_executor_state.key_event_buffer->event_buffer_pos) {
                    platform_time_t time_span = pipeline_executor_state.key_event_buffer->event_buffer[pipeline_executor_state.key_event_buffer->event_buffer_pos].time - pipeline_executor_state.key_event_buffer->event_buffer[pipeline_executor_state.key_event_buffer->event_buffer_pos - 1].time;
                    if (time_span >= last_execution.callback_time) {
                        physical_event_triggered_with_timer(&pipeline_executor_state, pipeline_index, last_execution.capture_key_events);
                        last_execution = pipeline_executor_state.return_data;
                    }
                }
                if (last_execution.capture_key_events == true && pipeline_executor_state.event_length == pipeline_executor_state.key_event_buffer->event_buffer_pos) break;
            }
            if (last_execution.capture_key_events == true && pipeline_executor_state.event_length == pipeline_executor_state.key_event_buffer->event_buffer_pos) break;
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

    if (last_execution.timer_behavior == PIPELINE_EXECUTOR_TIMEOUT_NONE || last_execution.timer_behavior == PIPELINE_EXECUTOR_TIMEOUT_NEW) {
        if (pipeline_executor_state.is_callback_set) {
            DEBUG_EXECUTOR("Cancelling deferred execution callback");
            platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
            pipeline_executor_state.is_callback_set = false; // Reset the callback set flag
        }
    }
    if (last_execution.timer_behavior == PIPELINE_EXECUTOR_TIMEOUT_NEW && last_execution.callback_time > 0) {
        DEBUG_EXECUTOR("Scheduling deferred execution callback for time %u", pipeline_executor_state.return_data.callback_time);
        pipeline_executor_state.deferred_exec_callback_token = platform_defer_exec(pipeline_executor_state.return_data.callback_time, physical_event_deferred_exec_callback, NULL);
        pipeline_executor_state.is_callback_set = true; // Set the callback set flag
    }

    // Process the virtual pipelines
    process_virtual_event_buffer();
}

static void pipeline_executor_create_state(void) {
    pipeline_executor_state.key_event_buffer = platform_key_event_create();
    pipeline_executor_state.virtual_event_buffer = platform_virtual_event_create();
    pipeline_executor_state.return_data.processed = false;
    pipeline_executor_state.return_data.timer_behavior = PIPELINE_EXECUTOR_TIMEOUT_NONE;
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.physical_pipeline_index = 0; // Initialize the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Initialize the deferred execution callback token
    pipeline_executor_state.is_callback_set = false; // Initialize the callback set flag
}

/**
 * Resets the global state of the pipeline executor, including all buffers, pipeline states,
 * and configuration.
 * This function should be called to reinitialize the pipeline executor to a clean state.
 */
void pipeline_executor_reset_state(void) {
    platform_key_event_reset(pipeline_executor_state.key_event_buffer);
    platform_virtual_event_reset(pipeline_executor_state.virtual_event_buffer);
    pipeline_executor_state.return_data.processed = false;
    pipeline_executor_state.return_data.timer_behavior = PIPELINE_EXECUTOR_TIMEOUT_NONE;
    pipeline_executor_state.return_data.callback_time = 0;
    pipeline_executor_state.return_data.capture_key_events = false;
    pipeline_executor_state.physical_pipeline_index = 0; // Reset the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Reset the deferred execution callback token
    for (uint8_t i = 0; i < pipeline_executor_config->physical_pipelines_length; i++) {
        physical_pipeline_t* pipeline = pipeline_executor_config->physical_pipelines[i];
        if (pipeline) {
            pipeline->callback_reset(pipeline->data);
        }
    }
    if (pipeline_executor_state.is_callback_set > 0) {
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }
    pipeline_executor_state.is_callback_set = false; // Reset the callback set flag

}

void pipeline_executor_create_config(uint8_t physical_pipeline_count, uint8_t virtual_pipeline_count) {
    DEBUG_PRINT_NL();
    pipeline_executor_create_state();
    pipeline_executor_config = malloc(sizeof(pipeline_executor_config_t));
    pipeline_executor_config->physical_pipelines_length = physical_pipeline_count;
    pipeline_executor_config->virtual_pipelines_length = virtual_pipeline_count;
    pipeline_executor_config->physical_pipelines = malloc(sizeof(physical_pipeline_t*) * physical_pipeline_count);
    pipeline_executor_config->virtual_pipelines = malloc(sizeof(virtual_pipeline_t*) * virtual_pipeline_count);

    physical_actions.register_key_fn = &register_virtual_key;
    physical_actions.unregister_key_fn = &unregister_virtual_key;
    physical_actions.tap_key_fn = &tap_virtual_key;
    physical_actions.get_physical_key_event_count_fn = &get_physical_key_event_count;
    physical_actions.get_physical_key_event_fn = &get_physical_key_event;
    physical_actions.remove_physical_press_fn = &remove_physical_press;
    physical_actions.remove_physical_release_fn = &remove_physical_release;
    physical_actions.remove_physical_tap_fn = &remove_physical_tap;
    physical_actions.change_key_code_fn = &change_key_code;
    physical_actions.mark_as_processed_fn = &mark_as_processed;

    virtual_actions.register_key_fn = &register_key;
    virtual_actions.unregister_key_fn = &unregister_key;
    virtual_actions.tap_key_fn = &tap_key;
    virtual_actions.report_press_fn = &report_press;
    virtual_actions.report_release_fn = &report_release;
    virtual_actions.report_send_fn = &report_send;
    virtual_actions.get_virtual_key_event_count_fn = &get_virtual_key_event_count;
    virtual_actions.get_virtual_key_event_fn = &get_virtual_key_event;
    virtual_actions.mark_as_processed_fn = &mark_as_processed;

    physical_return_actions.key_capture_fn = &end_with_capture_next_keys;
    physical_return_actions.no_capture_fn = &no_capture;

    layout_manager_initialize_nested_layers();
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

void pipeline_process_key(abskeyevent_t abskeyevent) {
    DEBUG_PRINT("=== ITERATION ===");

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

    if (event_added) {
        process_key_pool();
    } else if (buffer_full) {
        DEBUG_EXECUTOR("Error: Key event buffer is full, cannot add event");
        // Reset the global state
        pipeline_executor_reset_state();
        return;
    }
    DEBUG_BUFFERS(event_added ? "Key event buffer after processing:" : "Key event buffer not modified:");
    DEBUG_RETURN_DATA();
    DEBUG_PRINT("=================");
    DEBUG_PRINT_NL();
    return;
}
