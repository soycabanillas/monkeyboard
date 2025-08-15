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

class TimingBoundaryConditionsTest : public ::testing::Test {
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

// Test 6.1: Hold Timeout Boundary - 1ms Before
// Objective: Verify tap behavior when released exactly 1ms before hold timeout
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(TimingBoundaryConditionsTest, HoldTimeoutBoundary1msBefore) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 199);    // t=199ms (1ms before hold timeout)
    wait_ms(200);                      // t=399ms (tap timeout)

    // Expected Output: Tap action (released before hold timeout)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 399), td_release(3001, 399)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.2: Hold Timeout Boundary - Exactly At
// Objective: Verify hold behavior when timeout occurs exactly at boundary
// Configuration: Same as Test 6.1
TEST_F(TimingBoundaryConditionsTest, HoldTimeoutBoundaryExactlyAt) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    wait_ms(200);                      // t=200ms (exactly at hold timeout)
    release_key_at(TAP_DANCE_KEY, 200);    // t=200ms

    // Expected Output: Hold action (timeout reached exactly)
    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 200), td_layer(0, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.3: Hold Timeout Boundary - Just After
// Objective: Verify hold behavior when held past timeout
// Configuration: Same as Test 6.1
TEST_F(TimingBoundaryConditionsTest, HoldTimeoutBoundaryJustAfter) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 201);    // t=201ms (1ms after timeout)

    // Expected Output: Hold action at timeout, deactivation at release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200), td_layer(0, 201)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.4: Tap Timeout Boundary - Sequence Reset
// Objective: Verify sequence resets when tap timeout expires between taps
// Configuration: Same as Test 6.1
TEST_F(TimingBoundaryConditionsTest, TapTimeoutBoundarySequenceReset) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    wait_ms(200);                      // t=300ms (tap timeout expires - sequence resets)

    // Expected Output: Immediate execution on press
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 0), td_release(OUTPUT_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.5: Tap Timeout Boundary - Sequence Continuation
// Objective: Verify sequence continues when next press occurs before tap timeout
// Configuration: Same as Test 6.4
TEST_F(TimingBoundaryConditionsTest, TapTimeoutBoundarySequenceContinuation) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    press_key_at(TAP_DANCE_KEY, 199);      // t=299ms (1ms before tap timeout)
    release_key_at(TAP_DANCE_KEY, 50);     // t=349ms
    wait_ms(200);                      // t=549ms

    // Expected Output: Continuation with second tap action
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 549), td_release(3002, 549)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.6: Race Condition - Hold vs Tap Timeout
// Objective: Verify behavior when hold and tap timeouts could occur simultaneously
// Configuration: Same as Test 6.4
TEST_F(TimingBoundaryConditionsTest, RaceConditionHoldVsTapTimeout) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 100);    // t=100ms (start tap timeout)
    // Next press at exactly when both timeouts could occur
    press_key_at(TAP_DANCE_KEY, 200);      // t=300ms (tap timeout + hold start)
    wait_ms(200);                      // t=500ms (hold timeout)
    release_key_at(TAP_DANCE_KEY, 500);    // t=500ms

    // Expected Output: Tap timeout wins (sequence completes first)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 300), td_release(3001, 300),  // First sequence completes
        td_layer(TARGET_LAYER, 500), td_layer(0, 500)             // New sequence hold action
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    // Expected Output: Hold action for new sequence
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 6.7: Race Condition - Strategy vs Timeout
// Objective: Verify strategy behavior when interruption and timeout occur near simultaneously
// Configuration: Same as Test 6.4
TEST_F(TimingBoundaryConditionsTest, RaceConditionStrategyVsTimeout) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3010;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);          // t=0ms
    press_key_at(INTERRUPTING_KEY, 199);     // t=199ms (1ms before hold timeout)
    release_key_at(INTERRUPTING_KEY, 2);     // t=201ms (complete cycle after timeout)
    release_key_at(TAP_DANCE_KEY, 49);       // t=250ms

    // Expected Output: Interrupting key and hold action
    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 199), td_release(INTERRUPTING_KEY, 201),
        td_layer(TARGET_LAYER, 200), td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    // Expected Output: Hold action at timeout, deactivation at release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 6.8: Rapid Sequence Timing - Sub-Timeout Windows
// Objective: Verify system handles rapid sequences well within timeout windows
// Configuration: Same as Test 6.4
TEST_F(TimingBoundaryConditionsTest, RapidSequenceTimingSubTimeoutWindows) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Very rapid sequence - all within first 50ms
    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 10);     // t=10ms
    press_key_at(TAP_DANCE_KEY, 20);       // t=20ms
    release_key_at(TAP_DANCE_KEY, 30);     // t=30ms
    press_key_at(TAP_DANCE_KEY, 40);       // t=40ms
    release_key_at(TAP_DANCE_KEY, 50);     // t=50ms
    wait_ms(200);                      // t=250ms

    // Expected Output: Second tap action (rapid 3-tap sequence uses 2nd action overflow)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 250), td_release(3002, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.9: Timing Precision - Millisecond Accuracy
