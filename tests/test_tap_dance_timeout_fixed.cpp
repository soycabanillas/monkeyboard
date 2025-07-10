#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.h"

extern "C" {
#include "../src/keycodes.h"
#include "commons.h"
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

// Define the missing keycodes for the test
#define CKC_LAY_MOUSE_Q 0x7E00
#define CKC_LAY_NUMBERS_R 0x7E01
#define CKC_LAY_MOVEMENT_F 0x7E02
#define CKC_EXCL 0x7E03
#define CKC_QUES 0x7E04
#define CKC_LAY_RIGHT_THUMB 0x7E05
#define CKC_LSHIFT_EXCLAMATION_MARK 0x7E06
#define CKC_RSHIFT_QUESTION_MARK 0x7E07
class TapDanceTimeoutTest : public ::testing::Test {
protected:

    pipeline_tap_dance_global_config_t* global_config;
    // Mock layers array for testing
    uint8_t test_layers[10] = {0}; // Simple test layers array

    void SetUp() override {
        reset_mock_state();

        // Create minimal test configuration for the tap dance keys
        size_t n_pipelines = 3;
        pipeline_executor_config = static_cast<pipeline_executor_config_t*>(malloc(sizeof(pipeline_executor_config_t) + n_pipelines * sizeof(pipeline_t*)));
        pipeline_executor_config->length = n_pipelines;
        pipeline_executor_global_state_create();

        size_t n_elements = 6;

        pipeline_tap_dance_global_config_t* tap_dance_global_config = static_cast<pipeline_tap_dance_global_config_t*>(malloc(sizeof(*tap_dance_global_config) + n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));
        tap_dance_global_config->length = n_elements;

        pipeline_tap_dance_action_config_t* custom_actions[] =
            {
                createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_Q, _LMOUSE),
                createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_Q, _LMOUSE)
            };
        tap_dance_global_config->behaviours[0] = createbehaviour(CKC_LAY_MOUSE_Q, custom_actions, 2);

        pipeline_tap_dance_action_config_t* custom_actions2[] =
            {
                createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_R, _LNUMBERS),
                createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, S(KC_R), _LNUMBERS),
                createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_R, _LNUMBERS)
            };
        tap_dance_global_config->behaviours[1] = createbehaviour(CKC_LAY_NUMBERS_R, custom_actions2, 3);

        pipeline_tap_dance_action_config_t* custom_actions3[] =
            {
                createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_F, _LNUMBERS),
                createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_F, _LMOVEMENT),
                createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_F, _LMOUSE)
            };
        tap_dance_global_config->behaviours[2] = createbehaviour(CKC_LAY_MOVEMENT_F, custom_actions3, 3);

        pipeline_tap_dance_action_config_t* custom_actions4[] =
            {
                createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_EXLM, _LMOUSE),
                createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, RALT(KC_1), _LNUMBERS),
            };
        tap_dance_global_config->behaviours[3] = createbehaviour(CKC_EXCL, custom_actions4, 2);

        pipeline_tap_dance_action_config_t* custom_actions5[] =
            {
                createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_QUES, _LNUMBERS),
                createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, RALT(KC_SLSH), _LNUMBERS),
            };
        tap_dance_global_config->behaviours[4] = createbehaviour(CKC_QUES, custom_actions5, 2);

        pipeline_tap_dance_action_config_t* custom_actions6[] =
            {
                createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_QUES, _LRIGHT_THUMB),
            };
        tap_dance_global_config->behaviours[5] = createbehaviour(CKC_LAY_RIGHT_THUMB, custom_actions6, 1);

        pipeline_tap_dance_global_state_create();
        pipeline_executor_config->pipelines[1] = add_pipeline(&pipeline_tap_dance_callback, tap_dance_global_config);
    }

    void TearDown() override {
    }

    void simulate_key_event(uint16_t keycode, bool pressed, uint16_t time_offset = 0) {
        abskeyevent_t event = {
            .key = {.col = 0, .row = 0},
            .pressed = pressed,
            .time = static_cast<uint16_t>(g_mock_state.time + time_offset)
        };
        if (time_offset > 0) {
            platform_wait_ms(time_offset);
        }
        pipeline_process_key(keycode, event);
    }
};

;

