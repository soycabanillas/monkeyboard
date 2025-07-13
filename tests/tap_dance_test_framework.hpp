#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

// Test event types
enum class TestEventType {
    KEY_PRESS,
    KEY_RELEASE,
    TIME_PASSED,
    EXPECT_KEY_SENT,
    EXPECT_KEY_PRESSED,
    EXPECT_KEY_RELEASED,
    EXPECT_LAYER_SELECT,
    EXPECT_NO_EVENT
};

// Test event structure
struct TestEvent {
    TestEventType type;
    uint16_t keycode;
    uint16_t time_ms;
    uint8_t layer;
    std::string description;

    // Factory methods for cleaner test creation
    static TestEvent key_press(uint16_t keycode, const std::string& desc = "") {
        return {TestEventType::KEY_PRESS, keycode, 0, 0, desc};
    }

    static TestEvent key_release(uint16_t keycode, const std::string& desc = "") {
        return {TestEventType::KEY_RELEASE, keycode, 0, 0, desc};
    }

    static TestEvent time_passed(uint16_t ms, const std::string& desc = "") {
        return {TestEventType::TIME_PASSED, 0, ms, 0, desc};
    }

    static TestEvent expect_key_sent(uint16_t keycode, const std::string& desc = "") {
        return {TestEventType::EXPECT_KEY_SENT, keycode, 0, 0, desc};
    }

    static TestEvent expect_key_pressed(uint16_t keycode, const std::string& desc = "") {
        return {TestEventType::EXPECT_KEY_PRESSED, keycode, 0, 0, desc};
    }

    static TestEvent expect_key_released(uint16_t keycode, const std::string& desc = "") {
        return {TestEventType::EXPECT_KEY_RELEASED, keycode, 0, 0, desc};
    }

    static TestEvent expect_layer_select(uint8_t layer, const std::string& desc = "") {
        return {TestEventType::EXPECT_LAYER_SELECT, 0, 0, layer, desc};
    }

    static TestEvent expect_no_event(const std::string& desc = "") {
        return {TestEventType::EXPECT_NO_EVENT, 0, 0, 0, desc};
    }
};

// Test configuration builder
class TapDanceTestConfig {
private:
    std::vector<pipeline_tap_dance_behaviour_t*> behaviours;

public:
    // Add a simple tap action
    TapDanceTestConfig& add_tap_key(uint16_t trigger_keycode, uint8_t tap_count, uint16_t output_keycode, uint8_t layer = 0) {
        pipeline_tap_dance_action_config_t* action = createbehaviouraction(tap_count, TDCL_TAP_KEY_SENDKEY, output_keycode, layer);  // Use tap_count directly (1-based)
        pipeline_tap_dance_action_config_t* actions[] = {action};
        behaviours.push_back(createbehaviour(trigger_keycode, actions, 1));
        return *this;
    }

    // Add a simple hold action
    TapDanceTestConfig& add_hold_key(uint16_t trigger_keycode, uint8_t tap_count, uint8_t target_layer, int16_t interrupt_config = 0) {
        pipeline_tap_dance_action_config_t* action = createbehaviouraction_with_interrupt(tap_count, TDCL_HOLD_KEY_CHANGELAYERTEMPO, 0, target_layer, interrupt_config);  // Use tap_count directly (1-based)
        pipeline_tap_dance_action_config_t* actions[] = {action};
        behaviours.push_back(createbehaviour(trigger_keycode, actions, 1));
        return *this;
    }

    // Add a complete tap dance behavior with multiple actions
    TapDanceTestConfig& add_tap_dance(uint16_t trigger_keycode,
                                     std::vector<std::pair<uint8_t, uint16_t>> tap_actions,
                                     std::vector<std::tuple<uint8_t, uint8_t, int16_t>> hold_actions = {}) {
        std::vector<pipeline_tap_dance_action_config_t*> actions;

        for (const auto& tap_action : tap_actions) {
            actions.push_back(createbehaviouraction(tap_action.first, TDCL_TAP_KEY_SENDKEY, tap_action.second, 0));  // Use tap_action.first directly (1-based)
        }

        for (const auto& hold_action : hold_actions) {
            actions.push_back(createbehaviouraction_with_interrupt(std::get<0>(hold_action), TDCL_HOLD_KEY_CHANGELAYERTEMPO, 0, std::get<1>(hold_action), std::get<2>(hold_action)));  // Use std::get<0> directly (1-based)
        }

        pipeline_tap_dance_action_config_t** action_array = new pipeline_tap_dance_action_config_t*[actions.size()];
        for (size_t i = 0; i < actions.size(); i++) {
            action_array[i] = actions[i];
        }

        behaviours.push_back(createbehaviour(trigger_keycode, action_array, actions.size()));
        return *this;
    }

