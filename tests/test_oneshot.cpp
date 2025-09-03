#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "keyboard_simulator.hpp"
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "test_scenario.hpp"
#include "oneshot_test_helpers.hpp"

extern "C" {
#include "pipeline_oneshot_modifier.h"
#include "pipeline_oneshot_modifier_initializer.h"
#include "pipeline_executor.h"
}

class OneShotModifier : public ::testing::Test {
protected:
    void SetUp() override {
        reset_mock_state();
    }

    void TearDown() override {
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
    }
};

// Test one modifier on a single oneshot key
TEST_F(OneShotModifier, OneShotWithOneModifier) {
    const uint16_t ONE_SHOT_KEY = 100;
    const uint16_t OUTPUT_KEY = 101;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        {{ ONE_SHOT_KEY, OUTPUT_KEY }}
    }};

    TestScenario scenario(keymap);
    OneShotConfigBuilder config_builder;
    config_builder
        .add_modifiers(ONE_SHOT_KEY, {MACRO_KEY_MODIFIER_LEFT_CTRL})
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key(ONE_SHOT_KEY);
    keyboard.release_key(ONE_SHOT_KEY);
    keyboard.press_key(OUTPUT_KEY);
    keyboard.release_key(OUTPUT_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(PLATFORM_KC_LEFT_CTRL, 0),
        td_report_press(OUTPUT_KEY, 0),
        td_report_send(0),
        td_report_release(PLATFORM_KC_LEFT_CTRL, 0),
        td_report_send(0),
        td_release(OUTPUT_KEY),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}

// Test multiple modifiers on a single oneshot key
TEST_F(OneShotModifier, OneShotWithMultipleModifiers) {
    const uint16_t ONE_SHOT_KEY = 200;
    const uint16_t OUTPUT_KEY = 201;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        {{ ONE_SHOT_KEY, OUTPUT_KEY }}
    }};

    TestScenario scenario(keymap);
    OneShotConfigBuilder config_builder;
    config_builder
        .add_modifiers(ONE_SHOT_KEY, {MACRO_KEY_MODIFIER_LEFT_CTRL, MACRO_KEY_MODIFIER_LEFT_SHIFT})
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key(ONE_SHOT_KEY);
    keyboard.release_key(ONE_SHOT_KEY);
    keyboard.press_key(OUTPUT_KEY);
    keyboard.release_key(OUTPUT_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(PLATFORM_KC_LEFT_SHIFT, 0),
        td_report_press(PLATFORM_KC_LEFT_CTRL, 0),
        td_report_press(OUTPUT_KEY, 0),
        td_report_send(0),
        td_report_release(PLATFORM_KC_LEFT_SHIFT, 0),
        td_report_release(PLATFORM_KC_LEFT_CTRL, 0),
        td_report_send(0),
        td_release(OUTPUT_KEY),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}
