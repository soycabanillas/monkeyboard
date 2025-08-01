#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

class ImmediateDelayedExecutionTest : public ::testing::Test {
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

// Test 5.1: Immediate Execution - No Hold Action Configured
// Objective: Verify immediate execution when no hold action is available for current tap count
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [] // No hold actions configured
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ImmediateDelayedExecutionTest, ImmediateExecutionNoHoldAction) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms

    // Expected Output: Immediate execution on key press, release follows input timing exactly
    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 5.2: Delayed Execution - Hold Action Available
// Objective: Verify delayed execution when hold action is configured for current tap count
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYER(1)] // Hold action available
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionHoldActionAvailable) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms (before hold timeout)
    platform_wait_ms(200);          // t=300ms (tap timeout)

    // Expected Output: Delayed execution at tap timeout
    std::vector<key_action_t> expected_keys = {
        press(3001, 300), release(3001, 300)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 5.3: State Machine Bypass - Deterministic Outcome
// Objective: Verify state machine is bypassed when outcome is deterministic
// Configuration: Same as Test 5.1 (no hold actions)
TEST_F(ImmediateDelayedExecutionTest, StateMachineBypassDeterministicOutcome) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 300);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 300); // t=300ms (release well beyond timeout)

    // Expected Output: Immediate on press, release follows input timing
    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 300)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 5.4: Delayed Execution - Hold Timeout Reached
// Objective: Verify delayed execution resolves to hold action when timeout reached
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionHoldTimeoutReached) {
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

    // Input Sequence: press_key(TAP_DANCE_KEY); platform_wait_ms(250); release_key(TAP_DANCE_KEY);
    press_key(TAP_DANCE_KEY);        // t=0ms
    platform_wait_ms(250);          // t=250ms (exceed hold timeout)
    release_key(TAP_DANCE_KEY);      // t=250ms

    // Expected Output: Hold action at timeout, deactivation on release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 5.5: Execution Mode Transition - Multi-Tap Sequence
// Objective: Verify execution mode can change within a single multi-tap sequence
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, ExecutionModeTransitionMultiTapSequence) {
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

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); press_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap - hold available)
    release_key(TAP_DANCE_KEY, 100); // t=100ms
    press_key(TAP_DANCE_KEY, 50);    // t=150ms (2nd tap - no hold, immediate)
    release_key(TAP_DANCE_KEY, 100); // t=250ms
    platform_wait_ms(200);          // t=450ms

    // Expected Output: Delayed execution for 1st tap, immediate for 2nd tap
    std::vector<key_action_t> expected_keys = {
        press(3001, 300), release(3001, 300),
        press(3001, 450), release(3001, 450)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 5.6: Immediate Execution - Overflow with SENDKEY Only
// Objective: Verify immediate execution in overflow when no hold actions available
// Configuration: Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [] // No hold actions
TEST_F(ImmediateDelayedExecutionTest, ImmediateExecutionOverflowSENDKEYOnly) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input Sequence: tap_key(TAP_DANCE_KEY, 30); tap_key(TAP_DANCE_KEY, 50, 30); press_key(TAP_DANCE_KEY, 50);
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
    press_key(TAP_DANCE_KEY, 50);    // t=160ms (3rd tap - overflow, immediate)
    release_key(TAP_DANCE_KEY, 100); // t=260ms

    // Expected Output: Immediate execution on press (overflow + no hold)
    std::vector<key_action_t> expected_keys = {
        press(3002, 160), release(3002, 260)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 5.7: Delayed Execution - Overflow with Hold Available
// Objective: Verify delayed execution when overflow occurs but hold actions exist at lower counts
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionOverflowHoldAvailable) {
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

    // Input Sequence: tap_key(TAP_DANCE_KEY, 30); tap_key(TAP_DANCE_KEY, 50, 30); tap_key(TAP_DANCE_KEY, 50, 30);
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (3rd tap - no hold at this count)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (4th tap - overflow)
    platform_wait_ms(200);          // t=470ms

    // Expected Output: Uses 3rd tap action (last configured, delayed execution)
    std::vector<key_action_t> expected_keys = {
        press(3003, 470), release(3003, 470)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 5.8: Immediate Execution Decision Table Verification
// Objective: Verify immediate vs delayed execution decision in various overflow scenarios
TEST_F(ImmediateDelayedExecutionTest, ImmediateExecutionDecisionTableVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Immediate Execution (SENDKEY only, no hold)
    {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction_tap(1, 3001)
        };
        tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
        tap_dance_config->length++;

        // Input: 3 taps (overflow)
        tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
        tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
        tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (3rd tap - overflow)
        platform_wait_ms(200);          // t=390ms

        // Expected: Immediate execution on each press
        std::vector<key_action_t> expected_keys = {
            press(3001, 0), release(3001, 30),
            press(3001, 80), release(3001, 110),
            press(3001, 160), release(3001, 190)
        };
        EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
    }

    // Delayed Execution (Hold available at overflow count)
    {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction_tap(1, 3001),
            createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
        };
        tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
        tap_dance_config->length++;

        // Input: 3 taps with hold attempt
        tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
        tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
        tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms
        tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (overflow)
        platform_wait_ms(200);          // t=470ms

        // Expected: Delayed execution, hold action possible
        std::vector<key_action_t> expected_keys = {
            press(3001, 0), release(3001, 30),
            press(3001, 80), release(3001, 110),
            press(3001, 160), release(3001, 190),
            press(3001, 240), release(3001, 270)
        };
        EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
    }
}
