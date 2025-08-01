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

