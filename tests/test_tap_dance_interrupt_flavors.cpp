#include <sys/types.h>
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

class InterruptFlavorsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
const uint16_t KEY_B = 3010;          // KC_B
const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

// Test Case 1: AABB sequence - Press A, release A, press B, release B
// Sequence: LSFT_T(KC_A) down, LSFT_T(KC_A) up, KC_B down, KC_B up
// All actions happen before hold timeout
// Expected: All flavors should produce tap (KC_A) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);
    keyboard.press_key_at(KEY_B, 210);
    keyboard.release_key_at(KEY_B, 220);

    // Should produce tap (KC_A) then KC_B
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),
        td_release(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 210),
        td_release(KEY_B, 220)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);
    keyboard.press_key_at(KEY_B, 210);
    keyboard.release_key_at(KEY_B, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),
        td_release(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 210),
        td_release(KEY_B, 220)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);
    keyboard.press_key_at(KEY_B, 210);
    keyboard.release_key_at(KEY_B, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),
        td_release(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 210),
        td_release(KEY_B, 220)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test Case 2: AABB sequence - Hold A past timeout, then press B
// Sequence: LSFT_T(KC_A) down, wait past hold timeout, LSFT_T(KC_A) up, KC_B down, KC_B up
// Expected: All flavors should produce hold (shift layer) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 201);
    keyboard.press_key_at(KEY_B, 205);
    keyboard.release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_layer(0, 201),
        td_press(KEY_B, 205),
        td_release(KEY_B, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 201);
    keyboard.press_key_at(KEY_B, 205);
    keyboard.release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_layer(0, 201),
        td_press(KEY_B, 205),
        td_release(KEY_B, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 201);
    keyboard.press_key_at(KEY_B, 205);
    keyboard.release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_layer(0, 201),
        td_press(KEY_B, 205),
        td_release(KEY_B, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test Case 3: ABBA sequence - Press A, press B, release B, release A (all before hold timeout)
// Sequence: LSFT_T(KC_A) down, KC_B down, KC_B up, LSFT_T(KC_A) up
// All actions happen before hold timeout
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(KEY_B, 120);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 199),
        td_release(KEY_B, 199),
        td_release(OUTPUT_KEY_A, 199)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(KEY_B, 120);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 120),
        td_press(3012, 120),
        td_release(3012, 120),
        td_layer(0, 199)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(KEY_B, 120);
    keyboard.release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_release(3012, 120),
        td_layer(0, 199)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test Case 4: ABBA sequence - Press A, press B, release B, wait for timeout, release A
// Sequence: LSFT_T(KC_A) down, KC_B down, KC_B up, wait for hold timeout, LSFT_T(KC_A) up
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(KEY_B, 120);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    // TAP_PREFERRED: Should produce tap (KC_A) when interrupted, even with timeout after
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 200),
        td_release(3012, 200),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(KEY_B, 120);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    // BALANCED: Should produce hold (shift layer) when timeout is reached
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 120),
        td_press(3012, 120),
        td_release(3012, 120),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(KEY_B, 120);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    // HOLD_PREFERRED: Should produce hold (shift layer) when interrupted
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_release(3012, 120),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test Case 5: ABBA sequence - Press A, reach hold timeout, press B, release B, release A
// Sequence: LSFT_T(KC_A) down, wait past hold timeout, KC_B down, KC_B up, LSFT_T(KC_A) up
// Expected: All flavors should produce hold (shift layer) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 205);
    keyboard.release_key_at(KEY_B, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 205),
        td_release(3012, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 205);
    keyboard.release_key_at(KEY_B, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 205),
        td_release(3012, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 205);
    keyboard.release_key_at(KEY_B, 210);
    keyboard.release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 205),
        td_release(3012, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test Case 6: ABAB sequence - Press A, press B, release A, release B (before hold timeout)
// Sequence: LSFT_T(KC_A) down, KC_B down, LSFT_T(KC_A) up, KC_B up
// All actions happen before hold timeout (based on the first table in the image)
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(TAP_DANCE_KEY, 130);
    keyboard.release_key_at(KEY_B, 140);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 130),
        td_press(KEY_B, 130),
        td_release(OUTPUT_KEY_A, 130),
        td_release(KEY_B, 140)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(TAP_DANCE_KEY, 130);
    keyboard.release_key_at(KEY_B, 140);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 130),
        td_press(KEY_B, 130),
        td_release(OUTPUT_KEY_A, 130),
        td_release(KEY_B, 140)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(TAP_DANCE_KEY, 130);
    keyboard.release_key_at(KEY_B, 140);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_layer(0, 130),
        td_release(3012, 140)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test Case 7: ABAB sequence - Press A, press B, hold timeout reached, release A, release B
// Sequence: LSFT_T(KC_A) down, KC_B down, LSFT_T(KC_A) held, LSFT_T(KC_A) up, KC_B up
// Hold timeout is reached before A is released
// Expected: All flavors should produce "B" - hold action with B on shift layer

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_TapPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_TAP_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(TAP_DANCE_KEY, 205);
    keyboard.release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 200),
        td_layer(0, 205),
        td_release(3012, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_Balanced) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_BALANCED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(TAP_DANCE_KEY, 205);
    keyboard.release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 200),
        td_layer(0, 205),
        td_release(3012, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_HoldPreferred) {
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY, KEY_B }
    }, {
        { 3011, 3012 }  // Shift layer
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY_A}}, {{1, TARGET_LAYER_SHIFT}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(KEY_B, 110);
    keyboard.release_key_at(TAP_DANCE_KEY, 205);
    keyboard.release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_layer(0, 205),
        td_release(3012, 210)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}
