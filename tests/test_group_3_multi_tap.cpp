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

class MultiTapTest : public ::testing::Test {
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

// Test 3.1: Basic Two-Tap Sequence
// Objective: Verify basic two-tap sequence with proper tap count progression
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(MultiTapTest, BasicTwoTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); press_key(TAP_DANCE_KEY, 150); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap begins)
    release_key(TAP_DANCE_KEY, 100); // t=100ms (1st tap completes)
    press_key(TAP_DANCE_KEY, 150);   // t=250ms (2nd tap begins, within timeout)
    release_key(TAP_DANCE_KEY, 100); // t=350ms (2nd tap completes)
    platform_wait_ms(200);          // t=550ms (tap timeout expires)

    // Expected: Second tap action executed
    std::vector<key_action_t> expected_keys = {
        press(3002, 550), release(3002, 550)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.2: Three-Tap Sequence
// Objective: Verify three-tap sequence progression
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(MultiTapTest, ThreeTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); press_key(TAP_DANCE_KEY, 150); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap begins)
    release_key(TAP_DANCE_KEY, 100); // t=100ms (1st tap completes)
    press_key(TAP_DANCE_KEY, 150);   // t=250ms (2nd tap begins, within timeout)
    release_key(TAP_DANCE_KEY, 100); // t=350ms (2nd tap completes)
    press_key(TAP_DANCE_KEY, 150);   // t=500ms (3rd tap begins, within timeout)
    release_key(TAP_DANCE_KEY, 100); // t=600ms (3rd tap completes)
    platform_wait_ms(200);          // t=800ms (tap timeout expires)

    // Expected: Third tap action executed
    std::vector<key_action_t> expected_keys = {
        press(3003, 800), release(3003, 800)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.3: Sequence Reset - Tap Timeout Expiry
// Objective: Verify sequence resets when tap timeout expires between taps
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, SequenceResetTapTimeout) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(200); press_key(TAP_DANCE_KEY, 50); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms
    platform_wait_ms(200);          // t=300ms (tap timeout expires - sequence resets)
    press_key(TAP_DANCE_KEY, 50);    // t=350ms (new sequence begins)
    release_key(TAP_DANCE_KEY, 100); // t=450ms
    platform_wait_ms(200);          // t=650ms

    // Expected: First sequence completes (1st tap), Second sequence (also 1st tap)
    std::vector<key_action_t> expected_keys = {
        press(3001, 300), release(3001, 300),
        press(3001, 650), release(3001, 650)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.4: Multi-Tap with Hold Action (First Tap)
// Objective: Verify hold action works correctly during multi-tap sequence (1st tap count)
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapHoldActionFirstTap) {
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

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 50); platform_wait_ms(250);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    platform_wait_ms(250);          // t=300ms (hold timeout exceeded)

    // Expected: Layer activation at timeout, deactivation on release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 3.5: Multi-Tap with Hold Action (Second Tap)
