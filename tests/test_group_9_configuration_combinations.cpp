#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "common_functions.hpp"
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class ConfigurationCombinationsTest : public ::testing::Test {
protected:
    pipeline_tap_dance_global_config_t* tap_dance_config;

    void SetUp() override {
        reset_mock_state();
        pipeline_tap_dance_global_state_create();

        size_t n_elements = 10;
        tap_dance_config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*tap_dance_config)));
        tap_dance_config->length = 0;
        tap_dance_config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

        pipeline_executor_create_config(1, 0);
        pipeline_executor_add_physical_pipeline(0, &pipeline_tap_dance_callback_process_data_executor, &pipeline_tap_dance_callback_reset_executor, tap_dance_config);
    }

    void TearDown() override {
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
        if (tap_dance_config) {
            free(tap_dance_config);
            tap_dance_config = nullptr;
        }
    }
};

// Test 9.1: Tap-Only Configuration
// Objective: Verify behavior when only tap actions are configured (no hold actions)
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
// Hold actions: [] // No hold actions configured
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ConfigurationCombinationsTest, TapOnlyConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Only tap actions, no hold actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Single Tap - immediate execution
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 50)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    reset_mock_state();

    // Multi-Tap Sequence - immediate execution
    keyboard.press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    keyboard.press_key_at(TAP_DANCE_KEY, 70);     // t=70ms
    keyboard.release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    keyboard.press_key_at(TAP_DANCE_KEY, 160);    // t=160ms
    keyboard.release_key_at(TAP_DANCE_KEY, 210);  // t=210ms

    std::vector<tap_dance_event_t> expected_multi_events = {
        td_press(3003, 160), td_release(3003, 210)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_multi_events));
}

// Test 9.2: Hold-Only Configuration
// Objective: Verify behavior when only hold actions are configured (no tap actions)
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [] // No tap actions configured
// Hold actions: [1: CHANGELAYERTEMPO(1), 2: CHANGELAYERTEMPO(2)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ConfigurationCombinationsTest, HoldOnlyConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Only hold actions, no tap actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Tap Attempt (No Action)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 50);   // t=50ms
    keyboard.wait_ms(200);                    // t=250ms

    std::vector<tap_dance_event_t> expected_events = {};  // No output (no tap action configured)
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    reset_mock_state();

    // Hold Execution
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.wait_ms(250);                      // t=250ms
    keyboard.release_key_at(TAP_DANCE_KEY, 250);    // t=250ms

    std::vector<uint8_t> expected_layers = {1, 0};  // Layer 1 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_hold_events = {
        td_layer(1, 200), td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_hold_events));
}

// Sparse Configuration - Gaps in Tap Counts
// Objective: Verify behavior with non-sequential tap count configurations
TEST_F(ConfigurationCombinationsTest, SparseConfigurationTapNothingTap) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(3, 3003)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First Tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.wait_ms(200);

    std::vector<tap_dance_event_t> expected_events_1 = {
        td_press(3001, 250), td_release(3001, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_1));

    reset_mock_state();

    // Second Tap (no action configured)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);
    keyboard.wait_ms(200);

    std::vector<tap_dance_event_t> expected_events_2 = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_2));

    reset_mock_state();

    // Third Tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.release_key_at(TAP_DANCE_KEY, 150);
    keyboard.press_key_at(TAP_DANCE_KEY, 200);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events_3 = {
        td_press(3003, 200), td_release(3003, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_3));
}

