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
            malloc(sizeof(*tap_dance_config)));
        tap_dance_config->length = 0;
        tap_dance_config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

        pipeline_executor_create_config(1, 0);
        pipeline_executor_add_physical_pipeline(0, &pipeline_tap_dance_callback_process_data_executor, &pipeline_tap_dance_callback_reset_executor, tap_dance_config);
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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

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

    // Input: 50 rapid taps in 500ms (10ms per tap cycle)
    for (int i = 0; i < 50; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 10);     // t=i*10ms
        keyboard.press_key_at(TAP_DANCE_KEY, i * 10 + 1); // t=i*10+1ms (1ms hold)
        keyboard.wait_ms(9);                          // 9ms gap
    }
    keyboard.wait_ms(200);                            // Final timeout

    // Expected: Uses second action (overflow from 50 taps)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 700), td_release(3002, 700)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 10.2: Zero-Duration Input Patterns
// Objective: Verify handling of instantaneous press/release patterns
// Configuration: Same as Test 10.1
TEST_F(EdgeCasesTest, ZeroDurationSingleTap) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

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

    // Input: keyboard.tap_key(TAP_DANCE_KEY, 0); platform_wait_ms(200);
    keyboard.press_key_at(TAP_DANCE_KEY, 0);       // t=0ms (instantaneous)
    keyboard.release_key_at(TAP_DANCE_KEY, 0);     // t=0ms
    keyboard.wait_ms(200);                     // t=200ms

    // Expected: Delayed execution (hold action available)
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 200), td_release(3001, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 10.16: Final System Integrity Check
// Objective: Verify system maintains integrity after all stress tests
// Configuration: Basic configuration
TEST_F(EdgeCasesTest, FinalSystemIntegrityCheck) {
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

    // Input: Simple verification sequence
    keyboard.press_key_at(TAP_DANCE_KEY, 0);      // Basic tap
    keyboard.release_key_at(TAP_DANCE_KEY, 50);   // t=50ms
    keyboard.wait_ms(300);                    // Clean gap to t=350ms
    keyboard.press_key_at(TAP_DANCE_KEY, 350);    // Basic hold
    keyboard.wait_ms(250);                    // t=600ms
    keyboard.release_key_at(TAP_DANCE_KEY, 600);  // t=600ms

    // Expected: Perfect tap and hold
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 250), td_release(3001, 250),  // Tap action
        td_layer(1, 550), td_layer(0, 600)                        // Hold action
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}