// Objective: Verify hold action at second tap count when configured
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapHoldActionSecondTap) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); press_key(TAP_DANCE_KEY, 50); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(250);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 100); // t=100ms (1st tap complete)
    press_key(TAP_DANCE_KEY, 50);    // t=150ms (2nd tap begins)
    platform_wait_ms(250);          // t=400ms (hold 2nd tap)
    release_key(TAP_DANCE_KEY);      // t=400ms

    // Expected: Hold action for 2nd tap count (150ms + 200ms timeout)
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 3.6: Hold Action Not Available for Tap Count
// Objective: Verify behavior when hold action not configured for current tap count
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, HoldActionNotAvailableForTapCount) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 50); press_key(TAP_DANCE_KEY, 50); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(250);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    press_key(TAP_DANCE_KEY, 50);    // t=100ms (2nd tap - no hold action)
    release_key(TAP_DANCE_KEY, 100); // t=200ms
    platform_wait_ms(250);          // t=450ms

    // Expected: Tap action (no hold available for 2nd tap)
    std::vector<key_action_t> expected_keys = {
        press(3001, 450), release(3001, 450)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.7: Rapid Tap Sequence - All Within Timeout
// Objective: Verify system handles extremely rapid tap sequences
// Configuration: Same as Test 3.2
TEST_F(MultiTapTest, RapidTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: rapid sequence within timeout
    for (int i = 0; i < 5; i++) {
        press_key(TAP_DANCE_KEY, 10);
        release_key(TAP_DANCE_KEY, 10);
    }
    platform_wait_ms(200);          // t=100ms

    // Expected: Third tap action executed
    std::vector<key_action_t> expected_keys = {
        press(3002, 100), release(3002, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.8: Mixed Tap and Hold in Sequence
// Objective: Verify mix of tap and hold behaviors within single sequence
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MixedTapAndHoldInSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 50); press_key(TAP_DANCE_KEY, 50); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(250);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    press_key(TAP_DANCE_KEY, 50);    // t=100ms (2nd tap begins)
    platform_wait_ms(250);          // t=350ms (hold 2nd tap)
    release_key(TAP_DANCE_KEY);      // t=350ms

    // Expected: Hold action for 2nd tap count (150ms + 200ms timeout)
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 3.9: Tap Count Boundary - Exact Timeout Edge
// Objective: Verify timing precision at tap timeout boundaries
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, TapCountBoundaryExactTimeoutEdge) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 200); press_key(TAP_DANCE_KEY, 200); release_key(TAP_DANCE_KEY, 50); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 200); // t=200ms (exactly at timeout)
    press_key(TAP_DANCE_KEY, 200);   // t=400ms (new sequence, exactly at timeout)
    release_key(TAP_DANCE_KEY, 50);  // t=450ms
    platform_wait_ms(200);          // t=650ms

    // Expected: Normal tap action behavior
    std::vector<key_action_t> expected_keys = {
        press(3001, 650), release(3001, 650)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.10: Maximum Practical Tap Count
// Objective: Verify system handles high tap counts correctly
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MaximumPracticalTapCount) {
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

    // Input Sequence: Perform 5 rapid taps
    for (int i = 0; i < 5; i++) {
        press_key(TAP_DANCE_KEY, 20);
        release_key(TAP_DANCE_KEY, 20);
    }
    platform_wait_ms(200);          // t=100ms

    // Expected: Fifth tap action executed
    std::vector<key_action_t> expected_keys = {
        press(3005, 100), release(3005, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.11: Sequence Continuation vs New Sequence
// Objective: Verify clear distinction between sequence continuation and new sequence
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, SequenceContinuationVsNewSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); press_key(TAP_DANCE_KEY, 199); release_key(TAP_DANCE_KEY, 50); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms
    press_key(TAP_DANCE_KEY, 199);   // t=299ms (1ms before timeout)
    release_key(TAP_DANCE_KEY, 50);  // t=349ms
    platform_wait_ms(200);          // t=549ms

    // Expected: First sequence completes (1st tap), Second sequence (also 1st tap)
    std::vector<key_action_t> expected_keys = {
        press(3001, 300), release(3001, 300),
        press(3001, 549), release(3001, 549)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.12: Multi-Tap with Strategy Interruption
// Objective: Verify multi-tap behavior combined with hold strategy interruption
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapWithStrategyInterruption) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 50); press_key(TAP_DANCE_KEY, 50); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(250);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    press_key(TAP_DANCE_KEY, 50);    // t=100ms (2nd tap begins)
    press_key(3003, 50);             // t=150ms (interrupt - would trigger hold if available)
    release_key(3003, 50);           // t=200ms (complete cycle)
    release_key(TAP_DANCE_KEY, 50);  // t=250ms
    platform_wait_ms(200);          // t=450ms

    // Expected: Hold action for 2nd tap count (150ms + 200ms timeout)
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 3.13: Tap Count Reset Verification
// Objective: Verify tap count properly resets between independent sequences
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, TapCountResetVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 50); press_key(TAP_DANCE_KEY, 50); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(250);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 50);  // t=50ms
    press_key(TAP_DANCE_KEY, 50);    // t=100ms (2nd tap - new sequence)
    release_key(TAP_DANCE_KEY, 100); // t=200ms
    platform_wait_ms(250);          // t=450ms

    // Expected: Tap action (no hold available for 2nd tap)
    std::vector<key_action_t> expected_keys = {
        press(3001, 450), release(3001, 450)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.14: Very Fast Multi-Tap Sequence
// Objective: Verify system handles extremely rapid tap sequences
// Configuration: Same as Test 3.2
TEST_F(MultiTapTest, VeryFastMultiTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input: rapid sequence within timeout
    for (int i = 0; i < 5; i++) {
        press_key(TAP_DANCE_KEY, 10);
        release_key(TAP_DANCE_KEY, 10);
    }
    platform_wait_ms(200);          // t=100ms

    // Expected: Third tap action executed
    std::vector<key_action_t> expected_keys = {
        press(3002, 100), release(3002, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 3.15: Multi-Tap Overflow Preview
// Objective: Verify behavior approaching overflow conditions (sets up for Group 4)
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapOverflowPreview) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    // Input Sequence: tap_key(TAP_DANCE_KEY, 30); tap_key(TAP_DANCE_KEY, 50, 30); tap_key(TAP_DANCE_KEY, 50, 30); tap_key(TAP_DANCE_KEY, 50, 30); platform_wait_ms(200);
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (overflow)
    platform_wait_ms(200);          // t=470ms

    // Expected: Uses last configured action (2nd tap action)
    std::vector<key_action_t> expected_keys = {
        press(3002, 470), release(3002, 470)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}