// Test 9.4: Custom Timeout Configuration
// Objective: Verify behavior with non-default timeout values
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)]
// Hold actions: [1: CHANGELAYERTEMPO(1)]
// Hold timeout: 100ms, Tap timeout: 300ms // Custom timeouts
TEST_F(ConfigurationCombinationsTest, CustomTimeoutConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 100; // Set hold timeout to 100ms
    tap_dance_behavior->config->tap_timeout = 300; // Set tap timeout to 300ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Short Hold Timeout
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.wait_ms(150);                      // t=150ms (exceed 100ms timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 150);    // t=150ms

    std::vector<uint8_t> expected_layers_1 = {1, 0};  // Layer 1 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers_1));

    reset_mock_state();

    // Long Tap Timeout
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 50);     // t=50ms
    keyboard.wait_ms(300);                      // t=350ms (300ms tap timeout)
    std::vector<tap_dance_event_t> expected_events_1 = {
        td_press(3001, 350), td_release(3001, 350)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_1));

    reset_mock_state();

    // Sequence Continuation with Long Tap Timeout
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 50);     // t=50ms
    keyboard.press_key_at(TAP_DANCE_KEY, 299);      // t=349ms (1ms before tap timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 399);    // t=399ms
    keyboard.wait_ms(300);                      // t=699ms

    std::vector<tap_dance_event_t> expected_events_2 = {
        td_press(3001, 699), td_release(3001, 699)  // single tap, sequence continued
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_2));
}

// Test 9.5: Asymmetric Timeout Configuration
// Objective: Verify behavior when hold timeout > tap timeout
// Configuration: Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
// Hold actions: [1: CHANGELAYERTEMPO(1)]
// Hold timeout: 300ms, Tap timeout: 150ms // Hold > Tap
TEST_F(ConfigurationCombinationsTest, AsymmetricTimeoutConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 300; // Set hold timeout to 300ms
    tap_dance_behavior->config->tap_timeout = 150; // Set tap timeout to 150ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Tap Timeout Before Hold Timeout
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    keyboard.wait_ms(150);                      // t=250ms (tap timeout)
    std::vector<tap_dance_event_t> expected_events_1 = {
        td_press(3001, 250), td_release(3001, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_1));

    reset_mock_state();

    // Hold Timeout After Release
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.wait_ms(350);                      // t=350ms (exceed 300ms hold timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 350);    // t=350ms

    std::vector<uint8_t> expected_layers_2 = {TARGET_LAYER, 0};  // Layer 1 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers_2));
}

