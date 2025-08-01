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

class ActionOverflowTest : public ::testing::Test {
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

// Test 4.1: Basic Tap Action Overflow
// Objective: Verify tap action overflow uses last configured action
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ActionOverflowTest, BasicTapActionOverflow) {
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

    // Perform 4 taps (exceeds configured actions)
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (3rd tap - overflow)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (4th tap - overflow)
    platform_wait_ms(200);          // t=470ms

    // Expected Output: Uses last configured action (2nd tap action)
    std::vector<key_action_t> expected_keys = {
        press(3002, 470), release(3002, 470)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.2: Hold Action Non-Overflow
// Objective: Verify hold actions do NOT overflow - no hold available beyond configured counts
// Configuration: Same as Test 4.1
TEST_F(ActionOverflowTest, HoldActionNonOverflow) {
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

    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
    press_key(TAP_DANCE_KEY, 50);    // t=160ms (3rd tap - overflow, attempt hold)
    platform_wait_ms(250);          // t=410ms (exceed hold timeout)
    release_key(TAP_DANCE_KEY);      // t=410ms
    platform_wait_ms(200);          // t=610ms

    // Expected Output: Tap action only (no hold available for 3rd tap)
    std::vector<key_action_t> expected_keys = {
        press(3002, 610), release(3002, 610)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.3: Overflow with Only SENDKEY Actions - Immediate Execution
// Objective: Verify immediate execution when overflow occurs with only SENDKEY actions
// Configuration: Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [] // No hold actions
TEST_F(ActionOverflowTest, OverflowImmediateExecution) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

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

// Test 4.5: Extreme Overflow - High Tap Count
// Objective: Verify system handles very high tap counts with overflow
// Configuration: Same as Test 4.1
TEST_F(ActionOverflowTest, ExtremeOverflowHighTapCount) {
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

    // Perform 10 rapid taps
    for (int i = 0; i < 10; i++) {
        tap_key(TAP_DANCE_KEY, 20);
        platform_wait_ms(30);       // t = i*50 to (i*50+20)
    }
    platform_wait_ms(200);          // Final timeout at t=700ms

    // Expected Output: Still uses last configured action (2nd tap)
    std::vector<key_action_t> expected_keys = {
        press(3002, 700), release(3002, 700)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.6: Overflow Hold Attempt with Strategy
// Objective: Verify overflow with hold attempt using different strategies
// Configuration: Strategy: HOLD_PREFERRED, Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
// Hold actions: [1: CHANGELAYER(1)], INTERRUPTING_KEY = 3010
TEST_F(ActionOverflowTest, OverflowHoldAttemptWithStrategy) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    tap_key(TAP_DANCE_KEY, 30);        // t=0-30ms (1st tap)
    tap_key(TAP_DANCE_KEY, 50, 30);    // t=80-110ms (2nd tap)
    press_key(TAP_DANCE_KEY, 50);      // t=160ms (3rd tap - overflow)
    press_key(INTERRUPTING_KEY, 50);   // t=210ms (interrupt - would trigger hold if available)
    release_key(INTERRUPTING_KEY, 50); // t=260ms
    release_key(TAP_DANCE_KEY, 50);    // t=310ms
    platform_wait_ms(200);            // t=510ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 210),
        release(INTERRUPTING_KEY, 260),
        press(3002, 510),              // Tap action (no hold available for 3rd tap)
        release(3002, 510)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.7: Overflow Mixed with Non-Overflow Hold
// Objective: Verify overflow tap behavior mixed with valid hold actions at lower counts
// Configuration: Same as Test 4.4
TEST_F(ActionOverflowTest, OverflowMixedWithNonOverflowHold) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);        // t=0ms (1st tap - hold available)
    platform_wait_ms(250);          // t=250ms (hold timeout exceeded)
    release_key(TAP_DANCE_KEY);      // t=250ms
    platform_wait_ms(50);           // t=300ms

    // New sequence with overflow
    tap_key(TAP_DANCE_KEY, 30);      // t=330-360ms (1st tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=410-460ms (2nd tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=510-540ms (3rd tap)
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=590-620ms (4th tap - overflow)
    platform_wait_ms(200);          // t=820ms

    std::vector<uint8_t> expected_layers = {1, 0};  // First sequence - hold action
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<key_action_t> expected_keys = {
        press(3003, 820), release(3003, 820)  // Second sequence - overflow uses 3rd action
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.8: Overflow Boundary - Exactly at Last Configured Action
// Objective: Verify behavior exactly at the boundary of configured actions
TEST_F(ActionOverflowTest, OverflowBoundaryExactlyAtLastConfiguredAction) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_config->length++;

    // Exactly 3 taps (matches last configured action)
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (exactly at boundary)
    platform_wait_ms(200);          // t=390ms

    std::vector<key_action_t> expected_keys = {
        press(3003, 390), release(3003, 390)  // Uses exact configured action (not overflow)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.9: Overflow Boundary - One Beyond Last Configured
// Objective: Verify overflow behavior starts exactly one beyond last configured action
TEST_F(ActionOverflowTest, OverflowBoundaryOneBeyondLastConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_config->length++;

    // 4 taps (one beyond last configured)
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (first overflow)
    platform_wait_ms(200);          // t=470ms

    std::vector<key_action_t> expected_keys = {
        press(3003, 470), release(3003, 470)  // Uses last configured action (overflow behavior)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.10: Overflow with Hold Available at Overflow Count
// Objective: Verify hold action at overflow count when configured
TEST_F(ActionOverflowTest, OverflowWithHoldAvailableAtOverflowCount) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(3, 3, TAP_DANCE_HOLD_PREFERRED)  // Hold at 3rd
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_config->length++;

    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
    press_key(TAP_DANCE_KEY, 50);    // t=160ms (3rd tap - hold available)
    platform_wait_ms(250);          // t=410ms
    release_key(TAP_DANCE_KEY);      // t=410ms

    std::vector<uint8_t> expected_layers = {3, 0};  // Hold action available at 3rd tap
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 4.11: Immediate Execution Decision Table - Overflow Scenarios
// Objective: Verify immediate vs delayed execution decision in various overflow scenarios
TEST_F(ActionOverflowTest, ImmediateExecutionDecisionTableOverflowScenarios) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Test immediate execution (SENDKEY only, no hold)
    pipeline_tap_dance_action_config_t* actions_immediate[] = {
        createbehaviouraction_tap(1, 3001)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions_immediate, 1);
    tap_dance_config->length++;

    // Input: 3 taps (overflow)
    tap_key(TAP_DANCE_KEY, 20);      // t=0-20ms
    tap_key(TAP_DANCE_KEY, 30, 20);  // t=50-80ms
    tap_key(TAP_DANCE_KEY, 30, 20);  // t=120-150ms (overflow)

    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 20),     // Immediate execution on each press
        press(3001, 50), release(3001, 80),
        press(3001, 120), release(3001, 150)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.12: Overflow Reset Verification
// Objective: Verify overflow sequences properly reset tap counts
TEST_F(ActionOverflowTest, OverflowResetVerification) {
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

    // First overflow sequence (5 taps)
    for (int i = 0; i < 5; i++) {
        tap_key(TAP_DANCE_KEY, 20);
        platform_wait_ms(30);
    }
    platform_wait_ms(200);          // First sequence completes

    // Second sequence (2 taps - should not be affected by previous overflow)
    tap_key(TAP_DANCE_KEY, 30);      // Should be 1st tap
    tap_key(TAP_DANCE_KEY, 50, 30);  // Should be 2nd tap
    platform_wait_ms(200);

    std::vector<key_action_t> expected_keys = {
        press(3002, 350), release(3002, 350),  // First sequence - overflow (5th tap uses 2nd action)
        press(3002, 630), release(3002, 630)   // Second sequence - normal 2nd tap
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.13: Overflow with Different Action Types
// Objective: Verify overflow works correctly with different action types in sequence
TEST_F(ActionOverflowTest, OverflowWithDifferentActionTypes) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),  // Different key
        createbehaviouraction_tap(3, 3003),  // Third key for overflow
        createbehaviouraction_hold(1, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_config->length++;

    // 4 taps - overflow should use 3rd action (SENDKEY)
    tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms
    tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (overflow)
    platform_wait_ms(200);          // t=470ms

    std::vector<key_action_t> expected_keys = {
        press(3003, 470), release(3003, 470)  // Uses 3rd action (SENDKEY) for overflow
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.14: Continuous Overflow - Multiple Sequences
// Objective: Verify consistent overflow behavior across multiple sequences
TEST_F(ActionOverflowTest, ContinuousOverflowMultipleSequences) {
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

    // First overflow sequence
    tap_key(TAP_DANCE_KEY, 20);
    tap_key(TAP_DANCE_KEY, 30, 20);
    tap_key(TAP_DANCE_KEY, 30, 20);  // 3rd tap - overflow
    platform_wait_ms(200);          // t=220ms

    platform_wait_ms(100);          // Gap between sequences

    // Second overflow sequence
    tap_key(TAP_DANCE_KEY, 20);      // t=340-360ms
    tap_key(TAP_DANCE_KEY, 30, 20);  // t=380-410ms
    tap_key(TAP_DANCE_KEY, 30, 20);  // t=440-470ms
    tap_key(TAP_DANCE_KEY, 30, 20);  // t=500-530ms (4th tap - overflow)
    platform_wait_ms(200);          // t=730ms

    std::vector<key_action_t> expected_keys = {
        press(3002, 220), release(3002, 220),  // First overflow - 3rd tap uses 2nd action
        press(3002, 730), release(3002, 730)   // Second overflow - 4th tap uses 2nd action
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 4.15: Overflow Edge Case - Zero Configured Actions
// Objective: Verify behavior when no tap actions are configured but taps attempted
// Configuration: Tap actions: [], Hold actions: [1: CHANGELAYER(1)]
TEST_F(ActionOverflowTest, OverflowEdgeCaseZeroConfiguredActions) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Only hold actions, no tap actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms (no tap action available)
    platform_wait_ms(200);          // t=250ms

    std::vector<key_action_t> expected_keys = {};  // No output - no tap actions configured
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
}
