#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pipeline_tap_dance.h"
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

pipeline_tap_dance_action_config_t* createbehaviouraction_tap(uint8_t tap_count, platform_keycode_t keycode);
pipeline_tap_dance_action_config_t* createbehaviouraction_hold(uint8_t tap_count, uint8_t layer, tap_dance_hold_strategy_t hold_strategy);
pipeline_tap_dance_behaviour_t* createbehaviour(platform_keycode_t keycodemodifier, pipeline_tap_dance_action_config_t* actions[], size_t actionslength);

#ifdef __cplusplus
}
#endif
