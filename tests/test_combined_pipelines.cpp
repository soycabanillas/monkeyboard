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
#include "combo_test_helpers.hpp"
#include "tap_dance_test_helpers.hpp"

extern "C" {
#include "pipeline_combo.h"
#include "pipeline_combo_initializer.h"
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class CombinedPipelinesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

// Test combo and tap dance pipelines working together
// The combo pipeline processes first (physical), then tap dance (physical)
// They share a key position - when combo triggers, it outputs a key that tap dance processes
TEST_F(CombinedPipelinesTest, ComboOutputToTapDance) {
    const platform_keycode_t COMBO_KEY_A = 3000;
    const platform_keycode_t COMBO_KEY_B = 3001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 3002;  // Combo outputs this key
    const platform_keycode_t TAP_DANCE_OUTPUT = 3003;  // Tap dance final output
    const platform_keycode_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 3010, 3011 }
    }, {
        { 3020, 3021, 3022, 3023 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline (processed first as physical pipeline 0)
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline (processed second as physical pipeline 1)
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_OUTPUT_KEY, {{1, TAP_DANCE_OUTPUT}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press both combo keys and hold them for longer than combo timeout (50ms)
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 10);
    // Hold both keys for at least 50ms to activate combo
    keyboard.release_key_at(COMBO_KEY_A, 70);   // Released after combo timeout
    keyboard.release_key_at(COMBO_KEY_B, 80);

    // The combo should output COMBO_OUTPUT_KEY, which tap dance processes as a quick tap
    std::vector<event_t> expected_events = {
        td_press(TAP_DANCE_OUTPUT, 80),
        td_release(TAP_DANCE_OUTPUT, 80)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test combo output triggering tap dance hold action
TEST_F(CombinedPipelinesTest, ComboOutputToTapDanceHold) {
    const platform_keycode_t COMBO_KEY_A = 4000;
    const platform_keycode_t COMBO_KEY_B = 4001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 4002;  // Combo outputs this key
    const platform_keycode_t TAP_DANCE_OUTPUT = 4003;  // Tap dance tap output
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 4010, 4011 }
    }, {
        { 4020, 4021, 4022, 4023 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline with shorter timeout for testing
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_OUTPUT_KEY, {{1, TAP_DANCE_OUTPUT}}, {{1, TARGET_LAYER}}, 100, 100, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press both combo keys and hold them long enough for both combo activation and tap dance hold
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 10);
    // Hold long enough for combo activation (>50ms) + tap dance hold timeout (100ms)
    keyboard.release_key_at(COMBO_KEY_A, 200);  // Total 200ms hold
    keyboard.release_key_at(COMBO_KEY_B, 210);

    // The combo should output COMBO_OUTPUT_KEY, which tap dance processes as a hold
    // Combo activates at ~60ms (50ms after second key), tap dance hold timeout at 160ms (100ms after combo activation)
    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER, 160),  // Hold timeout reached at 60 + 100ms
        td_layer(0, 210)              // Released when last combo key released
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test normal key that doesn't trigger combo but does trigger tap dance
TEST_F(CombinedPipelinesTest, SingleKeyBypassesComboTriggerseTapDance) {
    const platform_keycode_t COMBO_KEY_A = 5000;
    const platform_keycode_t COMBO_KEY_B = 5001;
    const platform_keycode_t NORMAL_TAP_DANCE_KEY = 5002;  // Not part of combo, but has tap dance
    const platform_keycode_t COMBO_OUTPUT_KEY = 5003;
    const platform_keycode_t TAP_DANCE_OUTPUT = 5004;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, NORMAL_TAP_DANCE_KEY, 5010 }
    }, {
        { 5020, 5021, 5022, 5023 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline (only applies to positions 0,0 and 0,1)
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline for the normal key
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(NORMAL_TAP_DANCE_KEY, {{1, TAP_DANCE_OUTPUT}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press the normal key (not part of combo)
    keyboard.press_key_at(NORMAL_TAP_DANCE_KEY, 0);
    keyboard.release_key_at(NORMAL_TAP_DANCE_KEY, 150);

    // Should pass through combo unchanged and trigger tap dance
    std::vector<event_t> expected_events = {
        td_press(TAP_DANCE_OUTPUT, 150),
        td_release(TAP_DANCE_OUTPUT, 150)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test that when combo timeout is reached, individual keys still trigger tap dance
TEST_F(CombinedPipelinesTest, ComboTimeoutTriggersTapDanceOnIndividualKeys) {
    const platform_keycode_t COMBO_KEY_A = 6000;
    const platform_keycode_t COMBO_KEY_B = 6001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 6002;
    const platform_keycode_t TAP_DANCE_OUTPUT_A = 6003;
    const platform_keycode_t TAP_DANCE_OUTPUT_B = 6004;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 6010, 6011 }
    }, {
        { 6020, 6021, 6022, 6023 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline for both keys individually
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_KEY_A, {{1, TAP_DANCE_OUTPUT_A}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_tap_hold(COMBO_KEY_B, {{1, TAP_DANCE_OUTPUT_B}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press first key, release quickly (before combo can form), wait, then press second key
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.release_key_at(COMBO_KEY_A, 20);    // Released before combo timeout (50ms)
    keyboard.wait_ms(100);                       // Wait to ensure combo timeout has passed
    keyboard.press_key_at(COMBO_KEY_B, 150);
    keyboard.release_key_at(COMBO_KEY_B, 170);   // Quick tap

    // Should get two separate tap dance outputs, no combo
    std::vector<event_t> expected_events = {
        td_press(TAP_DANCE_OUTPUT_A, 20),   // First key triggers tap dance
        td_release(TAP_DANCE_OUTPUT_A, 20),
        td_press(TAP_DANCE_OUTPUT_B, 170),  // Second key triggers tap dance
        td_release(TAP_DANCE_OUTPUT_B, 170)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test combo timeout with hold on first key, tap on second
TEST_F(CombinedPipelinesTest, ComboTimeoutFirstKeyHoldSecondKeyTap) {
    const platform_keycode_t COMBO_KEY_A = 7000;
    const platform_keycode_t COMBO_KEY_B = 7001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 7002;
    const platform_keycode_t TAP_DANCE_OUTPUT_A = 7003;
    const platform_keycode_t TAP_DANCE_OUTPUT_B = 7004;
    const uint8_t TARGET_LAYER = 1;
    
    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 7010, 7011 }
    }, {
        { 7020, 7021, 7022, 7023 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline for both keys
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_KEY_A, {{1, TAP_DANCE_OUTPUT_A}}, {{1, TARGET_LAYER}}, 150, 150, TAP_DANCE_HOLD_PREFERRED)
        .add_tap_hold(COMBO_KEY_B, {{1, TAP_DANCE_OUTPUT_B}}, {{1, TARGET_LAYER}}, 150, 150, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press first key and hold it long enough to trigger tap dance hold
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.release_key_at(COMBO_KEY_A, 250);   // Hold past tap dance timeout
    
    // Then press second key after combo timeout would have expired
    keyboard.press_key_at(COMBO_KEY_B, 400);
    keyboard.release_key_at(COMBO_KEY_B, 450);   // Quick tap

    // Should get hold action for first key and tap action for second key
    std::vector<event_t> expected_events = {
        td_layer(TARGET_LAYER, 150),         // First key hold action
        td_layer(0, 250),                    // First key released
        td_press(TAP_DANCE_OUTPUT_B, 450),   // Second key tap action
        td_release(TAP_DANCE_OUTPUT_B, 450)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test rapid sequence where combo timeout prevents combo but allows tap dance
TEST_F(CombinedPipelinesTest, RapidSequenceComboTimeoutTapDanceStillWorks) {
    const platform_keycode_t COMBO_KEY_A = 8000;
    const platform_keycode_t COMBO_KEY_B = 8001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 8002;
    const platform_keycode_t TAP_DANCE_OUTPUT_A = 8003;
    const platform_keycode_t TAP_DANCE_OUTPUT_B = 8004;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 8010, 8011 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline for both keys with quick response
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_KEY_A, {{1, TAP_DANCE_OUTPUT_A}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_tap_hold(COMBO_KEY_B, {{1, TAP_DANCE_OUTPUT_B}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Rapid sequence that doesn't meet combo timing requirements
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.release_key_at(COMBO_KEY_A, 10);    // Quick release
    keyboard.press_key_at(COMBO_KEY_B, 100);     // Press second key after combo timeout would have expired
    keyboard.release_key_at(COMBO_KEY_B, 110);   // Quick release

    // Should get individual tap dance actions, no combo
    std::vector<event_t> expected_events = {
        td_press(TAP_DANCE_OUTPUT_A, 10),    // First key tap
        td_release(TAP_DANCE_OUTPUT_A, 10),
        td_press(TAP_DANCE_OUTPUT_B, 110),   // Second key tap  
        td_release(TAP_DANCE_OUTPUT_B, 110)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test overlapping keys where combo fails due to timing but tap dance succeeds
TEST_F(CombinedPipelinesTest, OverlappingKeysComboFailsTapDanceSucceeds) {
    const platform_keycode_t COMBO_KEY_A = 9000;
    const platform_keycode_t COMBO_KEY_B = 9001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 9002;
    const platform_keycode_t TAP_DANCE_OUTPUT_A = 9003;
    const platform_keycode_t TAP_DANCE_OUTPUT_B = 9004;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 9010, 9011 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline for both keys
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_KEY_A, {{1, TAP_DANCE_OUTPUT_A}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_tap_hold(COMBO_KEY_B, {{1, TAP_DANCE_OUTPUT_B}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press A, release A quickly, then press B while A is released
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.release_key_at(COMBO_KEY_A, 30);    // Released before 50ms combo timeout
    keyboard.press_key_at(COMBO_KEY_B, 35);      // Pressed after A released, no overlap for combo
    keyboard.release_key_at(COMBO_KEY_B, 55);

    // Combo should fail due to no simultaneous press, but both keys should trigger tap dance
    std::vector<event_t> expected_events = {
        td_press(TAP_DANCE_OUTPUT_A, 30),   // A triggers tap dance
        td_release(TAP_DANCE_OUTPUT_A, 30),
        td_press(TAP_DANCE_OUTPUT_B, 55),   // B triggers tap dance
        td_release(TAP_DANCE_OUTPUT_B, 55)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

// Test successful combo activation with proper timing
TEST_F(CombinedPipelinesTest, SuccessfulComboActivationWithProperTiming) {
    const platform_keycode_t COMBO_KEY_A = 10000;
    const platform_keycode_t COMBO_KEY_B = 10001;
    const platform_keycode_t COMBO_OUTPUT_KEY = 10002;
    const platform_keycode_t TAP_DANCE_OUTPUT = 10003;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { COMBO_KEY_A, COMBO_KEY_B, 10010, 10011 }
    }};

    TestScenario scenario(keymap);
    
    // Configure combo pipeline
    ComboConfigBuilder combo_builder;
    combo_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,0}, {0,1}}, COMBO_OUTPUT_KEY)
        .add_to_scenario(scenario);
    
    // Configure tap dance pipeline for combo output
    TapDanceConfigBuilder tap_dance_builder;
    tap_dance_builder
        .add_tap_hold(COMBO_OUTPUT_KEY, {{1, TAP_DANCE_OUTPUT}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Press both keys with proper timing for combo activation
    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 10);      // Both keys pressed within 50ms window
    keyboard.wait_ms(60);                        // Wait for combo to activate (>50ms)
    keyboard.release_key_at(COMBO_KEY_A, 70);
    keyboard.release_key_at(COMBO_KEY_B, 80);

    // Should get combo activation followed by tap dance
    std::vector<event_t> expected_events = {
        td_press(TAP_DANCE_OUTPUT, 80),   // Combo output triggers tap dance
        td_release(TAP_DANCE_OUTPUT, 80)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}