// Test 9.6: Strategy Variation per Configuration
// Objective: Verify different strategies work with various configurations
// Base Configuration: Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYERTEMPO(1)]
TEST_F(ConfigurationCombinationsTest, StrategyVariation) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3001;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][2][1] = {{{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_TAP_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // TAP_PREFERRED with Configuration
    {
        // Input: Trigger press, interrupt press+release, trigger release (before timeout)
        keyboard.press_key_at(TAP_DANCE_KEY, 0);          // t=0ms
        keyboard.press_key_at(INTERRUPTING_KEY, 50);      // t=50ms
        keyboard.release_key_at(INTERRUPTING_KEY, 100);   // t=100ms
        keyboard.release_key_at(TAP_DANCE_KEY, 150);      // t=150ms

        // Expected: Tap action (interruption ignored)
        std::vector<tap_dance_event_t> expected_events = {
            td_press(INTERRUPTING_KEY, 50), td_release(INTERRUPTING_KEY, 100),
            td_press(3001, 350), td_release(3001, 350)
        };
        EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
    }

    reset_mock_state();
    tap_dance_config->length = 0;

    // BALANCED with Configuration
    {
        pipeline_tap_dance_action_config_t* actions_balanced[] = {
            createbehaviouraction_tap(1, 3001),
            createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_BALANCED)
        };
        pipeline_tap_dance_behaviour_t* tap_dance_behavior_balanced = createbehaviour(TAP_DANCE_KEY, actions_balanced, 2);
        tap_dance_behavior_balanced->config->hold_timeout = 200; // Set hold timeout to 200ms
        tap_dance_behavior_balanced->config->tap_timeout = 200; // Set tap timeout to 200ms
        tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior_balanced;
        tap_dance_config->length++;

        // Input: Trigger press, interrupt press+release, trigger release
        keyboard.press_key_at(TAP_DANCE_KEY, 0);          // t=0ms
        keyboard.press_key_at(INTERRUPTING_KEY, 50);      // t=50ms
        keyboard.release_key_at(INTERRUPTING_KEY, 100);   // t=100ms
        keyboard.release_key_at(TAP_DANCE_KEY, 150);      // t=150ms

        // Expected: Hold action (complete cycle)
        std::vector<tap_dance_event_t> expected_events = {
            td_press(INTERRUPTING_KEY, 50), td_release(INTERRUPTING_KEY, 100),
            td_layer(1, 100), td_layer(0, 150)
        };
        EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

        std::vector<uint8_t> expected_layers = {1, 0};  // Layer 1 activation/deactivation
        EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
    }

    reset_mock_state();
    tap_dance_config->length = 0;

    // HOLD_PREFERRED with Configuration
    {
        pipeline_tap_dance_action_config_t* actions_hold[] = {
            createbehaviouraction_tap(1, 3001),
            createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
        };
        pipeline_tap_dance_behaviour_t* tap_dance_behavior_hold = createbehaviour(TAP_DANCE_KEY, actions_hold, 2);
        tap_dance_behavior_hold->config->hold_timeout = 200; // Set hold timeout to 200ms
        tap_dance_behavior_hold->config->tap_timeout = 200; // Set tap timeout to 200ms
        tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior_hold;
        tap_dance_config->length++;

        // Input: Trigger press, interrupt press, trigger release
        keyboard.press_key_at(TAP_DANCE_KEY, 0);          // t=0ms
        keyboard.press_key_at(INTERRUPTING_KEY, 50);      // t=50ms
        keyboard.release_key_at(INTERRUPTING_KEY, 100);   // t=100ms
        keyboard.release_key_at(TAP_DANCE_KEY, 150);      // t=150ms

        // Expected: Hold action (immediate on interrupt)
        std::vector<tap_dance_event_t> expected_events = {
            td_press(INTERRUPTING_KEY, 50), td_release(INTERRUPTING_KEY, 100),
            td_layer(1, 50), td_layer(0, 150)
        };
        EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

        std::vector<uint8_t> expected_layers = {1, 0};  // Layer 1 activation/deactivation
        EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
    }
}

// Test 9.7: Maximum Configuration Complexity
// Objective: Verify system handles maximum practical configuration complexity
// Configuration: TAP_DANCE_KEY = 3000, Strategy: BALANCED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003),
//               4: SENDKEY(3004), 5: SENDKEY(3005)]
// Hold actions: [1: CHANGELAYERTEMPO(1), 2: CHANGELAYERTEMPO(2),
//                3: CHANGELAYERTEMPO(3), 4: CHANGELAYERTEMPO(4), 5: CHANGELAYERTEMPO(5)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ConfigurationCombinationsTest, MaximumConfigurationComplexity) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_tap(4, 3004),
        createbehaviouraction_tap(5, 3005),
        createbehaviouraction_hold(5, 5, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 6);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Attempt to reach 5th tap count
    for (int i = 0; i < 5; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 30);
        keyboard.release_key_at(TAP_DANCE_KEY, i * 30 + 10);
        keyboard.wait_ms(20);
    }
    keyboard.wait_ms(200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3005, 350), td_release(3005, 350)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    reset_mock_state();

    // Then test 5th hold action
    for (int i = 0; i < 4; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 30);
        keyboard.release_key_at(TAP_DANCE_KEY, i * 30 + 10);
        keyboard.wait_ms(20);
    }
    keyboard.press_key_at(TAP_DANCE_KEY, 120);
    keyboard.wait_ms(250);
    keyboard.release_key_at(TAP_DANCE_KEY, 370);

    std::vector<uint8_t> expected_layers = {5, 0};  // Layer 5 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 9.8: Minimal Configuration
// Objective: Verify system handles minimal valid configurations
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)] // Single tap action only
// Hold actions: [] // No hold actions
TEST_F(ConfigurationCombinationsTest, MinimalConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Single Valid Action
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 50)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    reset_mock_state();

    // Overflow from Minimal
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 70);     // 2nd tap - overflow
    keyboard.release_key_at(TAP_DANCE_KEY, 110);
    std::vector<tap_dance_event_t> expected_events_2 = {
        td_press(3001, 0), td_release(3001, 30),   // immediate, first tap
        td_press(3001, 70), td_release(3001, 110) // immediate, overflow
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_2));
}

