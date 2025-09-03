#pragma once

#include <vector>
#include <memory>
#include <map>
#include "test_scenario.hpp"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
}

class TapDanceActionBuilder {
private:
    pipeline_tap_dance_action_config_t* action_;

public:
    static TapDanceActionBuilder tap(uint8_t tap_count, platform_keycode_t keycode) {
        TapDanceActionBuilder builder;
        builder.action_ = createbehaviouraction_tap(tap_count, keycode);
        return builder;
    }

    static TapDanceActionBuilder hold(uint8_t tap_count, uint8_t layer, tap_dance_hold_strategy_t preference = TAP_DANCE_HOLD_PREFERRED) {
        TapDanceActionBuilder builder;
        builder.action_ = createbehaviouraction_hold(tap_count, layer, preference);
        return builder;
    }

    pipeline_tap_dance_action_config_t* build() {
        return action_;
    }
};

struct TapHoldActions {
    TapDanceActionBuilder* tap_action = nullptr;
    TapDanceActionBuilder* hold_action = nullptr;
};

class TapDanceBehaviorBuilder {
private:
    platform_keycode_t trigger_key_;
    std::map<uint8_t, TapHoldActions> actions_; // Map from tap_count to tap/hold actions
    uint32_t hold_timeout_;
    uint32_t tap_timeout_;

public:
    TapDanceBehaviorBuilder(platform_keycode_t trigger_key) 
        : trigger_key_(trigger_key), hold_timeout_(g_hold_timeout), tap_timeout_(g_tap_timeout) {}

    TapDanceBehaviorBuilder& add_tap(uint8_t tap_count, platform_keycode_t keycode) {
        actions_[tap_count].tap_action = new TapDanceActionBuilder(TapDanceActionBuilder::tap(tap_count, keycode));
        return *this;
    }

    TapDanceBehaviorBuilder& add_hold(uint8_t tap_count, uint8_t layer, 
                                     tap_dance_hold_strategy_t preference = TAP_DANCE_HOLD_PREFERRED) {
        actions_[tap_count].hold_action = new TapDanceActionBuilder(TapDanceActionBuilder::hold(tap_count, layer, preference));
        return *this;
    }

    TapDanceBehaviorBuilder& with_hold_timeout(uint32_t timeout_ms) {
        hold_timeout_ = timeout_ms;
        return *this;
    }

    TapDanceBehaviorBuilder& with_tap_timeout(uint32_t timeout_ms) {
        tap_timeout_ = timeout_ms;
        return *this;
    }

    pipeline_tap_dance_behaviour_t* build() {
        // Find the maximum tap count and count total actions
        uint8_t max_tap_count = 0;
        size_t total_actions = 0;
        for (const auto& pair : actions_) {
            if (pair.first > max_tap_count) {
                max_tap_count = pair.first;
            }
            if (pair.second.tap_action) total_actions++;
            if (pair.second.hold_action) total_actions++;
        }

        pipeline_tap_dance_action_config_t** action_array = static_cast<pipeline_tap_dance_action_config_t**>(
            malloc(total_actions * sizeof(pipeline_tap_dance_action_config_t*)));
        
        // Fill in the configured actions
        size_t action_index = 0;
        for (const auto& pair : actions_) {
            if (pair.second.tap_action) {
                action_array[action_index++] = pair.second.tap_action->build();
            }
            if (pair.second.hold_action) {
                action_array[action_index++] = pair.second.hold_action->build();
            }
        }

        pipeline_tap_dance_behaviour_t* behavior = createbehaviour(trigger_key_, action_array, total_actions);
        behavior->config->hold_timeout = hold_timeout_;
        behavior->config->tap_timeout = tap_timeout_;
        
        return behavior;
    }
};

class TapDanceConfigBuilder {
private:
    std::vector<TapDanceBehaviorBuilder> behaviors_;

public:
    TapDanceConfigBuilder& add_behavior(const TapDanceBehaviorBuilder& behavior) {
        behaviors_.push_back(behavior);
        return *this;
    }

    // Flexible add_tap_hold method that accepts optional parameters
    TapDanceConfigBuilder& add_tap_hold(platform_keycode_t trigger_key,
                                       const std::map<uint8_t, platform_keycode_t>& taps = {},
                                       const std::map<uint8_t, uint8_t>& holds = {},
                                       uint32_t hold_timeout = g_hold_timeout,
                                       uint32_t tap_timeout = g_tap_timeout) {
        TapDanceBehaviorBuilder behavior(trigger_key);
        
        // Add tap actions
        for (const auto& tap_pair : taps) {
            behavior.add_tap(tap_pair.first, tap_pair.second);
        }
        
        // Add hold actions
        for (const auto& hold_pair : holds) {
            behavior.add_hold(hold_pair.first, hold_pair.second);
        }
        
        behavior.with_hold_timeout(hold_timeout).with_tap_timeout(tap_timeout);
        return add_behavior(behavior);
    }

    pipeline_tap_dance_global_config_t* build() {
        size_t n_elements = behaviors_.size();
        pipeline_tap_dance_global_config_t* config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*config)));
        config->length = behaviors_.size();
        config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

        for (size_t i = 0; i < behaviors_.size(); ++i) {
            TapDanceBehaviorBuilder behavior_copy = behaviors_[i];
            config->behaviours[i] = behavior_copy.build();
        }

        return config;
    }

    TestScenario& add_to_scenario(TestScenario& scenario) {
        pipeline_tap_dance_global_state_create();
        pipeline_tap_dance_global_config_t* config = build();
        return scenario.add_physical_pipeline(
            &pipeline_tap_dance_callback_process_data_executor,
            &pipeline_tap_dance_callback_reset_executor,
            config);
    }
};
