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
            malloc(sizeof(*tap_dance_config)));
        tap_dance_config->length = 0;
        tap_dance_config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);    // t=100ms

    // Expected Output: Immediate execution on key press, release follows input timing exactly
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 100)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.2: Delayed Execution - Hold Action Available
// Objective: Verify delayed execution when hold action is configured for current tap count
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYER(1)] // Hold action available
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionHoldActionAvailable) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);    // t=100ms (before hold timeout)
    keyboard.wait_ms(200);                      // t=300ms (tap timeout)

    // Expected Output: Delayed execution at tap timeout
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 300), td_release(3001, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.3: State Machine Bypass - Deterministic Outcome
// Objective: Verify state machine is bypassed when outcome is deterministic
// Configuration: Same as Test 5.1 (no hold actions)
TEST_F(ImmediateDelayedExecutionTest, StateMachineBypassDeterministicOutcome) {
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

    // Input Sequence: keyboard.press_key(TAP_DANCE_KEY); keyboard.release_key(TAP_DANCE_KEY, 300);
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 300);    // t=300ms (release well beyond timeout)

    // Expected Output: Immediate on press, release follows input timing
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.4: Delayed Execution - Hold Timeout Reached
// Objective: Verify delayed execution resolves to hold action when timeout reached
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionHoldTimeoutReached) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Input Sequence: keyboard.press_key(TAP_DANCE_KEY); platform_wait_ms(250); keyboard.release_key(TAP_DANCE_KEY);
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.wait_ms(250);                      // t=250ms (exceed hold timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 250);    // t=250ms

    // Expected Output: Hold action at timeout, deactivation on release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.5: Execution Mode Transition - Multi-Tap Sequence
// Objective: Verify execution mode can change within a single multi-tap sequence
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, ExecutionModeTransitionMultiTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Input Sequence: keyboard.press_key(TAP_DANCE_KEY); keyboard.release_key(TAP_DANCE_KEY, 100); keyboard.press_key(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms (1st tap - hold available)
    keyboard.release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    keyboard.press_key_at(TAP_DANCE_KEY, 150);      // t=150ms (2nd tap - no hold, immediate)
    keyboard.release_key_at(TAP_DANCE_KEY, 250);    // t=250ms
    keyboard.wait_ms(200);                      // t=450ms

    // Expected Output: Delayed execution for 1st tap, immediate for 2nd tap
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 300), td_release(3001, 300),
        td_press(3001, 450), td_release(3001, 450)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.6: Immediate Execution - Overflow with SENDKEY Only
// Objective: Verify immediate execution in overflow when no hold actions available
// Configuration: Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [] // No hold actions
TEST_F(ImmediateDelayedExecutionTest, ImmediateExecutionOverflowSENDKEYOnly) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Input Sequence: keyboard.tap_key(TAP_DANCE_KEY, 30); keyboard.tap_key(TAP_DANCE_KEY, 50, 30); keyboard.press_key(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 0);      // t=0ms (1st tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    keyboard.press_key_at(TAP_DANCE_KEY, 80);     // t=80ms (2nd tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    keyboard.press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - overflow, immediate)
    keyboard.release_key_at(TAP_DANCE_KEY, 260);  // t=260ms

    // Expected Output: Immediate execution on press (overflow + no hold)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 160), td_release(3002, 260)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.7: Delayed Execution - Overflow with Hold Available
// Objective: Verify delayed execution when overflow occurs but hold actions exist at lower counts
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionOverflowHoldAvailable) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Input Sequence: keyboard.tap_key(TAP_DANCE_KEY, 30); keyboard.tap_key(TAP_DANCE_KEY, 50, 30); keyboard.tap_key(TAP_DANCE_KEY, 50, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 0);      // t=0ms (1st tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    keyboard.press_key_at(TAP_DANCE_KEY, 80);     // t=80ms (2nd tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    keyboard.press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - no hold at this count)
    keyboard.release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
    keyboard.press_key_at(TAP_DANCE_KEY, 240);    // t=240ms (4th tap - overflow)
    keyboard.release_key_at(TAP_DANCE_KEY, 270);  // t=270ms
    keyboard.wait_ms(200);                    // t=470ms

    // Expected Output: Uses 3rd tap action (last configured, delayed execution)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3003, 470), td_release(3003, 470)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.8: Immediate Execution Decision Table Verification