// Test 9.9: Mixed Action Types Configuration
// Objective: Verify configurations with different action types at different tap counts
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
// Hold actions: [1: CHANGELAYERTEMPO(1), 3: CHANGELAYERTEMPO(3)] // Skip 2nd
TEST_F(ConfigurationCombinationsTest, MixedActionTypesConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(3, 3, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First Count (Both Available)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.wait_ms(200);
    std::vector<tap_dance_event_t> expected_events_1 = {
        td_press(3001, 250), td_release(3001, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_1));

    reset_mock_state();

    // Second Count (Tap Only)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 80);
    keyboard.release_key_at(TAP_DANCE_KEY, 130);
    keyboard.wait_ms(200);
    std::vector<tap_dance_event_t> expected_events_2 = {
        td_press(3002, 330), td_release(3002, 330)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_2));

    reset_mock_state();

    // Third Count (Hold Only)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 70);
    keyboard.release_key_at(TAP_DANCE_KEY, 110);
    keyboard.press_key_at(TAP_DANCE_KEY, 160);
    keyboard.wait_ms(250);
    keyboard.release_key_at(TAP_DANCE_KEY, 410);
    std::vector<uint8_t> expected_layers = {3, 0};  // Layer 3 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 9.10: Zero-Based vs One-Based Configuration
// Objective: Verify proper handling of tap count indexing
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Explicitly test that tap count 1 = first tap, count 2 = second tap, etc.
// Tap actions: [1: SENDKEY(0x31), 2: SENDKEY(0x32), 3: SENDKEY(0x33)] // '1', '2', '3'
// Hold actions: [], Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ConfigurationCombinationsTest, ZeroBasedVsOneBasedConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 0x31),  // '1' key
        createbehaviouraction_tap(2, 0x32),  // '2' key
        createbehaviouraction_tap(3, 0x33)   // '3' key
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First Tap (Count 1)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    std::vector<tap_dance_event_t> expected_events_1 = {
        td_press(0x31, 0), td_release(0x31, 50)  // '1' key
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_1));

    reset_mock_state();

    // Second Tap (Count 2)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 70);
    keyboard.release_key_at(TAP_DANCE_KEY, 120);
    std::vector<tap_dance_event_t> expected_events_2 = {
        td_press(0x32, 0), td_release(0x32, 120)  // '2' key
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_2));

    reset_mock_state();

    // Third Tap (Count 3)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 70);
    keyboard.release_key_at(TAP_DANCE_KEY, 110);
    keyboard.press_key_at(TAP_DANCE_KEY, 160);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);
    std::vector<tap_dance_event_t> expected_events_3 = {
        td_press(0x33, 0), td_release(0x33, 210)  // '3' key
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_3));
}

// Test 9.11: Configuration with Large Timeout Values
// Objective: Verify system handles large timeout values correctly
// Configuration: Hold timeout: 1000ms, Tap timeout: 2000ms
TEST_F(ConfigurationCombinationsTest, ConfigurationWithLargeTimeoutValues) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 1000; // Set hold timeout to 1000ms
    tap_dance_behavior->config->tap_timeout = 2000; // Set tap timeout to 2000ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.wait_ms(1100);                     // t=1100ms (exceed 1000ms timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 1100);   // t=1100ms

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 1000), td_layer(0, 1100)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 9.12: Configuration Edge Cases
// Objective: Verify handling of edge case configurations
TEST_F(ConfigurationCombinationsTest, ConfigurationEdgeCases) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Test with identical timeout values
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Press, release at 100ms, wait 200ms total
    // Clear precedence rules (tap timeout from release, hold from press)
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    keyboard.wait_ms(200);                      // t=300ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 300), td_release(3001, 300)  // Tap timeout from release
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 9.13: Configuration Consistency Verification
// Objective: Verify consistent behavior across different configurations
TEST_F(ConfigurationCombinationsTest, ConfigurationConsistencyVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Base Test Pattern: Single tap with 50ms hold

    // Tap-Only Config
    pipeline_tap_dance_action_config_t* actions_tap_only[] = {
        createbehaviouraction_tap(1, 3001)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior1 = createbehaviour(TAP_DANCE_KEY, actions_tap_only, 1);
    tap_dance_behavior1->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior1->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior1;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    std::vector<tap_dance_event_t> expected_immediate = {
        td_press(3001, 0), td_release(3001, 50)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_immediate));

    reset_mock_state();
    tap_dance_config->length = 0;

    // Hold-Only Config
    pipeline_tap_dance_action_config_t* actions_hold_only[] = {
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior2 = createbehaviour(TAP_DANCE_KEY, actions_hold_only, 1);
    tap_dance_behavior2->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior2->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior2;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.wait_ms(200);
    std::vector<tap_dance_event_t> expected_no_action = {};  // No action (no tap configured)
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_no_action));

    reset_mock_state();
    tap_dance_config->length = 0;

    // Mixed Config
    pipeline_tap_dance_action_config_t* actions_mixed[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior3 = createbehaviour(TAP_DANCE_KEY, actions_mixed, 2);
    tap_dance_behavior3->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior3->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior3;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.wait_ms(200);
    std::vector<tap_dance_event_t> expected_delayed = {
        td_press(3001, 250), td_release(3001, 250)  // Delayed tap execution
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_delayed));
}

