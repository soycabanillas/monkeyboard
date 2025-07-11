#include "pipeline_tap_dance.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pipeline_executor.h"
#include "platform_interface.h"

#define g_tap_timeout 200

pipeline_tap_dance_global_status_t* global_status;

platform_keycode_t lastKeyTapped = 0;
platform_time_t lastKeyTappedTime = 0;
platform_keycode_t lastKeyUntapped = 0;
platform_time_t lastKeyUntappedTime = 0;

// Forward declarations
pipeline_tap_dance_action_config_t* get_action_tap_key_sendkey(uint8_t repetitions, pipeline_tap_dance_behaviour_config_t* custom_switch_layer_custom_data);

// uint32_t hold_span_reached_timer(uint32_t trigger_time, void *cb_arg) {
//     pipeline_tap_dance_behaviour_status_t *status = (pipeline_tap_dance_behaviour_status_t*)cb_arg;
//     status->state = _HOLD_TRESHOLD_DETECTED;
//     status->sequence = _HOLD;
//     platform_layer_select(status->selected_layer);
//     status->hold_span_reached_token = 0;
//     return 0;
// }

// uint32_t key_repetition_span_exceeded_timer(uint32_t trigger_time, void *cb_arg) {
//     pipeline_tap_dance_behaviour_status_t *status = (pipeline_tap_dance_behaviour_status_t*)cb_arg;
//     status->key_repetition_span_exceeded_token = 0;
//     // platform_log_debug("key_repetition_span_exceeded_timer - tap_code16_delay");
//     platform_tap_code_delay(status->selected_keycode, 10);
//     return 0;
// }

pipeline_tap_dance_action_config_t* get_action_tap_key_sendkey(uint8_t repetitions, pipeline_tap_dance_behaviour_config_t* custom_switch_layer_custom_data) {
    pipeline_tap_dance_action_config_t* action;
    for (size_t i = 0; i < custom_switch_layer_custom_data->actionslength; i++)
    {
        action = custom_switch_layer_custom_data->actions[i];
        if (action->repetitions == repetitions && action->action == TDCL_TAP_KEY_SENDKEY) {
            return action;
        }
    }
    return NULL;
}

pipeline_tap_dance_action_config_t* get_action_hold_key_changelayertempo(uint8_t repetitions, pipeline_tap_dance_behaviour_config_t* custom_switch_layer_custom_data) {
    pipeline_tap_dance_action_config_t* action;
    for (size_t i = 0; i < custom_switch_layer_custom_data->actionslength; i++)
    {
        action = custom_switch_layer_custom_data->actions[i];
        if (action->repetitions == repetitions && action->action == TDCL_HOLD_KEY_CHANGELAYERTEMPO) {
            return action;
        }
    }
    return NULL;
}

bool has_subsequent_actions(pipeline_tap_dance_behaviour_config_t* custom_switch_layer_custom_data, uint8_t repetitions) {
    pipeline_tap_dance_action_config_t* action;
    for (size_t i = 0; i < custom_switch_layer_custom_data->actionslength; i++)
    {
        action = custom_switch_layer_custom_data->actions[i];
        if (action->repetitions > repetitions) {
            return true;
        }
    }
    return false;
}

void reset_behaviour_state(pipeline_tap_dance_behaviour_status_t *layer_status) {
    layer_status->key_status = _WAITING_FOR_TAP;
    layer_status->key_tap_repetition_count = 0;

    layer_status->state = _HOLD_STATE_NOT_SET;
    layer_status->sequence = _HOLD_OR_TAP_STATE_NOT_SET;
    layer_status->count = 0;
    layer_status->hasholdaction = false;
    layer_status->haskeyaction = false;
    layer_status->selected_layer = 0;
    layer_status->selected_keycode = 0;
    layer_status->keybuffer_length = 0;
    layer_status->hold_span_reached_token = 0;
    layer_status->key_repetition_span_exceeded_token = 0;
}