// Objective: Verify immediate vs delayed execution decision in various overflow scenarios
TEST_F(ImmediateDelayedExecutionTest, ImmediateExecutionDecisionTableVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Immediate Execution (SENDKEY only, no hold)
    {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction_tap(1, 3001)
        };
        pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
        tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
        tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
        tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
        tap_dance_config->length++;

        // Input: 3 taps (overflow)
        keyboard.press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
        keyboard.release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
        keyboard.press_key_at(TAP_DANCE_KEY, 80);     // t=80ms
        keyboard.release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
        keyboard.press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - overflow)
        keyboard.release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
        keyboard.wait_ms(200);                    // t=390ms

        // Expected: Immediate execution on each press
        std::vector<tap_dance_event_t> expected_events = {
            td_press(3001, 0), td_release(3001, 30),
            td_press(3001, 80), td_release(3001, 110),
            td_press(3001, 160), td_release(3001, 190)
        };
        EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
    }

    // Reset for next test
    reset_mock_state();
    tap_dance_config->length = 0;

    // Delayed Execution (Hold available at overflow count)
    {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction_tap(1, 3001),
            createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
        };
        pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
        tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
        tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
        tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
        tap_dance_config->length++;

        // Input: 4 taps with hold attempt
        keyboard.press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
        keyboard.release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
        keyboard.press_key_at(TAP_DANCE_KEY, 80);     // t=80ms
        keyboard.release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
        keyboard.press_key_at(TAP_DANCE_KEY, 160);    // t=160ms
        keyboard.release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
        keyboard.press_key_at(TAP_DANCE_KEY, 240);    // t=240ms (overflow)
        keyboard.release_key_at(TAP_DANCE_KEY, 270);  // t=270ms
        keyboard.wait_ms(200);                    // t=470ms

        // Expected: Delayed execution, uses last configured action
        std::vector<tap_dance_event_t> expected_events = {
            td_press(3001, 470), td_release(3001, 470)
        };
        EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
    }
}

// Test 5.9: Delayed Execution Timing Precision
// Objective: Verify delayed execution happens exactly at timeout boundaries
// Configuration: Same as Test 5.2
TEST_F(ImmediateDelayedExecutionTest, DelayedExecutionTimingPrecision) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.wait_ms(100);                      // t=100ms (establish baseline)
    keyboard.press_key_at(TAP_DANCE_KEY, 100);      // t=100ms
    keyboard.release_key_at(TAP_DANCE_KEY, 150);    // t=150ms (before hold timeout)
    keyboard.wait_ms(200);                      // t=350ms (tap timeout from release)

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 350), td_release(3001, 350)  // Exactly at tap timeout (150ms + 200ms)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.10: Mixed Execution Modes - Strategy Integration
// Objective: Verify execution mode determination with different hold strategies
TEST_F(ImmediateDelayedExecutionTest, MixedExecutionModesStrategyIntegration) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)  // Hold only for 1st tap
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);          // t=0ms (1st tap - hold available)
    keyboard.release_key_at(TAP_DANCE_KEY, 50);       // t=50ms
    keyboard.press_key_at(TAP_DANCE_KEY, 100);        // t=100ms (2nd tap - no hold available)
    keyboard.press_key_at(INTERRUPTING_KEY, 150);     // t=150ms (interrupt - would trigger hold if available)
    keyboard.release_key_at(INTERRUPTING_KEY, 200);   // t=200ms
    keyboard.release_key_at(TAP_DANCE_KEY, 250);      // t=250ms
    keyboard.wait_ms(200);                        // t=450ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 150),
        td_release(INTERRUPTING_KEY, 200),
        td_press(3002, 450), td_release(3002, 450)  // Delayed execution (sequence level decision)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.11: Execution Mode Decision Table Verification
