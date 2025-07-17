#include "pipeline_executor.h"
#include <stdint.h>
#include <stdlib.h>
#include "key_event_buffer.h"
#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_layout.h"
#include "platform_types.h"

pipeline_executor_state_t pipeline_executor_state;
pipeline_executor_config_t *pipeline_executor_config;

//Functions available in the pipeline_info_t struct

static bool is_keycode_pressed(platform_keycode_t keycode){
    return platform_key_press_keycode_is_pressed(pipeline_executor_state.key_press_buffer, keycode);
}

static void register_key(platform_keycode_t keycode, platform_keypos_t keypos) {
    platform_register_keycode(keycode);
    //add_to_press_buffer(pipeline_executor_state.key_buffer, keycode, abskeyevent.key, abskeyevent.time, abskeyevent.pressed, true, true, pipeline_executor_state.pipeline_index);
}

static void unregister_key(platform_keycode_t keycode) {
    platform_unregister_keycode(keycode);
    // add_to_press_buffer(pipeline_executor_state.key_buffer, keycode, abskeyevent.key, abskeyevent.time, 0, abskeyevent.pressed, true, pipeline_executor_state.pipeline_index);
}

static void tap_key(platform_keycode_t keycode, platform_keypos_t keypos) {
    platform_tap_keycode(keycode);
    // intern_tap_key(keycode, keypos);
    // intern_untap_key(keycode);
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

    pipeline_actions_t keybuffer;
    keybuffer.register_key_fn = &register_key;
    keybuffer.unregister_key_fn = &unregister_key;
    keybuffer.tap_key_fn = &tap_key;
    keybuffer.is_keycode_pressed = &is_keycode_pressed;
    pipeline_executor_config->pipelines[pipeline_index]->callback(&callback_params, &keybuffer, pipeline_executor_config->pipelines[pipeline_index]->data);
}

static void deferred_exec_callback(void *cb_arg) {
    (void)cb_arg; // Unused parameter
    execute_pipeline(pipeline_executor_state.capture_pipeline.callback_time, pipeline_executor_state.capture_pipeline.pipeline_index, pipeline_executor_state.key_event_buffer, NULL, &pipeline_executor_state.capture_pipeline);
}

static bool key_has_been_processed_on_pipeline_execution(capture_pipeline_t* capture_pipeline) {
    return (capture_pipeline->key_event_buffer == true || capture_pipeline->captured == true);
}

static void execute_middleware(uint8_t middleware_pos, platform_key_event_t* press_buffer_selected, bool* key_digested, bool* last_pipeline_execution_changed_buffer, bool* last_pipeline_execution_captured_key_processing) {
    execute_pipeline(0, middleware_pos, pipeline_executor_state.key_event_buffer, press_buffer_selected, &pipeline_executor_state.capture_pipeline);
    *last_pipeline_execution_changed_buffer = pipeline_executor_state.capture_pipeline.key_event_buffer;
    if (pipeline_executor_state.capture_pipeline.key_event_buffer == true) {
        // Swap buffers if the middleware has changed the key event buffer
        pipeline_executor_state.key_event_buffer_swap = pipeline_executor_state.key_event_buffer;
        pipeline_executor_state.key_event_buffer = pipeline_executor_state.capture_pipeline.key_buffer;
        platform_key_event_reset(pipeline_executor_state.key_event_buffer_swap);
        pipeline_executor_state.capture_pipeline.key_event_buffer = false; // Reset swap buffer flag
    }
    else {
        platform_key_event_reset(pipeline_executor_state.key_event_buffer_swap);
    }
    *key_digested = key_digested || key_has_been_processed_on_pipeline_execution(&pipeline_executor_state.capture_pipeline);
    *last_pipeline_execution_captured_key_processing = pipeline_executor_state.capture_pipeline.captured;
}

