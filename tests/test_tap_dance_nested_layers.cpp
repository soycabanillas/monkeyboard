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
#include "tap_dance_test_helpers.hpp"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class TapDanceNestedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

TEST_F(TapDanceNestedTest, SimpleNestingABBA) {
    const platform_keycode_t TAP_DANCE_KEY_1 = 2001;
    const platform_keycode_t OUTPUT_KEY_1 = 2002;
    const platform_keycode_t INTERRUPTING_KEY_1 = 2003;
    const platform_keycode_t TAP_DANCE_KEY_2 = 2111;
    const platform_keycode_t OUTPUT_KEY_2 = 2112;
    const platform_keycode_t INTERRUPTING_KEY_2 = 2113;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY_1, INTERRUPTING_KEY_1, 2051, 2052 }
    }, {
        { 2151, 2152, TAP_DANCE_KEY_2, INTERRUPTING_KEY_2 }
    }, {
        { 2053, 2054, 2055, 2056 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    // Configure first tap dance key: tap -> OUTPUT_KEY_1, hold -> layer 1
    config_builder.add_tap_hold(TAP_DANCE_KEY_1, {{1, OUTPUT_KEY_1}}, {{1, 1}}, 200, 200, TAP_DANCE_HOLD_PREFERRED);
    
    // Configure second tap dance key: tap -> OUTPUT_KEY_2, hold -> layer 2
    config_builder.add_tap_hold(TAP_DANCE_KEY_2, {{1, OUTPUT_KEY_2}}, {{1, 2}}, 200, 200, TAP_DANCE_HOLD_PREFERRED);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY_1, 0);
    keyboard.press_key_at(2051, 200);
    keyboard.release_key_at(2051, 400);
    keyboard.release_key_at(TAP_DANCE_KEY_1, 400);

    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(1, 400),
        td_layer(0, 400)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

