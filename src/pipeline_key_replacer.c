#include "pipeline_key_replacer.h"
#include "key_virtual_buffer.h"
#include "pipeline_executor.h"
#include <stddef.h>
#include <stdint.h>


void pipeline_key_replacer_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* user_data) {
    //platform_log_debug("pipeline_key_replacer_callback || up: %u || press: %u", params->up, params->callback_type);
    platform_virtual_buffer_virtual_event_t* key_event = params->key_event;

    pipeline_key_replacer_global_config_t* data = (pipeline_key_replacer_global_config_t*)user_data;
    if (key_event->is_press) {
        for (size_t i = 0; i < data->length; i++)
        {
            if (data->modifier_pairs[i]->keycode == key_event->keycode) {
                uint8_t buffer_size = data->modifier_pairs[i]->press_event_buffer->buffer_length;
                if (buffer_size > 0) {
                    for (size_t j = 0; j < buffer_size; j++) {
                        actions->report_press_fn(data->modifier_pairs[i]->press_event_buffer->buffer[j].keycode);
                    }
                    actions->report_send_fn();
                }
                break;
            }
        }
    } else {
        for (size_t i = 0; i < data->length; i++)
        {
            if (data->modifier_pairs[i]->keycode == key_event->keycode) {
                uint8_t buffer_size = data->modifier_pairs[i]->release_event_buffer->buffer_length;
                if (buffer_size > 0) {
                    for (size_t j = 0; j < buffer_size; j++) {
                        actions->report_release_fn(data->modifier_pairs[i]->release_event_buffer->buffer[j].keycode);
                    }
                    actions->report_send_fn();
                }
                break;
            }
        }
    }
}

void pipeline_key_replacer_callback_reset(void* user_data) {

}
