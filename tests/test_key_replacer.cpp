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
#include "key_replacer_test_helpers.hpp"

extern "C" {
#include "pipeline_key_replacer.h"
#include "pipeline_key_replacer_initializer.h"
#include "pipeline_executor.h"
}

class KeyReplacer : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

// Simple Key Replacer
// Objective: Verify key replacer functionality with a single output
TEST_F(KeyReplacer, SimpleKeyReplacerWithSingleOutput) {
    const platform_keycode_t ONE_SHOT_KEY = 100;
    const platform_keycode_t OUTPUT_KEY1 = 101;
    const platform_keycode_t OUTPUT_KEY2 = 102;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { ONE_SHOT_KEY }
    }};

    TestScenario scenario(keymap);
    KeyReplacerConfigBuilder config_builder;
    config_builder
        .add_replacement(ONE_SHOT_KEY, { OUTPUT_KEY1 }, { OUTPUT_KEY2 })
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key(ONE_SHOT_KEY);
    keyboard.release_key(ONE_SHOT_KEY);

    std::vector<event_t> expected_events = {
        td_report_press(OUTPUT_KEY1, 0),
        td_report_send(0),
        td_report_release(OUTPUT_KEY2, 0),
        td_report_send(0)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_relative(expected_events));
}

// Simple Key Replacer
// Objective: Verify key replacer functionality with multiple outputs
TEST_F(KeyReplacer, SimpleKeyReplacerWithMultipleOutputs) {
    const platform_keycode_t ONE_SHOT_KEY = 100;
    const platform_keycode_t OUTPUT_KEY1 = 101;
    const platform_keycode_t OUTPUT_KEY2 = 102;
    const platform_keycode_t OUTPUT_KEY3 = 103;
    const platform_keycode_t OUTPUT_KEY4 = 104;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { ONE_SHOT_KEY }
    }};

    TestScenario scenario(keymap);
    KeyReplacerConfigBuilder config_builder;
    config_builder
        .add_replacement(ONE_SHOT_KEY, {OUTPUT_KEY1, OUTPUT_KEY2}, {OUTPUT_KEY3, OUTPUT_KEY4})
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key(ONE_SHOT_KEY);
    keyboard.release_key(ONE_SHOT_KEY);

    std::vector<event_t> expected_events = {
        td_report_press(OUTPUT_KEY1, 0),
        td_report_press(OUTPUT_KEY2, 0),
        td_report_send(0),
        td_report_release(OUTPUT_KEY3, 0),
        td_report_release(OUTPUT_KEY4, 0),
        td_report_send(0)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_relative(expected_events));
}

