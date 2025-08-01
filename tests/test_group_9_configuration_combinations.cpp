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
            malloc(sizeof(*tap_dance_config) + n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));
        tap_dance_config->length = 0;

        pipeline_executor_create_config(1, 0);
        pipeline_executor_add_physical_pipeline(0, &pipeline_tap_dance_callback_process_data, &pipeline_tap_dance_callback_reset, tap_dance_config);
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Only tap actions, no hold actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Single Tap - immediate execution
    tap_key(TAP_DANCE_KEY, 50);
    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 50)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    reset_mock_state();

    // Multi-Tap Sequence - immediate execution
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
    tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-210ms

    std::vector<key_action_t> expected_multi_keys = {
        press(3003, 160), release(3003, 210)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_multi_keys));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Only hold actions, no tap actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Tap Attempt (No Action)
    tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
    platform_wait_ms(200);          // t=250ms

    std::vector<key_action_t> expected_keys = {};  // No output (no tap action configured)
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

    reset_mock_state();

    // Hold Execution
    press_key(TAP_DANCE_KEY);        // t=0ms
    platform_wait_ms(250);          // t=250ms
    release_key(TAP_DANCE_KEY);      // t=250ms

    std::vector<uint8_t> expected_layers = {1, 0};  // Layer 1 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 9.3: Sparse Configuration - Gaps in Tap Counts
// Objective: Verify behavior with non-sequential tap count configurations
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 3: SENDKEY(3003)] // No 2nd tap action
// Hold actions: [2: CHANGELAYERTEMPO(2)] // Only 2nd tap has hold
TEST_F(ConfigurationCombinationsTest, SparseConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(3, 3003)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // First Tap (Configured)
    tap_key(TAP_DANCE_KEY, 50);
    std::vector<key_action_t> expected_keys_1 = {
        press(3001, 250), release(3001, 250)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_1));

    reset_mock_state();

    // Second Tap Hold (Configured)
    tap_key(TAP_DANCE_KEY, 30);
    press_key(TAP_DANCE_KEY, 50);    // t=80ms (2nd tap, hold)
    platform_wait_ms(250);          // t=330ms
    release_key(TAP_DANCE_KEY);      // t=330ms

    std::vector<uint8_t> expected_layers = {2, 0};  // Layer 2 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    reset_mock_state();

    // Second Tap Tap (No Action)
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-130ms (no tap action for 2nd)
    platform_wait_ms(200);          // t=330ms

    std::vector<key_action_t> expected_keys_3 = {};  // No output (no tap action for 2nd tap count)
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys_3));

    reset_mock_state();

    // Third Tap (Configured)
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-210ms
    platform_wait_ms(200);          // t=410ms

    std::vector<key_action_t> expected_keys_4 = {
        press(3003, 410), release(3003, 410)  // Immediate execution
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_4));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Short Hold Timeout
    press_key(TAP_DANCE_KEY);        // t=0ms
    platform_wait_ms(150);          // t=150ms (exceed 100ms timeout)
    release_key(TAP_DANCE_KEY);      // t=150ms

    std::vector<uint8_t> expected_layers_1 = {1, 0};  // Layer 1 activation/deactivation
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers_1));

    reset_mock_state();

    // Long Tap Timeout
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    platform_wait_ms(300);          // t=350ms (300ms tap timeout)
    std::vector<key_action_t> expected_keys_1 = {
        press(3001, 350), release(3001, 350)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_1));

    reset_mock_state();

    // Sequence Continuation with Long Tap Timeout
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    press_key(TAP_DANCE_KEY, 299);   // t=349ms (1ms before tap timeout)
    release_key(TAP_DANCE_KEY, 50);  // t=399ms
    platform_wait_ms(300);          // t=699ms

    std::vector<key_action_t> expected_keys_2 = {
        press(3001, 699), release(3001, 699)  // single tap, sequence continued
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_2));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Tap Timeout Before Hold Timeout
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms
    platform_wait_ms(150);          // t=250ms (tap timeout)
    std::vector<key_action_t> expected_keys_1 = {
        press(3001, 250), release(3001, 250)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_1));

    reset_mock_state();

    // Hold Timeout After Release
    press_key(TAP_DANCE_KEY);        // t=0ms
    platform_wait_ms(350);          // t=350ms (exceed 300ms hold timeout)
    release_key(TAP_DANCE_KEY);      // t=350ms

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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // TAP_PREFERRED with Configuration
    {
        // Input: Trigger press, interrupt press+release, trigger release (before timeout)
        press_key(TAP_DANCE_KEY);          // t=0ms
        press_key(INTERRUPTING_KEY, 50);   // t=50ms
        release_key(INTERRUPTING_KEY, 50); // t=100ms
        release_key(TAP_DANCE_KEY, 50);    // t=150ms

        // Expected: Tap action (interruption ignored)
        std::vector<key_action_t> expected_keys = {
            press(3001, 0), release(3001, 150)
        };
        EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
    }

    reset_mock_state();

    // BALANCED with Configuration
    {
        // Input: Trigger press, interrupt press+release, trigger release
        press_key(TAP_DANCE_KEY);          // t=0ms
        press_key(INTERRUPTING_KEY, 50);   // t=50ms
        release_key(INTERRUPTING_KEY, 50); // t=100ms
        release_key(TAP_DANCE_KEY, 50);    // t=150ms

        // Expected: Hold action (complete cycle)
        std::vector<key_action_t> expected_keys = {};
        EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

        std::vector<uint8_t> expected_layers = {1, 0};  // Layer 1 activation/deactivation
        EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
    }

    reset_mock_state();

    // HOLD_PREFERRED with Configuration
    {
        // Input: Trigger press, interrupt press, trigger release
        press_key(TAP_DANCE_KEY);          // t=0ms
        press_key(INTERRUPTING_KEY, 50);   // t=50ms
        release_key(INTERRUPTING_KEY, 50); // t=100ms
        release_key(TAP_DANCE_KEY, 50);    // t=150ms

        // Expected: Hold action (immediate on interrupt)
        std::vector<key_action_t> expected_keys = {};
        EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_tap(4, 3004),
        createbehaviouraction_tap(5, 3005)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_config->length++;

    // Attempt to reach 5th tap count
    for (int i = 0; i < 5; i++) {
        tap_key(TAP_DANCE_KEY, 10);
        platform_wait_ms(20);
    }
    platform_wait_ms(200);          // t=150ms

    // Expected: {TAP_5_OUTPUT, PRESS, 1200}, {TAP_5_OUTPUT, RELEASE, 1200},
    std::vector<key_action_t> expected_keys = {
        press(3005, 1200), release(3005, 1200)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    reset_mock_state();

    // Then test 5th hold action
    for (int i = 0; i < 4; i++) {
        tap_key(TAP_DANCE_KEY, 10);
        platform_wait_ms(20);
    }
    press_key(TAP_DANCE_KEY, 20);
    platform_wait_ms(250);
    release_key(TAP_DANCE_KEY);

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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    // Single Valid Action
    tap_key(TAP_DANCE_KEY, 50);
    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 50)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    reset_mock_state();

    // Overflow from Minimal
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 40, 30);  // 2nd tap - overflow
    std::vector<key_action_t> expected_keys_2 = {
        press(3001, 0), release(3001, 30),  // immediate, first tap
        press(3001, 70), release(3001, 110) // immediate, overflow
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_2));
}

