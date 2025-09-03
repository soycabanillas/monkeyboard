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

class TapDanceComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

// ==================== BASIC TAP FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicSingleTap) {
    const uint16_t TAP_DANCE_KEY = 2000;
    const uint16_t OUTPUT_KEY = 2001;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}})
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 0);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 0), td_release(OUTPUT_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceComprehensiveTest, KeyRepetitionException) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {
        { { TAP_DANCE_KEY, 3010 }, { 3011, 3012 } }
    ,
        { { 3020, 3021 }, { 3022, 3023 } }
    };

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // First tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 0);

    // Second tap
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);

    // Third tap
    keyboard.press_key_at(TAP_DANCE_KEY, 200);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 0), td_release(OUTPUT_KEY, 0),
        td_press(OUTPUT_KEY, 100), td_release(OUTPUT_KEY, 100),
        td_press(OUTPUT_KEY, 200), td_release(OUTPUT_KEY, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceComprehensiveTest, NoActionConfigured) {
    const uint16_t NORMAL_KEY = 4000;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        { NORMAL_KEY }
    }};

    TestScenario scenario(keymap);
    // No tap dance configuration - empty config
    TapDanceConfigBuilder config_builder;
    config_builder.add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(NORMAL_KEY, 0);
    keyboard.release_key_at(NORMAL_KEY, 0);
    keyboard.wait_ms(250);

    // Should only have the original key press/release, no tap dance actions
    std::vector<tap_dance_event_t> expected_events = {
        td_press(NORMAL_KEY, 0), td_release(NORMAL_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {}; // No layer changes
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// ==================== BASIC HOLD FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicHoldTimeout) {
    const uint16_t TAP_DANCE_KEY = 5000;
    const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {
        { { TAP_DANCE_KEY, 5010 }, { 5011, 5012 } }
    ,
        { { 5020, 5021 }, { 5022, 5023 } }
    };

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    // Only hold action on position 1, no tap action
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.wait_ms(250);  // Wait for hold timeout
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(BASE_LAYER, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {TARGET_LAYER, BASE_LAYER};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

TEST_F(TapDanceComprehensiveTest, HoldReleasedBeforeTimeout) {
    const uint16_t TAP_DANCE_KEY = 6000;
    const uint16_t OUTPUT_KEY = 6001;
    const uint8_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {
        { { TAP_DANCE_KEY, 6010 }, { 6011, 6012 } }
    ,
        { { 6020, 6021 }, { 6022, 6023 } }
    };

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);   // Press key
    keyboard.release_key_at(TAP_DANCE_KEY, 100); // Release before timeout

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 100), td_release(OUTPUT_KEY, 100)         // Tap output
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// ==================== MULTI-TAP SEQUENCES ====================

TEST_F(TapDanceComprehensiveTest, DoubleTap) {
    const uint16_t TAP_DANCE_KEY = 7000;
    const uint16_t SINGLE_TAP_KEY = 7001;
    const uint16_t DOUBLE_TAP_KEY = 7011;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {
        { { TAP_DANCE_KEY, 7010 }, { 7012, 7013 } }
    ,
        { { 7020, 7021 }, { 7022, 7023 } }
    };

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, SINGLE_TAP_KEY}, {2, DOUBLE_TAP_KEY}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // First tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 0);
    // Should wait for potential second tap, no tap output yet
    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    // Second tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.wait_ms(250);  // Wait for timeout

    expected_events = {
        td_press(DOUBLE_TAP_KEY, 0), td_release(DOUBLE_TAP_KEY, 50)   // Double tap output
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceComprehensiveTest, TripleTap) {
    const uint16_t TAP_DANCE_KEY = 8000;
    const uint16_t SINGLE_TAP_KEY = 8001;
    const uint16_t DOUBLE_TAP_KEY = 8011;
    const uint16_t TRIPLE_TAP_KEY = 8012;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {
        { { TAP_DANCE_KEY, 8010 }, { 8013, 8014 } }
    ,
        { { 8020, 8021 }, { 8022, 8023 } }
    };

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, SINGLE_TAP_KEY}, {2, DOUBLE_TAP_KEY}, {3, TRIPLE_TAP_KEY}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 50);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 150);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);
    keyboard.wait_ms(250);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(TRIPLE_TAP_KEY, 150), td_release(TRIPLE_TAP_KEY, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceComprehensiveTest, TapCountExceedsConfiguration) {
    const uint16_t TAP_DANCE_KEY = 9000;
    const uint16_t SINGLE_TAP_KEY = 9001;
    const uint16_t DOUBLE_TAP_KEY = 9011;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {
        { { TAP_DANCE_KEY, 9010 }, { 9012, 9013 } }
    ,
        { { 9020, 9021 }, { 9022, 9023 } }
    };

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, SINGLE_TAP_KEY}, {2, DOUBLE_TAP_KEY}}, {}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Three taps (exceeds configuration)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 50);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 150);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);

    keyboard.wait_ms(250);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(DOUBLE_TAP_KEY, 50), td_release(DOUBLE_TAP_KEY, 100),
        td_press(SINGLE_TAP_KEY, 400), td_release(SINGLE_TAP_KEY, 400)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