void custom_switch_layer_custom_function (pipeline_callback_params_t* params, pipeline_tap_dance_behaviour_config_t *behaviour_config, pipeline_tap_dance_behaviour_status_t *behaviour_status, pipeline_actions_t* actions) {
    platform_keycode_t keycode = params->keycode;

    bool pressed = params->callback_type == PIPELINE_CALLBACK_KEY_PRESS;
    if (behaviour_config->actionslength == 0) {
        //platform_log_debug("cslcd - exit because actionslength == 0");
        return;
    }
    if (keycode == (behaviour_config->keycodemodifier)) {
        if (pressed == true) {
            behaviour_status->key_tap_repetition_count++;
            if (behaviour_status->key_tap_repetition_count == 1) {
                behaviour_status->original_layer = params->layer;
            }
        }

        if (pressed == true && behaviour_status->key_status == _WAITING_FOR_TAP) {
            if (has_subsequent_actions(behaviour_config, behaviour_status->key_tap_repetition_count - 1) == true) {
                behaviour_status->key_status = _AFTER_TAP;
                pipeline_executor_capture_next_keys_or_callback_on_timeout(g_tap_timeout);
            } else {
                pipeline_tap_dance_action_config_t* tap_key_action = get_action_tap_key_sendkey(behaviour_status->key_tap_repetition_count - 1, behaviour_config);
                behaviour_status->key_status = _AFTER_TAP;
                if (tap_key_action != NULL) {
                    reset_behaviour_state(behaviour_status);
                    actions->add_tap_fn(keycode, params->keypos);
                }
            }
        }
        if (pressed == false && behaviour_status->key_status == _WAITING_FOR_TAP) {
            // ignore
        }
        if (pressed == true && behaviour_status->key_status == _AFTER_TAP) {
            // ignore
        }
        if (pressed == false && behaviour_status->key_status == _AFTER_TAP) {
            if (has_subsequent_actions(behaviour_config, behaviour_status->key_tap_repetition_count - 1) == true) {
                pipeline_executor_capture_next_keys_or_callback_on_timeout(g_tap_timeout);
            } else {
                pipeline_tap_dance_action_config_t* tap_key_action = get_action_tap_key_sendkey(behaviour_status->key_tap_repetition_count - 1, behaviour_config);
                if (tap_key_action != NULL) {
                    reset_behaviour_state(behaviour_status);
                    actions->add_untap_fn(keycode);
                }
            }
        }
        if (pressed == true && behaviour_status->key_status == _HOLD_CONFIRMED) {
            // ignore
        }
        if (pressed == false && behaviour_status->key_status == _HOLD_CONFIRMED) {
            pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(behaviour_status->key_tap_repetition_count - 1, behaviour_config);
            if (hold_action != NULL) {
                behaviour_status->key_status = _HOLD_CONFIRMED;
                behaviour_status->selected_layer = hold_action->layer;
                platform_layer_select(behaviour_status->selected_layer);
            }
            reset_behaviour_state(behaviour_status);
        }
        if (pressed == true && behaviour_status->key_status == _AFTER_UNTAP) {
            if (has_subsequent_actions(behaviour_config, behaviour_status->key_tap_repetition_count - 1) == true) {
                behaviour_status->key_status = _WAITING_FOR_TAP;
                //pipeline_executor_capture_next_keys_or_callback_on_timeout(g_tap_timeout);
            } else {
                // ignore
            }
        }
        if (pressed == false && behaviour_status->key_status == _AFTER_UNTAP) {
        }
    }
    // if (keycode == (behaviour_config->keycodemodifier)) {
    //     if (pressed) {
    //         behaviour_status->state = _HOLD_STATE_NOT_SET;

    //         if (behaviour_status->sequence == _HOLD_OR_TAP_STATE_NOT_SET || behaviour_status->sequence == _NONE) {
    //             behaviour_status->count = 1;
    //         } else if (behaviour_status->sequence == _HOLD) {
    //             behaviour_status->count = 1;
    //         } else if (behaviour_status->sequence == _TAP) {
    //             if (has_subsequent_actions(behaviour_config, behaviour_status->count - 1) == true) {
    //                 platform_cancel_deferred_exec(behaviour_status->key_repetition_span_exceeded_token);
    //                 behaviour_status->key_repetition_span_exceeded_token = 0;
    //                 behaviour_status->count = behaviour_status->count + 1;
    //             } else {
    //                 behaviour_status->count = 1;
    //             }
    //         } else if (behaviour_status->sequence == _DECIDING) {
    //             //platform_log_debug("Invalid state: layer_status->hold_or_tap_sequence == _DECIDING");
    //         }
    //         if (behaviour_status->count == 1) {
    //             behaviour_status->original_layer = 0;//get_layer_topdown(event.key);
    //         }
    //         behaviour_status->sequence = _DECIDING;

    //         pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(behaviour_status->count - 1, behaviour_config);
    //         if (hold_action != NULL) {
    //             behaviour_status->state = _DURING_HOLD_DECISION;

    //             behaviour_status->hasholdaction = true;
    //             behaviour_status->selected_layer = hold_action->layer;
    //             behaviour_status->keybuffer_length = 0;
    //             behaviour_status->hold_span_reached_token = platform_defer_exec(g_tap_timeout, hold_span_reached_timer, behaviour_status);
    //         } else {
    //             behaviour_status->hasholdaction = false;
    //             behaviour_status->selected_layer = 0;
    //             behaviour_status->keybuffer_length = 0;
    //             behaviour_status->hold_span_reached_token = 0;
    //         }
    //         pipeline_tap_dance_action_config_t* tap_key_action = get_action_tap_key_sendkey(behaviour_status->count - 1, behaviour_config);
    //         if (tap_key_action != NULL) {
    //             behaviour_status->haskeyaction = true;
    //             behaviour_status->selected_keycode = tap_key_action->keycode;
    //         } else {
    //             behaviour_status->haskeyaction = false;
    //             behaviour_status->selected_keycode = 0;
    //         }
    //         // platform_log_debug("cslcd - pressed  : count: %u, hasholdaction: %u, haskeyaction %u, g_tap_timeout %u", layer_status->count, layer_status->hasholdaction, layer_status->haskeyaction, g_tap_timeout);
    //     } else {
    //         if (behaviour_status->state == _DURING_HOLD_DECISION) {
    //             platform_cancel_deferred_exec(behaviour_status->hold_span_reached_token);
    //             behaviour_status->hold_span_reached_token = 0;

    //             if (behaviour_status->haskeyaction) {
    //                 behaviour_status->sequence = _TAP;
    //             } else {
    //                 behaviour_status->sequence = _NONE;
    //             }
    //         } else if (behaviour_status->state == _HOLD_TRESHOLD_DETECTED) {
    //             platform_layer_select(behaviour_status->selected_layer);
    //             platform_clear_keyboard();
    //             // The value of layer_status->sequence has been set on the deferred execution to _HOLD
    //         } else if (behaviour_status->haskeyaction) {
    //             behaviour_status->sequence = _TAP;
    //         } else {
    //             behaviour_status->sequence = _NONE;
    //         }

    //         if (behaviour_status->sequence == _TAP && has_subsequent_actions(behaviour_config, behaviour_status->count - 1) == true) {
    //             behaviour_status->key_repetition_span_exceeded_token = platform_defer_exec(g_tap_timeout, key_repetition_span_exceeded_timer, behaviour_status);

    //             //->callback_layer_pos = layer_status->selected_layer;
    //             //callback_time = g_tap_timeout;

    //         }
    //         else {
    //             actions->add_key_fn(behaviour_status->selected_keycode, params->keypos);
    //             behaviour_status->state = _HOLD_STATE_NOT_SET;
    //         }
    //         //platform_log_debug("cslcd - unpressed: count: %u, hasholdaction: %u, haskeyaction: %u, selected_keycode: %u", layer_status->count, layer_status->hasholdaction, layer_status->haskeyaction, layer_status->selected_keycode);
    //     }
    //     return;
    // } else {
    //     if (behaviour_status->sequence == _DECIDING) {
    //         // if (layer_status->keybuffer_length < 8)
    //         // {
    //         //     layer_status->keybuffer[layer_status->keybuffer_length].key = event.key;
    //         //     layer_status->keybuffer[layer_status->keybuffer_length].pressed = event.pressed;
    //         //     layer_status->keybuffer[layer_status->keybuffer_length].time = event.time;
    //         //     layer_status->keybuffer_length += 1;
    //         // }
    //         return;
    //     } else if (behaviour_status->sequence == _HOLD) {
    //         //decide_over_keypressed(keycode, layer_status, false);
    //     } else if (behaviour_status->sequence == _TAP) {
    //         if (has_subsequent_actions(behaviour_config, behaviour_status->count - 1) == true) {
    //             platform_cancel_deferred_exec(behaviour_status->key_repetition_span_exceeded_token);
    //             behaviour_status->key_repetition_span_exceeded_token = 0;
    //         }
    //         behaviour_status->sequence = _HOLD_OR_TAP_STATE_NOT_SET;
    //     }
    //}
    return;
}

