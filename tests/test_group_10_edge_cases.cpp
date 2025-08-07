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

class EdgeCasesTest : public ::testing::Test {
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

// Test 10.1: Rapid Fire Stress Test
// Objective: Verify system stability under extremely rapid input sequences
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(EdgeCasesTest, RapidFireStressTest) {
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

    // Input: 50 rapid taps in 500ms (10ms per tap cycle)
    for (int i = 0; i < 50; i++) {
        tap_key(TAP_DANCE_KEY, 1);    // 1ms hold
        wait_ms(9);         // 9ms gap
    }
    wait_ms(200);           // Final timeout

    // Expected: Uses second action (overflow from 50 taps)
    std::vector<key_action_t> expected_keys = {
        press(3002, 700), release(3002, 700)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 10.2: Zero-Duration Input Patterns
// Objective: Verify handling of instantaneous press/release patterns
// Configuration: Same as Test 10.1
TEST_F(EdgeCasesTest, ZeroDurationSingleTap) {
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

    // Input: tap_key(TAP_DANCE_KEY, 0); platform_wait_ms(200);
    tap_key(TAP_DANCE_KEY, 0);       // t=0ms (instantaneous)
    wait_ms(200);          // t=200ms

    // Expected: Immediate execution
    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 200)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 10.16: Final System Integrity Check
// Objective: Verify system maintains integrity after all stress tests
// Configuration: Basic configuration
TEST_F(EdgeCasesTest, FinalSystemIntegrityCheck) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: Simple verification sequence
    tap_key(TAP_DANCE_KEY, 50);      // Basic tap
    wait_ms(300);          // Clean gap
    press_key(TAP_DANCE_KEY);        // Basic hold
    wait_ms(250);
    release_key(TAP_DANCE_KEY);

    // Expected: Perfect tap and hold
    std::vector<key_action_t> expected_keys = {
        press(3001, 350), release(3001, 350)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}