    pipeline_tap_dance_global_config_t* build() {
        size_t n_elements = behaviours.size();
        pipeline_tap_dance_global_config_t* config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*config) + n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));
        config->length = n_elements;

        for (size_t i = 0; i < n_elements; i++) {
            config->behaviours[i] = behaviours[i];
        }

        return config;
    }
};

// Main test framework class
class TapDanceTestFramework : public ::testing::Test {
protected:
    pipeline_tap_dance_global_config_t* global_config;
    std::unique_ptr<TapDanceTestConfig> config_builder;

    void SetUp() override {
        reset_mock_state();

        // Initialize pipeline executor
        size_t n_pipelines = 3;
        pipeline_executor_config = static_cast<pipeline_executor_config_t*>(
            malloc(sizeof(pipeline_executor_config_t) + n_pipelines * sizeof(pipeline_t*)));
        pipeline_executor_config->length = n_pipelines;
        pipeline_executor_global_state_create();

        config_builder = std::make_unique<TapDanceTestConfig>();
    }

    void TearDown() override {
        pipeline_executor_global_state_destroy();
        // Note: In a real implementation, we'd need proper cleanup of allocated memory
    }

    // Setup the tap dance configuration
    void setup_tap_dance(TapDanceTestConfig& config) {
        global_config = config.build();
        pipeline_tap_dance_global_state_create();
        pipeline_executor_config->pipelines[1] = add_pipeline(&pipeline_tap_dance_callback, global_config);
    }

    // Execute a sequence of test events
    void execute_test_sequence(const std::vector<TestEvent>& events) {
        size_t expectation_index = 0;

        for (const auto& event : events) {
            switch (event.type) {
                case TestEventType::KEY_PRESS:
                    simulate_key_event(event.keycode, true);
                    break;

                case TestEventType::KEY_RELEASE:
                    simulate_key_event(event.keycode, false);
                    break;

                case TestEventType::TIME_PASSED:
                    platform_wait_ms(event.time_ms);
                    break;

                case TestEventType::EXPECT_KEY_SENT:
                    EXPECT_GT(g_mock_state.send_key_calls_count(), expectation_index)
                        << "Expected key sent: " << event.keycode << " (" << event.description << ")";
                    if (g_mock_state.send_key_calls_count() > expectation_index) {
                        EXPECT_EQ(g_mock_state.send_key_calls[expectation_index], event.keycode)
                            << "Wrong key sent (" << event.description << ")";
                        expectation_index++;
                    }
                    break;

                case TestEventType::EXPECT_KEY_PRESSED:
                    EXPECT_TRUE(g_mock_state.is_key_pressed(event.keycode))
                        << "Expected key pressed: " << event.keycode << " (" << event.description << ")";
                    break;

                case TestEventType::EXPECT_KEY_RELEASED:
                    EXPECT_FALSE(g_mock_state.is_key_pressed(event.keycode))
                        << "Expected key released: " << event.keycode << " (" << event.description << ")";
                    break;

                case TestEventType::EXPECT_LAYER_SELECT:
                    EXPECT_EQ(g_mock_state.current_layer, event.layer)
                        << "Expected layer: " << (int)event.layer << " (" << event.description << ")";
                    break;

                case TestEventType::EXPECT_NO_EVENT:
                    // Check that no new events occurred since last expectation
                    EXPECT_EQ(g_mock_state.send_key_calls_count(), expectation_index)
                        << "Unexpected key events occurred (" << event.description << ")";
                    break;
            }
        }
    }

private:
    void simulate_key_event(uint16_t keycode, bool pressed, uint16_t time_offset = 0) {
        abskeyevent_t event = {
            .key = {.row = 0, .col = 0},
            .pressed = pressed,
            .time = static_cast<uint16_t>(platform_timer_read() + time_offset)
        };
        if (time_offset > 0) {
            platform_wait_ms(time_offset);
        }
        pipeline_process_key(keycode, event);
    }
};

// Common test keycodes
#define TEST_KEY_A 0x7E00
#define TEST_KEY_B 0x7E01
#define TEST_KEY_C 0x7E02
#define TEST_KEY_TAP_DANCE_1 0x7E10
#define TEST_KEY_TAP_DANCE_2 0x7E11
#define TEST_KEY_TAP_DANCE_3 0x7E12

// Common output keycodes
#define OUT_KEY_X 0x04  // KC_A equivalent
#define OUT_KEY_Y 0x1D  // KC_Z equivalent
#define OUT_KEY_Z 0x1C  // KC_Y equivalent

// Common layers
#define LAYER_BASE 0
#define LAYER_SYMBOLS 1
#define LAYER_NUMBERS 2
#define LAYER_FUNCTION 3