// Objective: Verify system maintains millisecond timing precision
// Configuration: Same as Test 6.1
TEST_F(TimingBoundaryConditionsTest, TimingPrecisionMillisecondAccuracy) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    wait_ms(1000);                     // t=1000ms (establish high baseline)
    press_key_at(TAP_DANCE_KEY, 1000);     // t=1000ms
    release_key_at(TAP_DANCE_KEY, 1150);   // t=1150ms
    wait_ms(200);                      // t=1350ms

    // Expected Output: Precise timing maintained
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 1350), td_release(3001, 1350)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.10: Timing Boundary Documentation - Reference Values
// Objective: Document exact timing behavior for reference (not executable test)
// Timing Rules Verification:
// - Hold timeout: Action triggers at exactly 200ms from key press
// - Tap timeout: Sequence resets at exactly 200ms from key release
// - Race conditions: Earlier event (by timestamp) takes precedence
// - Boundary conditions: >= timeout value triggers timeout behavior
// - Precision: System maintains millisecond accuracy
TEST_F(TimingBoundaryConditionsTest, TimingBoundaryDocumentation) {
    // This test is for documentation purposes, no executable code
    // See test description for timing rules and critical boundaries
    SUCCEED();
}

// Test 6.11: Multiple Timeout Windows - Sequence Chain
// Objective: Verify correct timeout calculation across multiple tap timeout windows
// Configuration: Same as Test 6.4
TEST_F(TimingBoundaryConditionsTest, MultipleTimeoutWindowsSequenceChain) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 50);     // t=50ms
    // Wait near tap timeout, then continue
    press_key_at(TAP_DANCE_KEY, 245);      // t=245ms (within first tap timeout)
    release_key_at(TAP_DANCE_KEY, 295);    // t=295ms
    // Wait near second tap timeout, then continue
    press_key_at(TAP_DANCE_KEY, 490);      // t=490ms (within second tap timeout)
    release_key_at(TAP_DANCE_KEY, 540);    // t=540ms
    wait_ms(200);                      // t=740ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 740), td_release(3002, 740)  // Third tap uses second action (overflow)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.12: Timeout Accumulation - Long Sequence
// Objective: Verify timeout calculations don't accumulate errors over long sequences
TEST_F(TimingBoundaryConditionsTest, TimeoutAccumulationLongSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // 5 taps, each at 180ms intervals (within tap timeout)
    for (int i = 0; i < 5; i++) {
        press_key_at(TAP_DANCE_KEY, i * 180);      // t=i*180ms
        release_key_at(TAP_DANCE_KEY, i * 180 + 50); // t=i*180+50ms
        wait_ms(130);                          // t=i*180+180ms
    }
    wait_ms(200);                              // Final timeout

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 920), td_release(3002, 920)  // Uses second action (overflow from 5 taps)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.13: Zero-Duration Edge Cases
// Objective: Verify timing behavior with zero-duration key presses
TEST_F(TimingBoundaryConditionsTest, ZeroDurationEdgeCases) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms
    release_key_at(TAP_DANCE_KEY, 0);      // t=0ms (zero duration)
    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms (immediate second press)
    release_key_at(TAP_DANCE_KEY, 100);    // t=100ms
    wait_ms(200);                      // t=300ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 300), td_release(3002, 300)  // Second tap action (two zero-duration taps)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 6.14: Timeout Boundary with Strategy Integration
// Objective: Verify timing boundaries work correctly with different hold strategies
TEST_F(TimingBoundaryConditionsTest, TimeoutBoundaryWithStrategyIntegration) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);          // t=0ms
    // Interrupt exactly at hold timeout
    press_key_at(INTERRUPTING_KEY, 200);     // t=200ms (exactly at timeout)
    release_key_at(INTERRUPTING_KEY, 201);   // t=201ms
    release_key_at(TAP_DANCE_KEY, 250);      // t=250ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 200),
        td_release(INTERRUPTING_KEY, 201),
        td_layer(1, 200), td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {1, 0};  // Timeout and strategy both trigger hold
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 6.15: Complex Timing Scenario - Mixed Boundaries
// Objective: Verify system handles complex timing with multiple near-boundary conditions
TEST_F(TimingBoundaryConditionsTest, ComplexTimingScenarioMixedBoundaries) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);          // t=0ms (1st tap)
    release_key_at(TAP_DANCE_KEY, 199);      // t=199ms (1ms before hold timeout)

    press_key_at(TAP_DANCE_KEY, 200);        // t=200ms (2nd tap, exactly at first timeout)
    press_key_at(INTERRUPTING_KEY, 399);     // t=399ms (1ms before second hold timeout)
    release_key_at(INTERRUPTING_KEY, 401);   // t=401ms (complete cycle after timeout)
    release_key_at(TAP_DANCE_KEY, 450);      // t=450ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 399),
        td_release(INTERRUPTING_KEY, 401),
        td_layer(2, 400), td_layer(0, 450)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {2, 0};  // Hold timeout for 2nd tap wins
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}
