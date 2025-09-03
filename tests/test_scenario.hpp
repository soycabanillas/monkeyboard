#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "keyboard_simulator.hpp"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"

extern "C" {
#include "pipeline_executor.h"
}

// Base class for pipeline configurations
class PipelineConfig {
public:
    virtual ~PipelineConfig() = default;
    virtual void add_to_executor(size_t index) = 0;
};

// Physical pipeline configuration
class PhysicalPipelineConfig : public PipelineConfig {
private:
    pipeline_physical_callback process_callback_;
    pipeline_callback_reset reset_callback_;
    void* config_data_;

public:
    PhysicalPipelineConfig(pipeline_physical_callback process_cb,
                          pipeline_callback_reset reset_cb,
                          void* config_data)
        : process_callback_(process_cb), reset_callback_(reset_cb), config_data_(config_data) {}

    void add_to_executor(size_t index) override {
        pipeline_executor_add_physical_pipeline(index, process_callback_, reset_callback_, config_data_);
    }
};

// Virtual pipeline configuration
class VirtualPipelineConfig : public PipelineConfig {
private:
    pipeline_virtual_callback process_callback_;
    pipeline_callback_reset reset_callback_;
    void* config_data_;

public:
    VirtualPipelineConfig(pipeline_virtual_callback process_cb,
                         pipeline_callback_reset reset_cb,
                         void* config_data)
        : process_callback_(process_cb), reset_callback_(reset_cb), config_data_(config_data) {}

    void add_to_executor(size_t index) override {
        pipeline_executor_add_virtual_pipeline(index, process_callback_, reset_callback_, config_data_);
    }
};

class TestScenario {
private:
    std::vector<std::unique_ptr<PipelineConfig>> physical_pipelines_;
    std::vector<std::unique_ptr<PipelineConfig>> virtual_pipelines_;
    KeyboardSimulator keyboard_;

public:
    TestScenario(const std::vector<std::vector<std::vector<uint16_t>>>& keymap) {
        reset_mock_state();

        // Convert vector keymap to C array format
        size_t layers = keymap.size();
        size_t rows = keymap[0].size();
        size_t cols = keymap[0][0].size();
        
        uint16_t* flat_keymap = static_cast<uint16_t*>(malloc(layers * rows * cols * sizeof(uint16_t)));
        for (size_t l = 0; l < layers; ++l) {
            for (size_t r = 0; r < rows; ++r) {
                for (size_t c = 0; c < cols; ++c) {
                    flat_keymap[l * rows * cols + r * cols + c] = keymap[l][r][c];
                }
            }
        }

        keyboard_ = create_layout(flat_keymap, layers, rows, cols);
        // Note: keeping flat_keymap allocated as keyboard might reference it
    }

    TestScenario& add_physical_pipeline(pipeline_physical_callback process_cb,
                                       pipeline_callback_reset reset_cb,
                                       void* config_data) {
        physical_pipelines_.push_back(
            std::make_unique<PhysicalPipelineConfig>(process_cb, reset_cb, config_data));
        return *this;
    }

    TestScenario& add_virtual_pipeline(pipeline_virtual_callback process_cb,
                                      pipeline_callback_reset reset_cb,
                                      void* config_data) {
        virtual_pipelines_.push_back(
            std::make_unique<VirtualPipelineConfig>(process_cb, reset_cb, config_data));
        return *this;
    }

    void build() {
        pipeline_executor_create_config(physical_pipelines_.size(), virtual_pipelines_.size());
        
        // Add physical pipelines
        for (size_t i = 0; i < physical_pipelines_.size(); ++i) {
            physical_pipelines_[i]->add_to_executor(i);
        }
        
        // Add virtual pipelines
        for (size_t i = 0; i < virtual_pipelines_.size(); ++i) {
            virtual_pipelines_[i]->add_to_executor(i);
        }
    }

    KeyboardSimulator& keyboard() { return keyboard_; }

    ~TestScenario() {
        // Note: In a real implementation, you'd want proper cleanup here
        // For now, relying on test framework cleanup
    }
};