// Test 9.9: Mixed Action Types Configuration
// Objective: Verify configurations with different action types at different tap counts
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
// Hold actions: [1: CHANGELAYERTEMPO(1), 3: CHANGELAYERTEMPO(3)] // Skip 2nd
TEST_F(ConfigurationCombinationsTest, MixedActionTypesConfiguration) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // First Count (Both Available)
    tap_key(TAP_DANCE_KEY, 50);
    std::vector<key_action_t> expected_keys_1 = {
        press(3001, 0), release(3001, 50)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_1));

    reset_mock_state();

    // Second Count (Tap Only)
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 50, 30);
    std::vector<key_action_t> expected_keys_2 = {
        press(3002, 0), release(3002, 280)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_2));

    reset_mock_state();

    // Third Count (Hold Only)
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 40, 30);
    press_key(TAP_DANCE_KEY, 30);
    platform_wait_ms(250);
    release_key(TAP_DANCE_KEY);
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 0x31),  // '1' key
        createbehaviouraction_tap(2, 0x32),  // '2' key
        createbehaviouraction_tap(3, 0x33)   // '3' key
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // First Tap (Count 1)
    tap_key(TAP_DANCE_KEY, 50);
    std::vector<key_action_t> expected_keys_1 = {
        press(0x31, 0), release(0x31, 50)  // '1' key
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_1));

    reset_mock_state();

    // Second Tap (Count 2)
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 50, 30);
    std::vector<key_action_t> expected_keys_2 = {
        press(0x32, 0), release(0x32, 80)  // '2' key
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_2));

    reset_mock_state();

    // Third Tap (Count 3)
    tap_key(TAP_DANCE_KEY, 30);
    tap_key(TAP_DANCE_KEY, 40, 30);
    tap_key(TAP_DANCE_KEY, 50, 30);
    std::vector<key_action_t> expected_keys_3 = {
        press(0x33, 0), release(0x33, 150)  // '3' key
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_3));
}

