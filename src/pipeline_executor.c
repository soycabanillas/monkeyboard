#include "pipeline_executor.h"
#include <stdint.h>
#include <stdlib.h>
#include "key_buffer.h"
#include "platform_interface.h"
#include "platform_layout.h"

pipeline_executor_state_t pipeline_executor_state;
pipeline_executor_config_t *pipeline_executor_config;

//Functions available in the pipeline_info_t struct

static bool is_keycode_pressed(platform_keycode_t keycode){
    return platform_keycode_is_pressed(pipeline_executor_state.key_buffer, keycode);
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
    callback_params.info.is_keycode_pressed = &is_keycode_pressed;

    pipeline_actions_t actions;
    actions.register_key_fn = &register_key;
    actions.unregister_key_fn = &unregister_key;
    actions.tap_key_fn = &tap_key;
    pipeline_executor_config->pipelines[pos]->callback(&callback_params, &actions, pipeline_executor_config->pipelines[pos]->data);
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
    pipeline_t* pipeline;
    pipeline = malloc(sizeof(pipeline_t));
    pipeline->callback = callback;
    pipeline->data = user_data;

    return pipeline;
}

bool pipeline_process_key(abskeyevent_t abskeyevent) {
    if (add_to_press_buffer(pipeline_executor_state.key_buffer, abskeyevent.key, abskeyevent.time, platform_layout_get_current_layer_impl(), abskeyevent.pressed)) {
        return process_key_pool();
    }
    return true;
}
