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
        reset_mock_state();
    }

    void TearDown() override {
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
    }
};

// Simple Key Replacer
// Objective: Verify key replacer functionality with a single output
TEST_F(KeyReplacer, SimpleKeyReplacerWithSingleOutput) {
    const uint16_t ONE_SHOT_KEY = 100;
    const uint16_t OUTPUT_KEY1 = 101;
    const uint16_t OUTPUT_KEY2 = 102;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
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

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(OUTPUT_KEY1, 0),
        td_report_send(0),
        td_report_release(OUTPUT_KEY2, 0),
        td_report_send(0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}

// Simple Key Replacer
// Objective: Verify key replacer functionality with multiple outputs
TEST_F(KeyReplacer, SimpleKeyReplacerWithMultipleOutputs) {
    const uint16_t ONE_SHOT_KEY = 100;
    const uint16_t OUTPUT_KEY1 = 101;
    const uint16_t OUTPUT_KEY2 = 102;
    const uint16_t OUTPUT_KEY3 = 103;
    const uint16_t OUTPUT_KEY4 = 104;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
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

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(OUTPUT_KEY1, 0),
        td_report_press(OUTPUT_KEY2, 0),
        td_report_send(0),
        td_report_release(OUTPUT_KEY3, 0),
        td_report_release(OUTPUT_KEY4, 0),
        td_report_send(0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}

