#include "pipeline_key_replacer.h"
#include "key_virtual_buffer.h"
#include "pipeline_executor.h"
#include <stddef.h>
#include <stdint.h>


void pipeline_key_replacer_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, pipeline_key_replacer_global_config_t* config) {
    //platform_log_debug("pipeline_key_replacer_callback || up: %u || press: %u", params->up, params->callback_type);
    platform_virtual_buffer_virtual_event_t* key_event = params->key_event;

    if (key_event->is_press) {
        for (size_t i = 0; i < config->length; i++)
        {
            if (config->modifier_pairs[i]->keycode == key_event->keycode) {
                uint8_t buffer_size = config->modifier_pairs[i]->press_event_buffer->buffer_length;
                if (buffer_size > 0) {
                    for (size_t j = 0; j < buffer_size; j++) {
                        actions->report_press_fn(config->modifier_pairs[i]->press_event_buffer->buffer[j].keycode);
                    }
                    actions->report_send_fn();
                }
                break;
            }
        }
    } else {
        for (size_t i = 0; i < config->length; i++)
        {
            if (config->modifier_pairs[i]->keycode == key_event->keycode) {
                uint8_t buffer_size = config->modifier_pairs[i]->release_event_buffer->buffer_length;
                if (buffer_size > 0) {
                    for (size_t j = 0; j < buffer_size; j++) {
                        actions->report_release_fn(config->modifier_pairs[i]->release_event_buffer->buffer[j].keycode);
                    }
                    actions->report_send_fn();
                }
                break;
            }
        }
    }
}

void pipeline_key_replacer_callback_process_data_executor(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* config) {
    pipeline_key_replacer_callback_process_data(params, actions, config);
}

void pipeline_key_replacer_callback_reset(pipeline_key_replacer_global_config_t* config) {

}

void pipeline_key_replacer_callback_reset_executor(void* config) {
    pipeline_key_replacer_callback_reset(config);
}
