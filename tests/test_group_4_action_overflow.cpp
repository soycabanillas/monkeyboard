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

class ActionOverflowTest : public ::testing::Test {
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

// Test 4.1: Basic Tap Action Overflow
// Objective: Verify tap action overflow uses last configured action
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ActionOverflowTest, BasicTapActionOverflow) {
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

    // Perform 4 taps (exceeds configured actions)
    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms (1st tap)
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms (2nd tap)
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - overflow)
    release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
    press_key_at(TAP_DANCE_KEY, 240);    // t=240ms (4th tap - overflow)
    release_key_at(TAP_DANCE_KEY, 270);  // t=270ms
    wait_ms(200);                    // t=470ms

    // Expected Output: Uses last configured action (2nd tap action)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 280),
        td_release(3002, 280),
        td_press(3002, 440),
        td_release(3002, 440)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.2: Hold Action Non-Overflow
// Objective: Verify hold actions do NOT overflow - no hold available beyond configured counts
// Configuration: Same as Test 4.1
TEST_F(ActionOverflowTest, HoldActionNonOverflow) {
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

    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms (1st tap)
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms (2nd tap)
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - overflow, attempt hold)
    wait_ms(250);                    // t=410ms (exceed hold timeout)
    release_key_at(TAP_DANCE_KEY, 410);  // t=410ms
    wait_ms(200);                    // t=610ms

    // Expected Output: Tap action only (no hold available for 3rd tap)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 280),
        td_release(3002, 280)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.3: Overflow with Only SENDKEY Actions - Immediate Execution
// Objective: Verify immediate execution when overflow occurs with only SENDKEY actions
// Configuration: Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [] // No hold actions
TEST_F(ActionOverflowTest, OverflowImmediateExecution) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms (1st tap)
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms (2nd tap)
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - overflow, immediate)
    release_key_at(TAP_DANCE_KEY, 260);  // t=260ms

    // Expected Output: Immediate execution on press (overflow + no hold)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 280),
        td_release(3002, 280)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.5: Extreme Overflow - High Tap Count
// Objective: Verify system handles very high tap counts with overflow
// Configuration: Same as Test 4.1
TEST_F(ActionOverflowTest, ExtremeOverflowHighTapCount) {
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

    // Perform 10 rapid taps
    for (int i = 0; i < 10; i++) {
        press_key_at(TAP_DANCE_KEY, i * 50);
        release_key_at(TAP_DANCE_KEY, i * 50 + 20);
    }
    wait_ms(200);          // Final timeout at t=700ms

    // Expected Output: Still uses last configured action (2nd tap)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 250),
        td_release(3002, 250),
        td_press(3002, 300),
        td_release(3002, 300),
        td_press(3002, 350),
        td_release(3002, 350),
        td_press(3002, 400),
        td_release(3002, 400),
        td_press(3002, 450),
        td_release(3002, 450)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 2, 1);

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

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms (1st tap)
    release_key_at(TAP_DANCE_KEY, 30);     // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);       // t=80ms (2nd tap)
    release_key_at(TAP_DANCE_KEY, 110);    // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);      // t=160ms (3rd tap - overflow)
    press_key_at(INTERRUPTING_KEY, 210);   // t=210ms (interrupt - would trigger hold if available)
    release_key_at(INTERRUPTING_KEY, 260); // t=260ms
    release_key_at(TAP_DANCE_KEY, 310);    // t=310ms
    wait_ms(200);                      // t=510ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 280),
        td_release(3002, 280),
        td_press(INTERRUPTING_KEY, 210),
        td_release(INTERRUPTING_KEY, 260)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.7: Overflow Mixed with Non-Overflow Hold