// Test that hold timeout is respected (200ms as defined in g_tap_timeout)
TEST_F(TapDanceTimeoutTest, HoldTimeoutRespected) {
    g_mock_state.layer_on_calls.clear();

    simulate_key_event(CKC_LAY_MOUSE_Q, true);

    // Just before timeout (199ms)
    platform_wait_ms(199);
    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 0);

    // Just after timeout (1ms more = 200ms total) - simulate layer activation
    platform_wait_ms(1);
    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 1);
}

// Test tap timeout for multiple taps
TEST_F(TapDanceTimeoutTest, TapTimeoutForMultipleTaps) {
    g_mock_state.tap_code_calls.clear();

    // First tap
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);

    // Count initial calls from first tap
    size_t initial_calls = g_mock_state.tap_code_calls.size();

    // Wait just under timeout
    platform_wait_ms(149); // Total 199ms
    // Tap count should not increase during wait
    EXPECT_EQ(g_mock_state.tap_code_calls.size(), initial_calls);

    // Second tap within timeout
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);

    // Now wait for timeout to trigger double-tap action
    platform_wait_ms(200);

    // Should have called tap_code for the double-tap action
    EXPECT_EQ(g_mock_state.tap_code_calls.size(), initial_calls + 1);
    EXPECT_EQ(g_mock_state.last_tapped_code, S(KC_R));
}

// Test that taps outside timeout window are treated separately
TEST_F(TapDanceTimeoutTest, TapsOutsideTimeoutTreatedSeparately) {
    g_mock_state.tap_code_calls.clear();

    // First tap
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);

    // Wait past timeout
    platform_wait_ms(250);

    // Should trigger single tap action
    EXPECT_EQ(g_mock_state.tap_code_calls.size(), 1);
    EXPECT_EQ(g_mock_state.last_tapped_code, KC_R);

    // Second tap after timeout
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);
    platform_wait_ms(250);

    // Should trigger another single tap action
    EXPECT_EQ(g_mock_state.tap_code_calls.size(), 2);
    EXPECT_EQ(g_mock_state.last_tapped_code, KC_R);
}

// Test early timeout cancellation when key is released quickly
TEST_F(TapDanceTimeoutTest, EarlyTimeoutCancellationOnQuickRelease) {
    g_mock_state.layer_on_calls.clear();
    g_mock_state.tap_code_calls.clear();

    simulate_key_event(CKC_LAY_MOUSE_Q, true);

    // Release before timeout
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 100);

    // Should not activate layer
    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 0);

    // Wait past original timeout to trigger tap
    platform_wait_ms(150);

    // Should trigger tap action instead
    EXPECT_EQ(g_mock_state.tap_code_calls.size(), 1);
    EXPECT_EQ(g_mock_state.last_tapped_code, KC_Q);
    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 0);
}

// Test precise timing of deferred execution
TEST_F(TapDanceTimeoutTest, PreciseTimingOfDeferredExecution) {
    g_mock_state.layer_on_calls.clear();

    simulate_key_event(CKC_LAY_MOUSE_Q, true);

    // Check that exactly at timeout threshold, action triggers
    platform_wait_ms(200); // Exactly at g_tap_timeout

    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 1);
}

// Test timeout behavior with multiple rapid taps
TEST_F(TapDanceTimeoutTest, TimeoutWithMultipleRapidTaps) {
    g_mock_state.tap_code_calls.clear();

    // Three rapid taps
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 30);
    platform_wait_ms(50);

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 30);
    platform_wait_ms(50);

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 30);

    // Wait for timeout
    platform_wait_ms(200);

    // Should handle gracefully (likely fallback to single tap)
    EXPECT_GE(g_mock_state.tap_code_calls.size(), 1);
}

// Test timeout cancellation when interrupted by another key
TEST_F(TapDanceTimeoutTest, TimeoutCancellationOnInterruption) {
    g_mock_state.layer_on_calls.clear();
    g_mock_state.tap_code_calls.clear();

    simulate_key_event(CKC_LAY_MOUSE_Q, true);

    // Interrupt with another key before timeout
    platform_wait_ms(100);
    simulate_key_event(KC_Q, true);
    simulate_key_event(KC_Q, false, 50);

    // Release original key
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    // Should not activate layer due to interruption
    platform_wait_ms(100);
    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 0);

    // Should eventually trigger tap action
    EXPECT_EQ(g_mock_state.tap_code_calls.size(), 1);
}
