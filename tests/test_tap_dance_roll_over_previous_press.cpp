#include <sys/wait.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "keyboard_simulator.hpp"
#include "test_scenario.hpp"
#include "tap_dance_test_helpers.hpp"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class TapDanceRollOverPreviousPress : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

const platform_keycode_t PREVIOUS_KEY_A = 2000;
const platform_keycode_t PREVIOUS_KEY_B = 2001;
const platform_keycode_t TAP_DANCE_KEY = 2002;
const platform_keycode_t OUTPUT_KEY = 2003;
const platform_keycode_t INTERRUPTING_KEY = 2004;

void test_A_TDK_A_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(OUTPUT_KEY, 30),
        td_release(OUTPUT_KEY, 30),
    };
    test_A_TDK_A_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(OUTPUT_KEY, 30),
        td_release(OUTPUT_KEY, 30),
    };
    test_A_TDK_A_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(OUTPUT_KEY, 30),
        td_release(OUTPUT_KEY, 30),
    };
    test_A_TDK_A_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
    };
    test_A_TDK_A_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
    };
    test_A_TDK_A_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
    };
    test_A_TDK_A_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_release(OUTPUT_KEY, 30),
    };
    test_A_TDK_A_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_release(OUTPUT_KEY, 30),
    };
    test_A_TDK_A_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_release(OUTPUT_KEY, 30),
    };
    test_A_TDK_A_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}

//

void test_A_TDK_HOLD_A_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    test_A_TDK_HOLD_A_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    test_A_TDK_HOLD_A_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    test_A_TDK_HOLD_A_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_HOLD_A_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    test_A_TDK_HOLD_A_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    test_A_TDK_HOLD_A_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    test_A_TDK_HOLD_A_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_HOLD_A_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 210),
        td_release(OUTPUT_KEY, 220),
    };
    test_A_TDK_HOLD_A_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 210),
        td_release(OUTPUT_KEY, 220),
    };
    test_A_TDK_HOLD_A_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_HOLD_A_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 210),
        td_release(OUTPUT_KEY, 220),
    };
    test_A_TDK_HOLD_A_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}

//

void test_A_TDK_A_HOLD_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_HOLD_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_HOLD_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_HOLD_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_HOLD_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_HOLD_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_release(OUTPUT_KEY, 210),
    };
    test_A_TDK_A_HOLD_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_release(OUTPUT_KEY, 210),
    };
    test_A_TDK_A_HOLD_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_HOLD_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_release(OUTPUT_KEY, 210),
    };
    test_A_TDK_A_HOLD_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}

//

void test_A_TDK_Interrupt_A_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_press(OUTPUT_KEY, 40),
        td_press(INTERRUPTING_KEY, 40),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 20),
        td_press(2103, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(0, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_press(OUTPUT_KEY, 40),
        td_press(INTERRUPTING_KEY, 40),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_Interrupt_A_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_press(INTERRUPTING_KEY, 40),
    };
    test_A_TDK_Interrupt_A_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 20),
        td_press(2103, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(0, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_press(INTERRUPTING_KEY, 40),
    };
    test_A_TDK_Interrupt_A_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_Interrupt_A_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_press(INTERRUPTING_KEY, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_press(INTERRUPTING_KEY, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_press(INTERRUPTING_KEY, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_Interrupt_A_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}

//

void test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210),
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 20),
        td_press(2103, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(0, 210),
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210),
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_Interrupt_A_HOLD_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210),
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 20),
        td_press(2103, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(0, 210),
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210),
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_press(INTERRUPTING_KEY, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 210)
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_press(INTERRUPTING_KEY, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 210)
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_Interrupt_A_HOLD_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_press(INTERRUPTING_KEY, 20),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 210)
    };
    test_A_TDK_Interrupt_A_HOLD_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}

//

void test_A_TDK_A_Interrupt_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(OUTPUT_KEY, 40),
        td_press(INTERRUPTING_KEY, 40),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 30),
        td_press(2103, 30),
        td_layer(0, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(OUTPUT_KEY, 40),
        td_press(INTERRUPTING_KEY, 40),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_Interrupt_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 40),
    };
    test_A_TDK_A_Interrupt_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 30),
        td_press(2103, 30),
        td_layer(0, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 40),
    };
    test_A_TDK_A_Interrupt_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_Interrupt_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 30),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 30),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 30),
        td_release(OUTPUT_KEY, 40)
    };
    test_A_TDK_A_Interrupt_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}

//

void test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, strategy)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 30),
        td_press(2103, 30),
        td_layer(0, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_Interrupt_HOLD_TDK_With_1Hold(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, strategy)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Hold_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Hold(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Hold_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 30),
        td_press(2103, 30),
        td_layer(0, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Hold(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Hold_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Hold(TAP_DANCE_BALANCED, expected_events);
}

void test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {}, 200, 200, strategy)  // Only tap action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Tap_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 30),
        td_release(OUTPUT_KEY, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Tap_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 30),
        td_release(OUTPUT_KEY, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceRollOverPreviousPress, A_TDK_A_Interrupt_HOLD_TDK_With_1Tap_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 10),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(INTERRUPTING_KEY, 30),
        td_release(OUTPUT_KEY, 210)
    };
    test_A_TDK_A_Interrupt_HOLD_TDK_With_1Tap(TAP_DANCE_BALANCED, expected_events);
}