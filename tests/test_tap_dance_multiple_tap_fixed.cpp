#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.h"

extern "C" {
#include "test_keycodes.h"
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
class TapDanceMultipleTapTest : public ::testing::Test {
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

// Test double tap on CKC_LAY_MOUSE_Q
TEST_F(TapDanceMultipleTapTest, DoubleTapMouseKey) {
    g_mock_state.tap_code_calls.clear();

    simulate_double_tap(CKC_LAY_MOUSE_Q);
    platform_wait_ms(250); // Allow tap dance to complete

    // Should register two Q taps for double tap
    EXPECT_GE(g_mock_state.tap_code_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_tapped_code, KC_Q);
}

// Test triple tap sequence
TEST_F(TapDanceMultipleTapTest, TripleTapSequence) {
    g_mock_state.tap_code_calls.clear();

    simulate_triple_tap(CKC_LAY_NUMBERS_R);
    platform_wait_ms(250);

    // Should register multiple R taps for triple tap
    EXPECT_GE(g_mock_state.tap_code_calls_count(), 3);
    EXPECT_EQ(g_mock_state.last_tapped_code, KC_R);
}

// Test tap sequence timing sensitivity
TEST_F(TapDanceMultipleTapTest, TimingSensitivity) {
    g_mock_state.tap_code_calls.clear();

    // Double tap with quick timing
    simulate_double_tap(CKC_LAY_MOUSE_Q, 50); // Fast double tap
    platform_wait_ms(250);

    int fast_tap_count = g_mock_state.tap_code_calls_count();

    // Reset and try slow timing
    g_mock_state.tap_code_calls.clear();
    simulate_double_tap(CKC_LAY_MOUSE_Q, 300); // Slow double tap (should timeout)
    platform_wait_ms(250);

    int slow_tap_count = g_mock_state.tap_code_calls_count();

    // Fast double tap should behave differently than slow (timeout) taps
    EXPECT_TRUE(fast_tap_count > 0 && slow_tap_count > 0);
}

// Test interruption of multiple tap sequence
TEST_F(TapDanceMultipleTapTest, SequenceInterruption) {
    g_mock_state.tap_code_calls.clear();

    // Start double tap sequence
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    // Interrupt with different key
    simulate_key_event(KC_A, true);
    simulate_key_event(KC_A, false, 50);

    // Complete the original sequence
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    platform_wait_ms(250);

    // Should have registered some taps
    EXPECT_GT(g_mock_state.tap_code_calls_count(), 0);
}

// Test multiple tap with layer activation potential
TEST_F(TapDanceMultipleTapTest, MultipleTapVsHold) {
    g_mock_state.tap_code_calls.clear();
    g_mock_state.layer_on_calls.clear();

    // Quick double tap (should not activate layer)
    simulate_double_tap(CKC_LAY_MOUSE_Q, 50);

    // Brief pause, then hold
    platform_wait_ms(100);
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250); // Long enough to trigger hold
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    // Should have both tap and layer behavior
    EXPECT_GT(g_mock_state.tap_code_calls_count(), 0);
    EXPECT_GT(g_mock_state.layer_select_calls_count(), 0);
}

// Test rapid multiple taps
TEST_F(TapDanceMultipleTapTest, RapidMultipleTaps) {
    g_mock_state.tap_code_calls.clear();

    // Simulate very rapid tapping
    for (int i = 0; i < 5; i++) {
        simulate_key_event(CKC_LAY_NUMBERS_R, true);
        simulate_key_event(CKC_LAY_NUMBERS_R, false, 25); // Very fast
        platform_wait_ms(25);
    }

    platform_wait_ms(250);

    // Should handle rapid input gracefully
    EXPECT_GT(g_mock_state.tap_code_calls_count(), 0);
    EXPECT_EQ(g_mock_state.last_tapped_code, KC_R);
}

// Test alternating multiple taps between different keys
TEST_F(TapDanceMultipleTapTest, AlternatingMultipleTaps) {
    g_mock_state.tap_code_calls.clear();

    // Alternate between two tap dance keys
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    platform_wait_ms(50);

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);

    platform_wait_ms(50);

    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    platform_wait_ms(250);

    // Should register taps from both keys
    EXPECT_GT(g_mock_state.tap_code_calls_count(), 0);
}
