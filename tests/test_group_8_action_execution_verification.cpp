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

class ActionExecutionVerificationTest : public ::testing::Test {
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

// Test 8.1: Basic SENDKEY Action Execution
// Objective: Verify basic `TDCL_TAP_KEY_SENDKEY` press/release sequence
// Configuration: TAP_DANCE_KEY = 3000, OUTPUT_KEY = 3001, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(OUTPUT_KEY)], Hold actions: []
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ActionExecutionVerificationTest, BasicSendkeyActionExecution) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
    wait_ms(200);          // t=250ms

    // Expected Output: Immediate press (no hold configured), release follows input timing
    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY, 0), release(OUTPUT_KEY, 50)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 8.2: Basic CHANGELAYERTEMPO Action Execution
// Objective: Verify basic `TDCL_HOLD_KEY_CHANGELAYERTEMPO` layer activation/deactivation
// Configuration: TAP_DANCE_KEY = 3000, TARGET_LAYER = 1, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYERTEMPO(TARGET_LAYER)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ActionExecutionVerificationTest, BasicChangelayertempoActionExecution) {
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

    press_key(TAP_DANCE_KEY);        // t=0ms
    wait_ms(250);          // t=250ms (exceed hold timeout)
    release_key(TAP_DANCE_KEY);      // t=250ms

    // Expected Output: Layer activation at hold timeout, deactivation on key release
    std::vector<uint8_t> expected_layers = {TARGET_LAYER, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 8.8: Action Parameter Validation
// Objective: Verify actions execute with correct parameters
// Configuration: Tap actions: [1: SENDKEY(0x41)] // 'A' key, Hold actions: [1: CHANGELAYERTEMPO(3)] // Layer 3
TEST_F(ActionExecutionVerificationTest, ActionParameterValidation) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 0x41),  // 'A' key
        createbehaviouraction_hold(1, 3, TAP_DANCE_HOLD_PREFERRED)  // Layer 3
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Test SENDKEY Parameter
    tap_key(TAP_DANCE_KEY, 50);
    wait_ms(200);

    std::vector<key_action_t> expected_keys = {
        press(0x41, 200), release(0x41, 200)  // 'A' key
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    reset_mock_state();

    // Test CHANGELAYERTEMPO Parameter
    press_key(TAP_DANCE_KEY);
    wait_ms(250);
    release_key(TAP_DANCE_KEY);

    std::vector<uint8_t> expected_layers = {3, 0};  // Layer 3
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 8.15: Action Execution Performance - Rapid Sequences
// Objective: Verify action execution performance with rapid sequences
// Configuration: Same as Test 8.1 (immediate execution)
TEST_F(ActionExecutionVerificationTest, ActionExecutionPerformanceRapidSequences) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    // 10 rapid tap sequences
    for (int i = 0; i < 10; i++) {
        tap_key(TAP_DANCE_KEY, 5);   // Very fast taps
        wait_ms(10);       // Minimal gap
    }

    // Expected Output: 10 pairs of press/release events at precise timing
    std::vector<key_action_t> expected_keys;
    for (int i = 0; i < 10; i++) {
        expected_keys.push_back(press(OUTPUT_KEY, i * 15));      // t = i*15
        expected_keys.push_back(release(OUTPUT_KEY, 5));         // 5ms gap
    }
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 8.16: Action State Cleanup Verification
// Objective: Verify proper cleanup of action states between sequences
// Configuration: Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYER(1)]
TEST_F(ActionExecutionVerificationTest, ActionStateCleanupVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // First sequence - hold with early termination
    press_key(TAP_DANCE_KEY);        // t=0ms
    wait_ms(250);          // t=250ms (layer activated)
    release_key(TAP_DANCE_KEY);      // t=250ms (layer deactivated)

    // Immediate second sequence - should start clean
    press_key(TAP_DANCE_KEY);        // t=250ms
    release_key(TAP_DANCE_KEY, 50);  // t=300ms (tap, not hold)
    wait_ms(200);          // t=500ms

    std::vector<key_action_t> expected_keys = {
        press(3001, 0), release(3001, 50)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // First sequence hold, second sequence tap
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}