// Objective: Verify overflow tap behavior mixed with valid hold actions at lower counts
// Configuration: Same as Test 4.4
TEST_F(ActionOverflowTest, OverflowMixedWithNonOverflowHold) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);        // t=0ms (1st tap - hold available)
    wait_ms(250);                      // t=250ms (hold timeout exceeded)
    release_key_at(TAP_DANCE_KEY, 250);    // t=250ms
    wait_ms(50);                       // t=300ms

    // New sequence with overflow
    press_key_at(TAP_DANCE_KEY, 330);      // t=330ms (1st tap)
    release_key_at(TAP_DANCE_KEY, 360);    // t=360ms
    press_key_at(TAP_DANCE_KEY, 410);      // t=410ms (2nd tap)
    release_key_at(TAP_DANCE_KEY, 460);    // t=460ms
    press_key_at(TAP_DANCE_KEY, 510);      // t=510ms (3rd tap)
    release_key_at(TAP_DANCE_KEY, 540);    // t=540ms
    press_key_at(TAP_DANCE_KEY, 590);      // t=590ms (4th tap - overflow)
    release_key_at(TAP_DANCE_KEY, 620);    // t=620ms
    wait_ms(200);                      // t=820ms

    std::vector<uint8_t> expected_layers = {1, 0};  // First sequence - hold action
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 250),
        td_press(3003, 730),
        td_release(3003, 730)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.8: Overflow Boundary - Exactly at Last Configured Action
// Objective: Verify behavior exactly at the boundary of configured actions
TEST_F(ActionOverflowTest, OverflowBoundaryExactlyAtLastConfiguredAction) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Exactly 3 taps (matches last configured action)
    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (exactly at boundary)
    release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
    wait_ms(200);                    // t=390ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3003, 390),
        td_release(3003, 390)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.9: Overflow Boundary - One Beyond Last Configured