// bool macros_process_key(platform_keycode_t keycode, abskeyevent_t abskeyevent, custom_layers_struct *custom_layers) {
//     if (abskeyevent.pressed) {
//         lastKeyTapped = keycode;
//         lastKeyTappedTime = abskeyevent.time;
//     }
//     else {
//         lastKeyUntapped = keycode;
//         lastKeyUntappedTime = abskeyevent.time;
//     }
//     //print_layer_status();
//     //Esc and Enter should remove sticky keys... but what about other selections that use numbers or letters?
//     //platform_log_debug("sizeof *custom_layers: %u, nelements: %u, sizeof *custom_layers->layers: %u, modifier1: %u, modifier2: %u", sizeof *custom_layers, nelements, sizeof *custom_layers->layers, CKC_LAY_MOUSE_Q, CKC_LAY_NUMBERS_R);
//     for (uint8_t i = 0; i < custom_layers->length; i++)
//     {
//         //platform_log_debug("custom_layers_length: %u, keycodemodifier: %u", custom_layers->length, custom_layers->layers[i]->status.keycodemodifier);
//         custom_behaviour_config *layer_status = custom_layers->layers[i];
//         //platform_log_debug("keycodemodifier: %u", layer_status->status.keycodemodifier);
//         if (custom_switch_layer_custom_function(keycode, abskeyevent, &layer_status->status, layer_status->user_data) == false) {
//             return false;
//         }
//     }
//     return true;
// }

