#include "pipeline_tap_dance_initializer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pipeline_tap_dance.h"
#include "platform_interface.h"

pipeline_tap_dance_behaviour_status_t* pipeline_tap_dance_behaviour_state_create(void) {
    pipeline_tap_dance_behaviour_status_t* behaviour_status = (pipeline_tap_dance_behaviour_status_t*)malloc(sizeof(pipeline_tap_dance_behaviour_status_t));
    reset_behaviour_state(behaviour_status);
    return behaviour_status;
}

pipeline_tap_dance_action_config_t* createbehaviouraction(uint8_t tap_count, td_customlayer_action_t action, platform_keycode_t keycode, uint8_t layer) {
    pipeline_tap_dance_action_config_t behaviouraction = {
        .tap_count = tap_count,
        .action = action,
        .keycode = keycode,
        .layer = layer,
        .interrupt_config = 0  // Default interrupt config (activate on any key press)
    };
    pipeline_tap_dance_action_config_t* allocation = (pipeline_tap_dance_action_config_t*)malloc(sizeof behaviouraction);
    memcpy(allocation, &behaviouraction, sizeof behaviouraction);
    return allocation;
}

pipeline_tap_dance_action_config_t* createbehaviouraction_with_interrupt(uint8_t tap_count, td_customlayer_action_t action, platform_keycode_t keycode, uint8_t layer, int16_t interrupt_config) {
    pipeline_tap_dance_action_config_t behaviouraction = {
        .tap_count = tap_count,
        .action = action,
        .keycode = keycode,
        .layer = layer,
        .interrupt_config = interrupt_config
    };
    pipeline_tap_dance_action_config_t* allocation = (pipeline_tap_dance_action_config_t*)malloc(sizeof behaviouraction);
    memcpy(allocation, &behaviouraction, sizeof behaviouraction);
    return allocation;
}

pipeline_tap_dance_behaviour_t* createbehaviour(platform_keycode_t keycodemodifier, pipeline_tap_dance_action_config_t* actions[], size_t actionslength) {
    pipeline_tap_dance_behaviour_config_t userdata = {
        .keycodemodifier = keycodemodifier,
        .actionslength = actionslength
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


