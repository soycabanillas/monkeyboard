#include "pipeline_executor.h"
#include <stdint.h>
#include <stdlib.h>
#include "key_buffer.h"
#include "platform_interface.h"

pipeline_executor_state_t pipeline_executor_state;
pipeline_executor_config_t *pipeline_executor_config;

//Functions available in the pipeline_info_t struct

bool info_is_pressed(platform_keycode_t keycode){
    return key_buffer_is_pressed(pipeline_executor_state.key_buffer, keycode);
}

void intern_tap_key(platform_keycode_t keycode, platform_keypos_t keypos) {
    abskeyevent_t abskeyevent;
    abskeyevent.key = keypos;
    abskeyevent.pressed = true;
    abskeyevent.time = platform_timer_read();
    platform_register_key(keycode);
    add_to_press_buffer(pipeline_executor_state.key_buffer, keycode, abskeyevent.key, abskeyevent.time, abskeyevent.pressed, true, true, pipeline_executor_state.pipeline_index);
}

void intern_untap_key(platform_keycode_t keycode) {
    abskeyevent_t abskeyevent;
    // Use invalid keypos (-1, -1) to indicate pipeline-generated event
    // This distinguishes it from real physical key positions
    abskeyevent.key.col = 255;  // Use max value to indicate invalid/pipeline position
    abskeyevent.key.row = 255;
    abskeyevent.pressed = false;  // Fixed: untap should be false, not true
    abskeyevent.time = platform_timer_read();
    platform_unregister_key(keycode);
    // add_to_press_buffer(pipeline_executor_state.key_buffer, keycode, abskeyevent.key, abskeyevent.time, 0, abskeyevent.pressed, true, pipeline_executor_state.pipeline_index);
}

void intern_add_key(platform_keycode_t keycode, platform_keypos_t keypos) {
    platform_send_key(keycode);
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
void execute_pipeline(uint16_t callback_time, uint8_t pos, press_buffer_item_t* press_buffer_selected, capture_pipeline_t return_data) {
    pipeline_callback_params_t callback_params;
    if (callback_time == 0) {
        callback_params.keycode = press_buffer_selected->keycode;
        callback_params.keypos = press_buffer_selected->key;
        callback_params.time = callback_time;
        callback_params.layer = press_buffer_selected->layer;
        if (press_buffer_selected->is_press == true) {
            callback_params.callback_type = PIPELINE_CALLBACK_KEY_PRESS;
        } else {
            callback_params.callback_type = PIPELINE_CALLBACK_KEY_RELEASE;
        }
    } else {
        callback_params.callback_type = PIPELINE_CALLBACK_TIMER;
        callback_params.time = callback_time;
    }
    callback_params.info.is_pressed_fn = &info_is_pressed;

    pipeline_actions_t actions;
    actions.add_tap_fn = &intern_tap_key;
    actions.add_untap_fn = &intern_untap_key;
    actions.add_key_fn = &intern_add_key;
    pipeline_executor_config->pipelines[pos]->definition->callback(&callback_params, &actions, pipeline_executor_config->pipelines[pos]->definition->data);
}

void deferred_exec_callback(void *cb_arg) {
    (void)cb_arg; // Unused parameter
    execute_pipeline(pipeline_executor_state.capture_pipeline.callback_time, pipeline_executor_state.capture_pipeline.pipeline_index, NULL, pipeline_executor_state.capture_pipeline);
}

bool process_key_pool(void) {

    pipeline_executor_state.capture_pipeline.callback_time = 0;
    pipeline_executor_state.capture_pipeline.captured = false;
    pipeline_executor_state.capture_pipeline.pipeline_index = 0;

    if (pipeline_executor_state.capture_pipeline.callback_time > 0) {
        platform_cancel_deferred_exec(pipeline_executor_state.deferred_exec_callback_token);
    }
    while (pipeline_executor_state.key_buffer->press_buffer_pos > 0) {
        press_buffer_item_t* press_buffer_selected = &pipeline_executor_state.key_buffer->press_buffer[0];
        if (pipeline_executor_state.capture_pipeline.captured == true) {
            execute_pipeline(0, pipeline_executor_state.capture_pipeline.pipeline_index, press_buffer_selected, pipeline_executor_state.capture_pipeline);
        } else {
            for (size_t i = 0; i < pipeline_executor_config->length; i++) {
                pipeline_executor_state.pipeline_index = i;
                execute_pipeline(0, i, press_buffer_selected, pipeline_executor_state.capture_pipeline);
                if (pipeline_executor_state.capture_pipeline.captured == true) break;
            };
        }
        remove_from_press_buffer(pipeline_executor_state.key_buffer,  0);

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

    return false;
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
    pipeline_executor_state.capture_pipeline.callback_time = 0;
    pipeline_executor_state.capture_pipeline.captured = false;
    pipeline_executor_state.capture_pipeline.pipeline_index = 0;
    pipeline_executor_state.key_buffer = pipeline_key_buffer_create();
    pipeline_executor_state.pipeline_index = 0; // Initialize the pipeline index
    pipeline_executor_state.deferred_exec_callback_token = 0; // Initialize the deferred execution callback token
}

void pipeline_executor_global_state_destroy(void) {
    pipeline_key_buffer_destroy(pipeline_executor_state.key_buffer);
}

pipeline_t* add_pipeline(pipeline_callback callback, void* user_data) {
    pipeline_definition_t* definition = malloc(sizeof(pipeline_definition_t));
    definition->callback = callback;
    definition->data = user_data;

    pipeline_t* pipeline;
    pipeline = malloc(sizeof(pipeline_t));
    pipeline->key_buffer = malloc(sizeof(key_buffer_t)); // Use the global key buffer
    pipeline->definition = definition;

    return pipeline;
}

bool pipeline_process_key(platform_keycode_t keycode, abskeyevent_t abskeyevent) {
    if (add_to_press_buffer(pipeline_executor_state.key_buffer, keycode, abskeyevent.key, abskeyevent.time, 0, abskeyevent.pressed, false, 0)) {
        return process_key_pool();
    }
    return true;
}
