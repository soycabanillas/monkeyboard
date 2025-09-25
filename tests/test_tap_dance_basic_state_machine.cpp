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

class BasicStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

// Simple Tap
// Objective: Verify basic tap sequence with release before hold timeout
TEST_F(BasicStateMachineTest, SimpleTap) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3001;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);

    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY, 150),
        td_release(OUTPUT_KEY, 150)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Simple Hold
// Objective: Verify basic hold sequence with timeout triggering hold action
TEST_F(BasicStateMachineTest, SimpleHold) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3002;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Hold Timeout Boundary - Just Before
// Objective: Verify tap behavior when released exactly at hold timeout boundary
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryJustBefore) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3002;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 0}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);

    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY, 199),
        td_release(OUTPUT_KEY, 199)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Hold Timeout Boundary - Exactly At
// Objective: Verify hold behavior when timeout occurs exactly at boundary
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryExactlyAt) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3002;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);

    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 200)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Hold Timeout Boundary - Just After
// Objective: Verify hold behavior when held past timeout
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryJustAfter) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3002;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 201);

    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 201)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// No Hold Action Configured - Immediate Execution
// Objective: Verify immediate execution when no hold action available
TEST_F(BasicStateMachineTest, NoHoldActionConfiguredImmediateExecution) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3002;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);

    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY, 0),
        td_release(OUTPUT_KEY, 150)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Only Hold Action Configured - Timeout Not Reached
// Objective: Verify behavior when only hold action is configured
TEST_F(BasicStateMachineTest, OnlyHoldActionTimeoutNotReached) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);

    std::vector<event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Only Hold Action - Timeout Reached
// Objective: Verify hold action executes when only hold configured and timeout reached
TEST_F(BasicStateMachineTest, OnlyHoldActionTimeoutReached) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Tap -> Reset -> Hold
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, TapResetHold) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY_1 = 3003;
    const platform_keycode_t OUTPUT_KEY_2 = 3004;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }, {
        { 3002 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, 
                     {{1, OUTPUT_KEY_1}, {2, OUTPUT_KEY_2}}, 
                     {{1, TARGET_LAYER_1}, {2, TARGET_LAYER_2}}, 
                     200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // First sequence - tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);
    
    // Wait for tap timeout then second sequence - hold
    keyboard.press_key_at(TAP_DANCE_KEY, 400);
    keyboard.release_key_at(TAP_DANCE_KEY, 650);

    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY_1, 350),
        td_release(OUTPUT_KEY_1, 350),
        td_layer(TARGET_LAYER_1, 600),
        td_layer(0, 650)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Tap -> Reset -> Tap
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, TapResetTap) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY_1 = 3003;
    const platform_keycode_t OUTPUT_KEY_2 = 3004;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }, {
        { 3002 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, 
                     {{1, OUTPUT_KEY_1}, {2, OUTPUT_KEY_2}}, 
                     {{1, TARGET_LAYER_1}, {2, TARGET_LAYER_2}}, 
                     200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // First sequence - tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);

    // Wait for tap timeout then second sequence - tap
    keyboard.press_key_at(TAP_DANCE_KEY, 400);
    keyboard.release_key_at(TAP_DANCE_KEY, 550);
    keyboard.wait_ms(200); // Ensure tap timeout is reached

    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY_1, 350),
        td_release(OUTPUT_KEY_1, 350),
        td_press(OUTPUT_KEY_1, 750),
        td_release(OUTPUT_KEY_1, 750)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Hold -> Reset -> Tap
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, HoldResetTap) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY_1 = 3003;
    const platform_keycode_t OUTPUT_KEY_2 = 3004;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }, {
        { 3002 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, 
                     {{1, OUTPUT_KEY_1}, {2, OUTPUT_KEY_2}}, 
                     {{1, TARGET_LAYER_1}, {2, TARGET_LAYER_2}}, 
                     200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // First sequence - hold
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    // Second sequence - tap
    keyboard.press_key_at(TAP_DANCE_KEY, 300);
    keyboard.release_key_at(TAP_DANCE_KEY, 450);
    keyboard.wait_ms(200); // Ensure tap timeout is reached

    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER_1, 200),
        td_layer(0, 250),
        td_press(OUTPUT_KEY_1, 650),
        td_release(OUTPUT_KEY_1, 650)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Hold -> Reset -> Hold
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, HoldResetHold) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY_1 = 3003;
    const platform_keycode_t OUTPUT_KEY_2 = 3004;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 3001 }
    }, {
        { 3002 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, 
                     {{1, OUTPUT_KEY_1}, {2, OUTPUT_KEY_2}}, 
                     {{1, TARGET_LAYER_1}, {2, TARGET_LAYER_2}}, 
                     200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // First sequence - hold
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    // Second sequence - hold
    keyboard.press_key_at(TAP_DANCE_KEY, 300);
    keyboard.release_key_at(TAP_DANCE_KEY, 550);

    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER_1, 200),
        td_layer(0, 250),
        td_layer(TARGET_LAYER_1, 500),
        td_layer(0, 550)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Hold with no actions configured
// Objective: Verify hold functionality when there are no actions
TEST_F(BasicStateMachineTest, HoldWithNoActionsConfigured) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, 
                     {},
                     {},
                     200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);  // Empty configuration
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Tap with no actions configured
// Objective: Verify tap functionality when there are no actions
TEST_F(BasicStateMachineTest, TapWithNoActionsConfigured) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, 
                     {},
                     {},
                     200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);  // Empty configuration
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);

    std::vector<event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}
