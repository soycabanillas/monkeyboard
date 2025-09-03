#pragma once

#include <vector>
#include <memory>
#include "test_scenario.hpp"

extern "C" {
#include "pipeline_combo.h"
#include "pipeline_combo_initializer.h"
}

class ComboKeyBuilder {
private:
    platform_keypos_t keypos_;
    pipeline_combo_key_translation_t press_action_;
    pipeline_combo_key_translation_t release_action_;

public:
    ComboKeyBuilder(platform_keypos_t keypos) 
        : keypos_(keypos)
        , press_action_(create_combo_key_action(COMBO_KEY_ACTION_NONE, 0))
        , release_action_(create_combo_key_action(COMBO_KEY_ACTION_NONE, 0)) {}

    ComboKeyBuilder& with_press_action(pipeline_combo_key_action_t type, platform_keycode_t keycode) {
        press_action_ = create_combo_key_action(type, keycode);
        return *this;
    }

    ComboKeyBuilder& with_release_action(pipeline_combo_key_action_t type, platform_keycode_t keycode) {
        release_action_ = create_combo_key_action(type, keycode);
        return *this;
    }

    pipeline_combo_key_t* build() {
        return create_combo_key(keypos_, press_action_, release_action_);
    }
};

class ComboConfigBuilder {
private:
    std::vector<pipeline_combo_config_t*> combos_;
    combo_activate_strategy_t strategy_;

public:
    ComboConfigBuilder() : strategy_(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON) {}

    ComboConfigBuilder& with_strategy(combo_activate_strategy_t strategy) {
        strategy_ = strategy;
        return *this;
    }

    ComboConfigBuilder& add_combo(const std::vector<ComboKeyBuilder>& keys, 
                                  pipeline_combo_key_translation_t press_action, 
                                  pipeline_combo_key_translation_t release_action) {
        pipeline_combo_key_t** key_array = static_cast<pipeline_combo_key_t**>(
            malloc(keys.size() * sizeof(pipeline_combo_key_t*)));
        
        for (size_t i = 0; i < keys.size(); ++i) {
            ComboKeyBuilder builder_copy = keys[i];
            key_array[i] = builder_copy.build();
        }

        pipeline_combo_config_t* combo = create_combo(keys.size(), key_array, press_action, release_action);
        combos_.push_back(combo);
        return *this;
    }

    ComboConfigBuilder& add_simple_combo(const std::vector<platform_keypos_t>& positions,
                                         uint16_t output_keycode) {
        std::vector<ComboKeyBuilder> keys;
        for (const auto& pos : positions) {
            keys.emplace_back(pos);
        }
        
        pipeline_combo_key_translation_t press = create_combo_key_action(COMBO_KEY_ACTION_REGISTER, output_keycode);
        pipeline_combo_key_translation_t release = create_combo_key_action(COMBO_KEY_ACTION_UNREGISTER, output_keycode);
        
        return add_combo(keys, press, release);
    }

    pipeline_combo_global_config_t* build() {
        pipeline_combo_global_config_t* config = static_cast<pipeline_combo_global_config_t*>(
            malloc(sizeof(*config)));
        
        config->length = combos_.size();
        config->combos = static_cast<pipeline_combo_config_t**>(
            malloc(combos_.size() * sizeof(pipeline_combo_config_t*)));
        config->strategy = strategy_;

        for (size_t i = 0; i < combos_.size(); ++i) {
            config->combos[i] = combos_[i];
        }

        return config;
    }

    TestScenario& add_to_scenario(TestScenario& scenario) {
        pipeline_combo_global_state_create();
        pipeline_combo_global_config_t* config = build();
        return scenario.add_physical_pipeline(
            &pipeline_combo_callback_process_data_executor,
            &pipeline_combo_callback_reset_executor,
            config);
    }
};
