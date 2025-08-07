#include "pipeline_tap_dance_initializer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pipeline_tap_dance.h"
#include "platform_types.h"

pipeline_tap_dance_behaviour_status_t* pipeline_tap_dance_behaviour_state_create(void) {
    pipeline_tap_dance_behaviour_status_t* behaviour_status = (pipeline_tap_dance_behaviour_status_t*)malloc(sizeof(pipeline_tap_dance_behaviour_status_t));
    //behaviour_status->key_buffer = platform_key_event_create(); // Initialize the key buffer for this tap dance sequence
    reset_behaviour_state(behaviour_status);
    return behaviour_status;
}

pipeline_tap_dance_action_config_t* createbehaviouraction_tap(uint8_t tap_count, platform_keycode_t keycode) {
    pipeline_tap_dance_action_config_t behaviouraction = {
        .tap_count = tap_count,
        .action = TDCL_TAP_KEY_SENDKEY,
        .keycode = keycode
    };
    pipeline_tap_dance_action_config_t* allocation = (pipeline_tap_dance_action_config_t*)malloc(sizeof behaviouraction);
    memcpy(allocation, &behaviouraction, sizeof behaviouraction);
    return allocation;
}

pipeline_tap_dance_action_config_t* createbehaviouraction_hold(uint8_t tap_count, uint8_t layer, tap_dance_hold_strategy_t hold_strategy) {
    pipeline_tap_dance_action_config_t behaviouraction = {
        .tap_count = tap_count,
        .action = TDCL_HOLD_KEY_CHANGELAYERTEMPO,
        .layer = layer,
        .hold_strategy = hold_strategy
    };
    pipeline_tap_dance_action_config_t* allocation = (pipeline_tap_dance_action_config_t*)malloc(sizeof behaviouraction);
    memcpy(allocation, &behaviouraction, sizeof behaviouraction);
    return allocation;
}

pipeline_tap_dance_behaviour_t* createbehaviour(platform_keycode_t keycodemodifier, pipeline_tap_dance_action_config_t* actions[], size_t actionslength) {
    pipeline_tap_dance_behaviour_config_t userdata = {
        .keycodemodifier = keycodemodifier,
        .actionslength = actionslength,
        .hold_timeout = g_hold_timeout,
        .tap_timeout = g_tap_timeout
    };
    pipeline_tap_dance_behaviour_config_t* allocationuserdata = (pipeline_tap_dance_behaviour_config_t*)malloc(sizeof(pipeline_tap_dance_behaviour_config_t) + actionslength * sizeof (pipeline_tap_dance_action_config_t*));
    memcpy(allocationuserdata, &userdata, sizeof userdata);
    for (size_t i = 0; i < actionslength; i++)
    {
        allocationuserdata->actions[i] = actions[i];
    }
    pipeline_tap_dance_behaviour_t behaviour = {
        .status = pipeline_tap_dance_behaviour_state_create(),
        .config = allocationuserdata
    };
    pipeline_tap_dance_behaviour_t* allocation = (pipeline_tap_dance_behaviour_t*)malloc(sizeof behaviour);
    memcpy(allocation, &behaviour, sizeof behaviour);
    return allocation;
}


