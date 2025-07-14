#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"

extern "C" {
#include "keycodes.h"
#include "commons.h"
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

// Test keycodes
#define TEST_KEY_1 0x7E00
#define TEST_KEY_2 0x7E01
#define TEST_KEY_3 0x7E02

class TapDanceComprehensiveSimpleTest : public ::testing::Test {
protected:
    pipeline_tap_dance_global_config_t* global_config;

    void SetUp() override {
        reset_mock_state();

        // Create pipeline executor
        size_t n_pipelines = 3;
        pipeline_executor_config = static_cast<pipeline_executor_config_t*>(
            malloc(sizeof(pipeline_executor_config_t) + n_pipelines * sizeof(pipeline_t*)));
        pipeline_executor_config->length = n_pipelines;

        // Initialize all pipeline slots to NULL
        for (size_t i = 0; i < n_pipelines; i++) {
            pipeline_executor_config->pipelines[i] = nullptr;
        }

        pipeline_executor_global_state_create();

        // Create tap dance configuration
        size_t n_elements = 3;
        global_config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*global_config) + n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));
        global_config->length = n_elements;

        // Configure TEST_KEY_1: 1 tap = KC_A, 2 taps = KC_B, hold = layer 1
        pipeline_tap_dance_action_config_t* actions1[] = {
            createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, KC_A, 0),
            createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, KC_B, 0),
            createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_A, 1)
        };
        global_config->behaviours[0] = createbehaviour(TEST_KEY_1, actions1, 3);

        // Configure TEST_KEY_2: 1 tap = KC_X, hold = layer 2, interrupt config = 0
        pipeline_tap_dance_action_config_t* actions2[] = {
            createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, KC_X, 0),
            createbehaviouraction_with_interrupt(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_X, 2, 0)
        };
        global_config->behaviours[1] = createbehaviour(TEST_KEY_2, actions2, 2);

        // Configure TEST_KEY_3: 1 tap = KC_Y, hold = layer 3, interrupt config = -1
        pipeline_tap_dance_action_config_t* actions3[] = {
            createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, KC_Y, 0),
            createbehaviouraction_with_interrupt(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_Y, 3, -1)
        };
        global_config->behaviours[2] = createbehaviour(TEST_KEY_3, actions3, 2);

        pipeline_tap_dance_global_state_create();
        pipeline_executor_config->pipelines[1] = add_pipeline(&pipeline_tap_dance_callback, global_config);
    }

    void TearDown() override {
        // Cleanup allocated memory
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
        if (global_config) {
            free(global_config);
            global_config = nullptr;
        }
    }

    void simulate_key_event(uint16_t keycode, bool pressed, uint16_t time_offset = 0) {
        if (time_offset > 0) {
            platform_wait_ms(time_offset);
        }

        abskeyevent_t event;
        event.key.row = 0;
        event.key.col = 0;
        event.pressed = pressed;
        event.time = static_cast<uint16_t>(platform_timer_read());

        pipeline_process_key(keycode, event);
    }
};

// Test 1: Basic single tap
TEST_F(TapDanceComprehensiveSimpleTest, BasicSingleTap) {
    simulate_key_event(TEST_KEY_1, true);    // Press
    simulate_key_event(TEST_KEY_1, false, 50);  // Release after 50ms
    platform_wait_ms(250);  // Wait past timeout (total 300ms)

    // Should have sent KC_A
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}

// Test 2: Basic hold timeout
TEST_F(TapDanceComprehensiveSimpleTest, BasicHoldTimeout) {
    simulate_key_event(TEST_KEY_1, true);    // Press
    platform_wait_ms(250);  // Wait for hold timeout
    simulate_key_event(TEST_KEY_1, false);   // Release

    // Should have changed to layer 1
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, 1);
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 0);
}

// Test 3: Double tap
TEST_F(TapDanceComprehensiveSimpleTest, DoubleTap) {
    simulate_key_event(TEST_KEY_1, true);     // First press
    simulate_key_event(TEST_KEY_1, false, 50); // First release
    simulate_key_event(TEST_KEY_1, true, 50);  // Second press
    simulate_key_event(TEST_KEY_1, false, 50); // Second release
    platform_wait_ms(250);  // Wait past timeout

    // Should have sent KC_B (double tap action)
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_B);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}