static bool process_key_pool(void) {

    pipeline_executor_state.capture_pipeline.callback_time = 0;
    pipeline_executor_state.capture_pipeline.captured = false;
    pipeline_executor_state.capture_pipeline.pipeline_index = 0;
    pipeline_executor_state.capture_pipeline.key_event_buffer = false; // Reset swap buffer flag
    pipeline_executor_state.capture_pipeline.key_buffer = pipeline_executor_state.key_event_buffer_swap;

    if (pipeline_executor_state.capture_pipeline.callback_time > 0) {
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }

    bool key_digested = false;
    bool last_pipeline_execution_changed_buffer = false;
    bool last_pipeline_execution_captured_key_processing = false;

    while (last_pipeline_execution_changed_buffer == true) {
        platform_key_event_t* press_buffer_selected = &pipeline_executor_state.key_event_buffer->event_buffer[0];
        if (pipeline_executor_state.capture_pipeline.captured == true) {
            execute_middleware(pipeline_executor_state.capture_pipeline.pipeline_index, press_buffer_selected, &key_digested, &last_pipeline_execution_changed_buffer, &last_pipeline_execution_captured_key_processing);
        } else {
            for (size_t i = 0; i < pipeline_executor_config->length; i++) {
                pipeline_executor_state.pipeline_index = i;
                execute_middleware(i, press_buffer_selected, &key_digested, &last_pipeline_execution_changed_buffer, &last_pipeline_execution_captured_key_processing);
                if (last_pipeline_execution_captured_key_processing == true) break;
            };
        }
        //remove_from_press_buffer(pipeline_executor_state.key_buffer,  0);
    }

    if (pipeline_executor_state.capture_pipeline.captured == true && pipeline_executor_state.capture_pipeline.callback_time > 0) {
        pipeline_executor_state.deferred_exec_callback_token = platform_defer_exec(pipeline_executor_state.capture_pipeline.callback_time, deferred_exec_callback, NULL);
    }

    // if (press_buffer_selected->keycode <= 0xFF) {
    //     platform_keycode_t fallthrough = press_buffer_selected->keycode;
    //     if (press_buffer[0].is_press == true) {
    //         platform_register_code(fallthrough);
    //         platform_wait_ms(10);
    //     } else {
    //         platform_unregister_code(fallthrough);
    //         platform_wait_ms(10);
    //     }
    // }
    // print_press_buffers(10);

    key_digested = (key_digested && pipeline_executor_state.capture_pipeline.captured == true);
    return (key_digested == false);
}

void pipeline_executor_capture_next_keys_or_callback_on_timeout(platform_time_t callback_time) {
    pipeline_executor_state.capture_pipeline.callback_time = callback_time;
    pipeline_executor_state.capture_pipeline.captured = true;
    pipeline_executor_state.capture_pipeline.pipeline_index = pipeline_executor_state.pipeline_index;
}

void pipeline_executor_capture_next_keys(void) {
    pipeline_executor_state.capture_pipeline.callback_time = 0;
    pipeline_executor_state.capture_pipeline.captured = true;
    pipeline_executor_state.capture_pipeline.pipeline_index = pipeline_executor_state.pipeline_index;
}

void pipeline_executor_global_state_create(void) {
    pipeline_executor_state.key_press_buffer = platform_key_press_create();
    pipeline_executor_state.key_event_buffer = platform_key_event_create();
    pipeline_executor_state.key_event_buffer_swap = platform_key_event_create();
    pipeline_executor_state.capture_pipeline.callback_time = 0;
    pipeline_executor_state.capture_pipeline.captured = false;
    pipeline_executor_state.capture_pipeline.pipeline_index = 0;
    pipeline_executor_state.capture_pipeline.key_event_buffer = false;
    pipeline_executor_state.capture_pipeline.key_buffer = pipeline_executor_state.key_event_buffer_swap;
    pipeline_executor_state.pipeline_index = 0; // Initialize the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Initialize the deferred execution callback token
}

pipeline_t* add_pipeline(pipeline_callback callback, void* user_data) {
    pipeline_t* pipeline;
    pipeline = malloc(sizeof(pipeline_t));
    pipeline->callback = callback;
    pipeline->data = user_data;

    return pipeline;
}

bool pipeline_process_key(abskeyevent_t abskeyevent) {
    uint8_t layer = platform_layout_get_current_layer_impl();
    platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, abskeyevent.key);

    if (platform_key_press_add_press(pipeline_executor_state.key_press_buffer, abskeyevent.time, layer, abskeyevent.key, abskeyevent.pressed)) {
        platform_key_event_add_event(pipeline_executor_state.key_event_buffer, abskeyevent.time, layer, abskeyevent.key, keycode, abskeyevent.pressed);
        return process_key_pool();
    }
    return true;
}