// Objective: Systematically verify all execution mode decision conditions
TEST_F(ImmediateDelayedExecutionTest, ExecutionModeDecisionTableVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Test immediate: No hold, within config
    pipeline_tap_dance_action_config_t* actions_immediate[] = {
        createbehaviouraction_tap(1, 3001)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior1 = createbehaviour(TAP_DANCE_KEY, actions_immediate, 1);
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

    // Test delayed: Hold available, within config
    pipeline_tap_dance_action_config_t* actions_delayed[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior2 = createbehaviour(TAP_DANCE_KEY, actions_delayed, 2);
    tap_dance_behavior2->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior2->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior2;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.wait_ms(200);
    std::vector<tap_dance_event_t> expected_delayed = {
        td_press(3001, 250), td_release(3001, 250)  // Delayed execution
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_delayed));
}

// Test 5.12: State Machine Bypass Verification
// Objective: Verify internal state machine is actually bypassed in immediate execution
TEST_F(ImmediateDelayedExecutionTest, StateMachineBypassVerification) {
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

    // Rapid sequence that would normally require state machine
    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 10);     // t=10ms
    keyboard.press_key_at(TAP_DANCE_KEY, 20);       // t=20ms (rapid second press)
    keyboard.release_key_at(TAP_DANCE_KEY, 30);     // t=30ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 10),    // First immediate execution
        td_press(3001, 20), td_release(3001, 30)    // Second immediate execution (bypassed)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.13: Execution Responsiveness Comparison
// Objective: Compare response timing between immediate and delayed execution
TEST_F(ImmediateDelayedExecutionTest, ExecutionResponsivenessComparison) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    // Immediate Execution Test
    pipeline_tap_dance_action_config_t* actions_immediate[] = {
        createbehaviouraction_tap(1, 3001)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior1 = createbehaviour(TAP_DANCE_KEY, actions_immediate, 1);
    tap_dance_behavior1->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior1->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior1;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    // Expected output at t=0ms (immediate)
    std::vector<tap_dance_event_t> expected_immediate = {
        td_press(3001, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_immediate));

    reset_mock_state();
    tap_dance_config->length = 0;

    // Delayed Execution Test
    pipeline_tap_dance_action_config_t* actions_delayed[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior2 = createbehaviour(TAP_DANCE_KEY, actions_delayed, 2);
    tap_dance_behavior2->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior2->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior2;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    keyboard.wait_ms(200);                      // t=300ms
    // Expected output at t=300ms (after tap timeout)
    std::vector<tap_dance_event_t> expected_delayed = {
        td_press(3001, 300), td_release(3001, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_delayed));
}

// Test 5.14: Execution Mode with Zero-Duration Actions
// Objective: Verify execution mode handling with instantaneous press/release
TEST_F(ImmediateDelayedExecutionTest, ExecutionModeWithZeroDurationActions) {
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

    keyboard.press_key_at(TAP_DANCE_KEY, 0);       // t=0ms (instant press+release)
    keyboard.release_key_at(TAP_DANCE_KEY, 0);     // t=0ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0), td_release(3001, 0)  // Immediate execution on press and release
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 5.15: Complex Execution Mode Scenario
// Objective: Verify execution mode determination in complex multi-tap with mixed availability
// Configuration: Tap actions: [1-4: SENDKEY], Hold actions: [2,4: CHANGELAYER]
TEST_F(ImmediateDelayedExecutionTest, ComplexExecutionModeScenario) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_tap(4, 3004),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(4, 4, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 6);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Test 5th tap (overflow, hold exists at 4th) - Delayed expected
    for (int i = 0; i < 5; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 50);
        keyboard.release_key_at(TAP_DANCE_KEY, i * 50 + 20);
    }
    keyboard.wait_ms(200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3004, 450), td_release(3004, 450)  // Delayed execution (hold exists at lower count)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
