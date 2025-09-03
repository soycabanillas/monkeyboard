#include <sys/wait.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "common_functions.hpp"
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

const uint16_t PREVIOUS_KEY_A = 2000;
const uint16_t PREVIOUS_KEY_B = 2001;
const uint16_t TAP_DANCE_KEY = 2002;
const uint16_t OUTPUT_KEY = 2003;
const uint16_t INTERRUPTING_KEY = 2004;

TEST_F(TapDanceRollOverPreviousPress, 1Tap1Hold_A_TDK_A_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_press(OUTPUT_KEY, 30),
        td_release(OUTPUT_KEY, 30),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, 1Tap1Hold_A_TDK_A_HOLD_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, 1Hold_A_TDK_A_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, 1Hold_A_TDK_HOLD_A_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, 1Hold_A_TDK_A_HOLD_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, 1Hold_A_TDK_Interrupt_A_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.press_key_at(INTERRUPTING_KEY, 20);
    keyboard.release_key_at(PREVIOUS_KEY_A, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 40);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
        td_press(INTERRUPTING_KEY, 40),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceRollOverPreviousPress, 1Hold_A_TDK_A_Interrupt_TDK_TAP_PREFERRED) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }
    }, {
        { 2100, 2101, 2102, 2103 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, 1}}, 200, 200, TAP_DANCE_TAP_PREFERRED)  // Only hold action
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.press_key_at(INTERRUPTING_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 20),
        td_layer(1, 210),
        td_press(2103, 210),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}