#include "pipeline_key_replacer.h"
#include "pipeline_executor.h"
#include "platform_interface.h"
#include <stddef.h>

void pipeline_key_replacer_callback_process_data(pipeline_virtual_callback_params_t* params, pipeline_virtual_actions_t* actions, void* user_data) {
    //platform_log_debug("pipeline_key_replacer_callback || up: %u || press: %u", params->up, params->callback_type);
    platform_virtual_buffer_virtual_event_t* key_event = params->key_event;
    
    pipeline_key_replacer_global_t* data = (pipeline_key_replacer_global_t*)user_data;
    if (key_event->is_press) {
        for (size_t i = 0; i < data->config->length; i++)
        {
            if (data->config->modifier_pairs[i]->keycode == key_event->keycode) {
                for (size_t j = 0; j < data->config->modifier_pairs[i]->virtual_event_buffer_press->press_buffer_pos; j++) {
                    actions->register_key_fn(data->config->modifier_pairs[i]->virtual_event_buffer_press->press_buffer[j].keycode);
                }
            }
        }
    } else {
        for (size_t i = 0; i < data->config->length; i++)
        {
            if (data->config->modifier_pairs[i]->keycode == key_event->keycode) {
                for (size_t j = 0; j < data->config->modifier_pairs[i]->virtual_event_buffer_release->press_buffer_pos; j++) {
                    actions->unregister_key_fn(data->config->modifier_pairs[i]->virtual_event_buffer_release->press_buffer[j].keycode);
                }
            }
        }
    }
}

void pipeline_oneshot_modifier_callback_reset(void* user_data) {

}
