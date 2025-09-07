#pragma once

#include <vector>
#include <memory>
#include "test_scenario.hpp"

extern "C" {
#include "pipeline_key_replacer.h"
#include "pipeline_key_replacer_initializer.h"
}

class KeyReplacerEventBufferBuilder {
private:
    std::vector<platform_keycode_t> keycodes_;

public:
    KeyReplacerEventBufferBuilder& add_key(platform_keycode_t keycode) {
        keycodes_.push_back(keycode);
        return *this;
    }

    KeyReplacerEventBufferBuilder& add_keys(const std::vector<platform_keycode_t>& keycodes) {
        keycodes_.insert(keycodes_.end(), keycodes.begin(), keycodes.end());
        return *this;
    }

    platform_key_replacer_event_buffer_t* build() {
        platform_key_replacer_event_buffer_t* buffer = static_cast<platform_key_replacer_event_buffer_t*>(
            malloc(sizeof(platform_key_replacer_event_buffer_t)));
        
        buffer->buffer_length = keycodes_.size();
        for (size_t i = 0; i < keycodes_.size(); ++i) {
            buffer->buffer[i].keycode = keycodes_[i];
        }
        
        return buffer;
    }
};

class KeyReplacerConfigBuilder {
private:
    std::vector<pipeline_key_replacer_pair_t*> pairs_;

public:
    KeyReplacerConfigBuilder& add_replacement(platform_keycode_t trigger_key,
                                              const std::vector<platform_keycode_t>& press_keys,
                                              const std::vector<platform_keycode_t>& release_keys) {
        KeyReplacerEventBufferBuilder press_builder;
        KeyReplacerEventBufferBuilder release_builder;
        
        press_builder.add_keys(press_keys);
        release_builder.add_keys(release_keys);
        
        platform_key_replacer_event_buffer_t* press_buffer = press_builder.build();
        platform_key_replacer_event_buffer_t* release_buffer = release_builder.build();
        
        pipeline_key_replacer_pair_t* pair = pipeline_key_replacer_create_pairs(
            trigger_key, press_buffer, release_buffer);
        pairs_.push_back(pair);
        
        return *this;
    }

    pipeline_key_replacer_global_config_t* build() {
        pipeline_key_replacer_global_config_t* config = static_cast<pipeline_key_replacer_global_config_t*>(
            malloc(sizeof(*config)));
        
        config->length = pairs_.size();
        config->modifier_pairs = static_cast<pipeline_key_replacer_pair_t**>(
            malloc(pairs_.size() * sizeof(pipeline_key_replacer_pair_t*)));
        
        for (size_t i = 0; i < pairs_.size(); ++i) {
            config->modifier_pairs[i] = pairs_[i];
        }
        
        return config;
    }

    TestScenario& add_to_scenario(TestScenario& scenario) {
        pipeline_key_replacer_global_config_t* config = build();
        return scenario.add_virtual_pipeline(
            &pipeline_key_replacer_callback_process_data_executor,
            &pipeline_key_replacer_callback_reset_executor,
            config);
    }
};