// Test 9.14: Multi-Key Configuration Comparison
// Objective: Verify independent behavior of multiple configured keys
TEST_F(ConfigurationCombinationsTest, MultiKeyConfigurationComparison) {
    const uint16_t TAP_DANCE_KEY_1 = 3000;  // Tap-only configuration
    const uint16_t TAP_DANCE_KEY_2 = 3100;  // Hold-only configuration
    const uint16_t TAP_DANCE_KEY_3 = 3200;  // Mixed configuration

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY_1 }, { TAP_DANCE_KEY_2 }, { TAP_DANCE_KEY_3 }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 3, 1);

    // Configure three different keys with different behaviors
    pipeline_tap_dance_action_config_t* actions_key1[] = {
        createbehaviouraction_tap(1, 3001)  // Tap-only
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior1 = createbehaviour(TAP_DANCE_KEY_1, actions_key1, 1);
    tap_dance_behavior1->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior1->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior1;
    tap_dance_config->length++;

    pipeline_tap_dance_action_config_t* actions_key2[] = {
        createbehaviouraction_hold(1, 2, TAP_DANCE_HOLD_PREFERRED)  // Hold-only
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior2 = createbehaviour(TAP_DANCE_KEY_2, actions_key2, 1);
    tap_dance_behavior2->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior2->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior2;
    tap_dance_config->length++;

    pipeline_tap_dance_action_config_t* actions_key3[] = {
        createbehaviouraction_tap(1, 3003),
        createbehaviouraction_hold(1, 3, TAP_DANCE_HOLD_PREFERRED)  // Mixed
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior3 = createbehaviour(TAP_DANCE_KEY_3, actions_key3, 2);
    tap_dance_behavior3->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior3->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior3;
    tap_dance_config->length++;

    // Simultaneous activation of all three keys
    keyboard.press_key_at(TAP_DANCE_KEY_1, 0);      // t=0ms
    keyboard.press_key_at(TAP_DANCE_KEY_2, 10);     // t=10ms
    keyboard.press_key_at(TAP_DANCE_KEY_3, 20);     // t=20ms
    keyboard.wait_ms(250);                      // t=270ms
    keyboard.release_key_at(TAP_DANCE_KEY_1, 270);  // t=270ms
    keyboard.release_key_at(TAP_DANCE_KEY_2, 270);  // t=270ms
    keyboard.release_key_at(TAP_DANCE_KEY_3, 270);  // t=270ms

    // Expected Output:
    // Key 1 (tap-only) - immediate
    // Key 2 (hold-only) - layer activation at timeout
    // Key 3 (mixed) - layer activation at timeout
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 270),  // Key 1 immediate execution
        td_layer(2, 210), td_layer(0, 270),                     // Key 2 hold
        td_layer(3, 220), td_layer(0, 270)                      // Key 3 hold
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {2, 3, 3, 0, 2, 0};  // Key 2 and Key 3 layer changes
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

