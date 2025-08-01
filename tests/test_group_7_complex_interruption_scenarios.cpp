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

class ComplexInterruptionScenariosTest : public ::testing::Test {
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

// Test 7.1: Multiple Sequential Interruptions - TAP_PREFERRED
// Objective: Verify multiple interrupting keys are all ignored with TAP_PREFERRED strategy
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYER(1)]
// INTERRUPTING_KEY_1 = 3010, INTERRUPTING_KEY_2 = 3011
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ComplexInterruptionScenariosTest, MultipleSequentialInterruptionsTapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms (first interrupt)
    press_key(INTERRUPTING_KEY_2, 40);   // t=70ms (second interrupt)
    release_key(INTERRUPTING_KEY_1, 30); // t=100ms
    release_key(INTERRUPTING_KEY_2, 50); // t=150ms
    release_key(TAP_DANCE_KEY, 30);      // t=180ms (before hold timeout)
    platform_wait_ms(200);              // t=380ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30), release(INTERRUPTING_KEY_1, 100),
        press(INTERRUPTING_KEY_2, 70), release(INTERRUPTING_KEY_2, 150),
        press(3001, 380), release(3001, 380)  // Tap action (all interruptions ignored)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.2: Multiple Sequential Interruptions - BALANCED
// Objective: Verify BALANCED strategy triggers hold on first complete press/release cycle
// Configuration: Same as Test 7.1, but Strategy: BALANCED
TEST_F(ComplexInterruptionScenariosTest, MultipleSequentialInterruptionsBalanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
    press_key(INTERRUPTING_KEY_2, 20);   // t=50ms
    release_key(INTERRUPTING_KEY_1, 30); // t=80ms (first complete cycle)
    release_key(INTERRUPTING_KEY_2, 40); // t=120ms (second complete cycle)
    release_key(TAP_DANCE_KEY, 30);      // t=150ms
    platform_wait_ms(200);              // t=350ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30), release(INTERRUPTING_KEY_1, 80),
        press(INTERRUPTING_KEY_2, 50), release(INTERRUPTING_KEY_2, 120),
        release(3001, 150)  // Tap action (hold triggered by first complete cycle)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.3: Multiple Sequential Interruptions - HOLD_PREFERRED
// Objective: Verify HOLD_PREFERRED triggers hold on first key press
// Configuration: Same as Test 7.1, but Strategy: HOLD_PREFERRED
TEST_F(ComplexInterruptionScenariosTest, MultipleSequentialInterruptionsHoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms (first interrupt - triggers hold)
    press_key(INTERRUPTING_KEY_2, 20);   // t=50ms (second interrupt - ignored)
    release_key(INTERRUPTING_KEY_1, 30); // t=80ms
    release_key(INTERRUPTING_KEY_2, 40); // t=120ms
    release_key(TAP_DANCE_KEY, 30);      // t=150ms
    platform_wait_ms(200);              // t=350ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30),
        release(INTERRUPTING_KEY_1, 80),
        press(3001, 80), release(3001, 80),  // Hold action triggered immediately
        release(3001, 150)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.4: Rapid Interruption Sequence
// Objective: Verify system handles very rapid interruption patterns
// Configuration: Same as Test 7.1, Strategy: BALANCED
TEST_F(ComplexInterruptionScenariosTest, RapidInterruptionSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    // Rapid fire interruptions
    press_key(INTERRUPTING_KEY_1, 10);   // t=10ms
    release_key(INTERRUPTING_KEY_1, 5);  // t=15ms (very fast complete cycle)
    press_key(INTERRUPTING_KEY_2, 5);    // t=20ms
    release_key(INTERRUPTING_KEY_2, 5);  // t=25ms (second fast cycle)
    release_key(TAP_DANCE_KEY, 25);      // t=50ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 10), release(INTERRUPTING_KEY_1, 15),
        press(INTERRUPTING_KEY_2, 20), release(INTERRUPTING_KEY_2, 25)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0}; // Hold triggered by first rapid cycle
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.11: Complex Interruption Pattern - Nested Timing
// Objective: Verify handling of complex nested interruption patterns
// Configuration: Same as Test 7.1, Strategy: BALANCED
TEST_F(ComplexInterruptionScenariosTest, ComplexInterruptionPatternNestedTiming) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 20);   // t=20ms
      press_key(INTERRUPTING_KEY_2, 10); // t=30ms (nested interrupt)
      release_key(INTERRUPTING_KEY_2, 20); // t=50ms (nested complete)
    release_key(INTERRUPTING_KEY_1, 30); // t=80ms (first complete)
    release_key(TAP_DANCE_KEY, 20);      // t=100ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 20),
        press(INTERRUPTING_KEY_2, 30), release(INTERRUPTING_KEY_2, 50),
        release(INTERRUPTING_KEY_1, 80)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0}; // Hold triggered by first complete cycle (nested)
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