// Test 4: Hold released before timeout (should act as tap)
TEST_F(TapDanceComprehensiveSimpleTest, HoldReleasedBeforeTimeout) {
    simulate_key_event(TEST_KEY_1, true);     // Press
    simulate_key_event(TEST_KEY_1, false, 150); // Release before timeout (150ms < 200ms)
    platform_wait_ms(250);  // Wait past timeout

    // Should have sent KC_A (tap action)
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}

// Test 5: Interruption with config = 0 (allow interruption)
TEST_F(TapDanceComprehensiveSimpleTest, InterruptConfigZero) {
    simulate_key_event(TEST_KEY_2, true);     // Press key with interrupt_config = 0
    simulate_key_event(TEST_KEY_1, true, 50); // Press different key (interrupt)
    simulate_key_event(TEST_KEY_1, false, 50); // Release interrupt key
    simulate_key_event(TEST_KEY_2, false, 50); // Release original key

    // TEST_KEY_2 should have been interrupted and executed its tap action
    EXPECT_GT(g_mock_state.send_key_calls_count(), 0);
    // Should include KC_X from the interrupted key
    bool found_kc_x = false;
    for (const auto& call : g_mock_state.send_key_calls) {
        if (call == KC_X) {
            found_kc_x = true;
            break;
        }
    }
    EXPECT_TRUE(found_kc_x);
}

// Test 6: No interruption with config = -1
TEST_F(TapDanceComprehensiveSimpleTest, InterruptConfigMinus1) {
    simulate_key_event(TEST_KEY_3, true);     // Press key with interrupt_config = -1
    simulate_key_event(TEST_KEY_1, true, 50); // Try to interrupt
    simulate_key_event(TEST_KEY_1, false, 50); // Release interrupt key
    simulate_key_event(TEST_KEY_3, false, 50); // Release original key
    platform_wait_ms(250);  // Wait past timeout

    // TEST_KEY_3 should NOT have been interrupted, should execute normally
    EXPECT_GT(g_mock_state.send_key_calls_count(), 0);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_Y);
}

// Test 7: Different keycodes can nest
TEST_F(TapDanceComprehensiveSimpleTest, DifferentKeycodesCanNest) {
    simulate_key_event(TEST_KEY_1, true);     // Press first key
    simulate_key_event(TEST_KEY_2, true, 50); // Nest second key
    simulate_key_event(TEST_KEY_2, false, 50); // Release second key
    platform_wait_ms(250);  // Wait past timeout for second key
    simulate_key_event(TEST_KEY_1, false);    // Release first key
    platform_wait_ms(250);  // Wait past timeout for first key

    // Both keys should have executed their tap actions
    EXPECT_GE(g_mock_state.send_key_calls_count(), 2);
}

// Test 8: Very fast tap/release sequence
TEST_F(TapDanceComprehensiveSimpleTest, VeryFastTapRelease) {
    simulate_key_event(TEST_KEY_1, true);     // Press
    simulate_key_event(TEST_KEY_1, false, 10); // Very quick release (10ms)
    platform_wait_ms(250);  // Wait past timeout

    // Should still register as a tap
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
}

// Test 9: No action configured (regular key)
TEST_F(TapDanceComprehensiveSimpleTest, NoActionConfigured) {
    simulate_key_event(KC_Z, true);     // Regular key not in tap dance config
    simulate_key_event(KC_Z, false, 50);
    platform_wait_ms(250);

    // Should have no tap dance effects
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 0);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}

// Test 10: Tap count exceeds configuration
TEST_F(TapDanceComprehensiveSimpleTest, TapCountExceedsConfiguration) {
    // TEST_KEY_1 only has 1-tap and 2-tap configurations, try 3 taps
    simulate_key_event(TEST_KEY_1, true);     // First tap
    simulate_key_event(TEST_KEY_1, false, 50);
    simulate_key_event(TEST_KEY_1, true, 50);  // Second tap
    simulate_key_event(TEST_KEY_1, false, 50);
    simulate_key_event(TEST_KEY_1, true, 50);  // Third tap (exceeds config)
    simulate_key_event(TEST_KEY_1, false, 50);
    platform_wait_ms(250);

    // Should execute the highest configured tap count (2-tap = KC_B)
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_B);
}