// Objective: Verify overflow behavior starts exactly one beyond last configured action
TEST_F(ActionOverflowTest, OverflowBoundaryOneBeyondLastConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // 4 taps (one beyond last configured)
    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms
    release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
    press_key_at(TAP_DANCE_KEY, 240);    // t=240ms (first overflow)
    release_key_at(TAP_DANCE_KEY, 270);  // t=270ms
    wait_ms(200);                    // t=470ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3003, 470),
        td_release(3003, 470)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.10: Overflow with Hold Available at Overflow Count
// Objective: Verify hold action at overflow count when configured
TEST_F(ActionOverflowTest, OverflowWithHoldAvailableAtOverflowCount) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_hold(3, 3, TAP_DANCE_HOLD_PREFERRED)  // Hold at 3rd
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms (3rd tap - hold available)
    wait_ms(250);                    // t=410ms
    release_key_at(TAP_DANCE_KEY, 410);  // t=410ms

    std::vector<uint8_t> expected_layers = {3, 0};  // Hold action available at 3rd tap
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(3, 360),
        td_layer(0, 410)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.11: Immediate Execution Decision Table - Overflow Scenarios
// Objective: Verify immediate vs delayed execution decision in various overflow scenarios
TEST_F(ActionOverflowTest, ImmediateExecutionDecisionTableOverflowScenarios) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Test immediate execution (SENDKEY only, no hold)
    pipeline_tap_dance_action_config_t* actions_immediate[] = {
        createbehaviouraction_tap(1, 3001)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions_immediate, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Input: 3 taps (overflow)
    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    release_key_at(TAP_DANCE_KEY, 20);   // t=20ms
    press_key_at(TAP_DANCE_KEY, 50);     // t=50ms
    release_key_at(TAP_DANCE_KEY, 80);   // t=80ms
    press_key_at(TAP_DANCE_KEY, 120);    // t=120ms (overflow)
    release_key_at(TAP_DANCE_KEY, 150);  // t=150ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 0),
        td_release(3001, 20),
        td_press(3001, 50),
        td_release(3001, 80),
        td_press(3001, 120),
        td_release(3001, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.12: Overflow Reset Verification
// Objective: Verify overflow sequences properly reset tap counts
TEST_F(ActionOverflowTest, OverflowResetVerification) {
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

    // First overflow sequence (5 taps)
    for (int i = 0; i < 5; i++) {
        press_key_at(TAP_DANCE_KEY, i * 50);
        release_key_at(TAP_DANCE_KEY, i * 50 + 20);
    }
    wait_ms(200);          // First sequence completes at t=420ms

    // Second sequence (2 taps - should not be affected by previous overflow)
    press_key_at(TAP_DANCE_KEY, 630);      // Should be 1st tap
    release_key_at(TAP_DANCE_KEY, 660);    // t=660ms
    press_key_at(TAP_DANCE_KEY, 710);      // Should be 2nd tap
    release_key_at(TAP_DANCE_KEY, 740);    // t=740ms
    wait_ms(200);                      // t=940ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 420),
        td_release(3002, 420),
        td_press(3002, 940),
        td_release(3002, 940)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.13: Overflow with Different Action Types
// Objective: Verify overflow works correctly with different action types in sequence
TEST_F(ActionOverflowTest, OverflowWithDifferentActionTypes) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),  // Different key
        createbehaviouraction_tap(3, 3003),  // Third key for overflow
        createbehaviouraction_hold(1, 2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // 4 taps - overflow should use 3rd action (SENDKEY)
    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms
    release_key_at(TAP_DANCE_KEY, 30);   // t=30ms
    press_key_at(TAP_DANCE_KEY, 80);     // t=80ms
    release_key_at(TAP_DANCE_KEY, 110);  // t=110ms
    press_key_at(TAP_DANCE_KEY, 160);    // t=160ms
    release_key_at(TAP_DANCE_KEY, 190);  // t=190ms
    press_key_at(TAP_DANCE_KEY, 240);    // t=240ms (overflow)
    release_key_at(TAP_DANCE_KEY, 270);  // t=270ms
    wait_ms(200);                    // t=470ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3003, 470),
        td_release(3003, 470)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.14: Continuous Overflow - Multiple Sequences
// Objective: Verify consistent overflow behavior across multiple sequences
TEST_F(ActionOverflowTest, ContinuousOverflowMultipleSequences) {
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

    // First overflow sequence
    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 20);
    press_key_at(TAP_DANCE_KEY, 50);
    release_key_at(TAP_DANCE_KEY, 80);
    press_key_at(TAP_DANCE_KEY, 120);
    release_key_at(TAP_DANCE_KEY, 150);  // 3rd tap - overflow
    wait_ms(200);                    // t=350ms

    wait_ms(100);                    // Gap between sequences

    // Second overflow sequence
    press_key_at(TAP_DANCE_KEY, 450);    // t=450ms
    release_key_at(TAP_DANCE_KEY, 480);  // t=480ms
    press_key_at(TAP_DANCE_KEY, 520);    // t=520ms
    release_key_at(TAP_DANCE_KEY, 550);  // t=550ms
    press_key_at(TAP_DANCE_KEY, 590);    // t=590ms
    release_key_at(TAP_DANCE_KEY, 620);  // t=620ms
    press_key_at(TAP_DANCE_KEY, 660);    // t=660ms (4th tap - overflow)
    release_key_at(TAP_DANCE_KEY, 690);  // t=690ms
    wait_ms(200);                    // t=890ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 350),
        td_release(3002, 350),
        td_press(3002, 890),
        td_release(3002, 890)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 4.15: Overflow Edge Case - Zero Configured Actions
// Objective: Verify behavior when no tap actions are configured but taps attempted
// Configuration: Tap actions: [], Hold actions: [1: CHANGELAYER(1)]
TEST_F(ActionOverflowTest, OverflowEdgeCaseZeroConfiguredActions) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Only hold actions, no tap actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);      // t=0ms (no tap action available)
    release_key_at(TAP_DANCE_KEY, 50);   // t=50ms
    wait_ms(200);                    // t=250ms

    std::vector<tap_dance_event_t> expected_events = {};  // No output - no tap actions configured
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
