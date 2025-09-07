#pragma once

#include <vector>
#include <memory>
#include "test_scenario.hpp"

extern "C" {
#include "pipeline_oneshot_modifier.h"
#include "pipeline_oneshot_modifier_initializer.h"
}

class OneShotConfigBuilder {
private:
    std::vector<pipeline_oneshot_modifier_pair_t*> pairs_;

public:
    OneShotConfigBuilder& add_modifiers(platform_keycode_t trigger_key, 
                                       const std::vector<uint8_t>& modifiers) {
        uint8_t combined_modifier = 0;
        for (uint8_t modifier : modifiers) {
            combined_modifier |= modifier;
        }
        pipeline_oneshot_modifier_pair_t* pair = pipeline_oneshot_modifier_create_pairs(
            trigger_key, combined_modifier);
        pairs_.push_back(pair);
        return *this;
    }

    pipeline_oneshot_modifier_global_t* build() {
        pipeline_oneshot_modifier_global_status_t* global_status = 
            pipeline_oneshot_modifier_global_state_create();
        
        pipeline_oneshot_modifier_global_config_t* global_config = 
            static_cast<pipeline_oneshot_modifier_global_config_t*>(
                malloc(sizeof(*global_config)));
        
        global_config->length = pairs_.size();
        global_config->modifier_pairs = static_cast<pipeline_oneshot_modifier_pair_t**>(
            malloc(pairs_.size() * sizeof(pipeline_oneshot_modifier_pair_t*)));
        
        for (size_t i = 0; i < pairs_.size(); ++i) {
            global_config->modifier_pairs[i] = pairs_[i];
        }
        
        pipeline_oneshot_modifier_global_t* global = 
            static_cast<pipeline_oneshot_modifier_global_t*>(
                malloc(sizeof(*global)));
        global->config = global_config;
        global->status = global_status;
        
        return global;
    }

    TestScenario& add_to_scenario(TestScenario& scenario) {
        pipeline_oneshot_modifier_global_t* global = build();
        return scenario.add_virtual_pipeline(
            &pipeline_oneshot_modifier_callback_process_data_executor,
            &pipeline_oneshot_modifier_callback_reset_executor,
            global);
    }
};
