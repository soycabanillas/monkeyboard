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

class BasicStateMachineTest : public ::testing::Test {
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

// Test 1.1: Simple Tap (IDLE → WAITING_FOR_HOLD → WAITING_FOR_TAP → IDLE)
// Objective: Verify basic tap sequence with release before hold timeout
// Configuration: TAP_DANCE_KEY = 3000, OUTPUT_KEY = 3001, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(OUTPUT_KEY)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(BasicStateMachineTest, SimpleTap) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms (before hold timeout)
    platform_wait_ms(200);          // t=300ms (tap timeout)

    // Expected Output: Tap action executed at timeout
    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY, 300), release(OUTPUT_KEY, 300)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 1.2: Simple Hold (IDLE → WAITING_FOR_HOLD → HOLDING → IDLE)
// Objective: Verify basic hold sequence with timeout triggering hold action
// Configuration: Same as Test 1.1
TEST_F(BasicStateMachineTest, SimpleHold) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    // Setup same as SimpleTap
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

    // Expected Output: Layer activation at timeout, deactivation on release
    std::vector<key_action_t> expected_keys = {};
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0}; // BASE_LAYER = 0
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 1.3: Hold Timeout Boundary - Just Before
// Objective: Verify tap behavior when released exactly at hold timeout boundary
// Configuration: Same as Test 1.1
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryJustBefore) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 199); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 199); // t=199ms (1ms before timeout)
    platform_wait_ms(200);          // Wait for tap timeout

    // Expected Output: Tap action (released before hold timeout)
    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY, 399), release(OUTPUT_KEY, 399)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 1.4: Hold Timeout Boundary - Exactly At
// Objective: Verify hold behavior when timeout occurs exactly at boundary
// Configuration: Same as Test 1.1
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryExactlyAt) {
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

    // Input Sequence: press_key(TAP_DANCE_KEY); platform_wait_ms(200); release_key(TAP_DANCE_KEY);
    press_key(TAP_DANCE_KEY);        // t=0ms
    platform_wait_ms(200);          // t=200ms (exactly at timeout)
    release_key(TAP_DANCE_KEY);      // t=200ms

    // Expected Output: Hold action (timeout reached exactly)
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 1.5: Hold Timeout Boundary - Just After
// Objective: Verify hold behavior when held past timeout
// Configuration: Same as Test 1.1
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryJustAfter) {
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

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 201);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 201); // t=201ms (1ms after timeout)

    // Expected Output: Hold action at timeout, deactivation at release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 1.9: No Hold Action Configured - Immediate Execution
// Objective: Verify immediate execution when no hold action available
// Configuration: Tap actions: [1: SENDKEY(OUTPUT_KEY)], Hold actions: [] // No hold actions
TEST_F(BasicStateMachineTest, NoHoldActionConfiguredImmediateExecution) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Only tap actions, no hold actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms

    // Expected Output: Immediate execution on press, release follows input timing
    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY, 0), release(OUTPUT_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 1.10: Only Hold Action Configured
// Objective: Verify behavior when only hold action is configured
// Configuration: Tap actions: [] // No tap actions, Hold actions: [1: CHANGELAYER(1)]
TEST_F(BasicStateMachineTest, OnlyHoldActionConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    // Only hold actions, no tap actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); release_key(TAP_DANCE_KEY, 100); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);        // t=0ms
    release_key(TAP_DANCE_KEY, 100); // t=100ms (before timeout)
    platform_wait_ms(200);          // Wait for potential processing

    // Expected Output: No output - no tap action configured and hold timeout not reached
    std::vector<key_action_t> expected_keys = {};
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
}

// Test 1.11: Only Hold Action - Timeout Reached
// Objective: Verify hold action executes when only hold configured and timeout reached
// Configuration: Same as Test 1.10
TEST_F(BasicStateMachineTest, OnlyHoldActionTimeoutReached) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    // Input Sequence: press_key(TAP_DANCE_KEY); platform_wait_ms(250); release_key(TAP_DANCE_KEY);
    press_key(TAP_DANCE_KEY);        // t=0ms
    platform_wait_ms(250);          // Exceed hold timeout
    release_key(TAP_DANCE_KEY);      // t=250ms

    // Expected Output: Hold action activation/deactivation
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}