// Create the global state for tap dance

void pipeline_tap_dance_global_state_create(void) {
    global_status = (pipeline_tap_dance_global_status_t*)malloc(sizeof(pipeline_tap_dance_global_status_t));
    global_status->last_behaviour = 0;
}

void pipeline_tap_dance_callback(pipeline_callback_params_t* params, pipeline_actions_t* actions, void* user_data) {
    pipeline_tap_dance_global_config_t* global_config = (pipeline_tap_dance_global_config_t*)user_data;;


    if (params->callback_type == PIPELINE_CALLBACK_KEY_PRESS || params->callback_type == PIPELINE_CALLBACK_KEY_RELEASE) {
        if (params->callback_type == PIPELINE_CALLBACK_KEY_PRESS) {
            lastKeyTapped = params->keycode;
            lastKeyTappedTime = params->time;
        }
        else if (params->callback_type == PIPELINE_CALLBACK_KEY_RELEASE) {
            lastKeyUntapped = params->keycode;
            lastKeyUntappedTime = params->time;
        }
        for (uint8_t i = 0; i < global_config->length; i++)
        {
            global_status->last_behaviour = i;
            pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[i];
            custom_switch_layer_custom_function(params, behaviour->config, behaviour->status, actions);
        }
    }
    else if (params->callback_type == PIPELINE_CALLBACK_TIMER) {
        pipeline_tap_dance_behaviour_t *behaviour = global_config->behaviours[global_status->last_behaviour];
        pipeline_tap_dance_behaviour_config_t* behaviour_config = behaviour->config;
        pipeline_tap_dance_behaviour_status_t* behaviour_status = behaviour->status;
        if (behaviour_status->key_status == _AFTER_TAP) {
            pipeline_tap_dance_action_config_t* hold_action = get_action_hold_key_changelayertempo(behaviour_status->key_tap_repetition_count - 1, behaviour_config);
            if (hold_action != NULL) {
                behaviour_status->key_status = _HOLD_CONFIRMED;
                behaviour_status->selected_layer = hold_action->layer;
                platform_layer_select(behaviour_status->selected_layer);
            }
        } else if (behaviour_status->key_status == _AFTER_UNTAP) {
            if (has_subsequent_actions(behaviour_config, behaviour_status->key_tap_repetition_count - 1) == true) {
                behaviour_status->key_status = _WAITING_FOR_TAP;
            }
            else {
                // The key was added to the key buffer, so we don't need to do anything here
            }
        }
    }

    //print_layer_status();
    //Esc and Enter should remove sticky keys... but what about other selections that use numbers or letters?
    //platform_log_debug("sizeof *custom_layers: %u, nelements: %u, sizeof *custom_layers->layers: %u, modifier1: %u, modifier2: %u", sizeof *custom_layers, nelements, sizeof *custom_layers->layers, CKC_LAY_MOUSE_Q, CKC_LAY_NUMBERS_R);

    // if (params->more_data == true) {
    //     if (params->up == true && params->is_press == true) {
    //         for (uint8_t i = 0; i < custom_layers->length; i++)
    //         {
    //             custom_behaviour_config *layer_status = custom_layers->layers[i];
    //             if (layer_status->status.keycodemodifier == params->keycode) {
    //                 config->needs_more_data = true;
    //             }
    //             // if (layer_status->callback(keycode, abskeyevent, &layer_status->status, layer_status->user_data) == false) return false;
    //         }
    //     }
    // } else {
    //     if (params->up == true && params->is_press == true) {
    //         #ifdef CONSOLE_ENABLE
    //             uprintf("MORE_DATA : 0x%04X\n", params->keycode);
    //         #endif
    //     }
    // }
}
