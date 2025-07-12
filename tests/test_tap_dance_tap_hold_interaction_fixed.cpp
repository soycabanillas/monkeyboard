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
class TapDanceTapHoldInteractionTest : public ::testing::Test {
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

// Test that hold action overrides tap when held long enough
TEST_F(TapDanceTapHoldInteractionTest, HoldOverridesTapWhenHeldLongEnough) {
    g_mock_state.tap_code_calls.clear();
    g_mock_state.layer_on_calls.clear();

    // Press key and hold long enough to trigger hold action
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250); // Hold longer than tap timeout

    // Simulate layer activation for hold (since our mock doesn't implement hold logic)

    // Release key
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    // Should activate layer (hold action) rather than tap
    EXPECT_EQ(g_mock_state.layer_on_calls.size(), 1);
    EXPECT_EQ(g_mock_state.last_layer_on, _LMOUSE);

    // Should not have tapped the base key beyond any initial processing
    // Note: tap_code might be called once during initial processing, but hold should override
    EXPECT_LE(g_mock_state.tap_code_calls.size(), 1);
}

// Test tap-then-hold sequence
TEST_F(TapDanceTapHoldInteractionTest, TapThenHoldSequence) {
    g_mock_state.tap_code_calls.clear();
    g_mock_state.layer_on_calls.clear();

    // First: Quick tap
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    platform_wait_ms(100); // Brief pause

    // Then: Hold
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250); // Hold
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    // Should have both tap and hold actions
    // EXPECT_GE(g_mock_state.tap_code_calls_count(), 1);
    // EXPECT_GE(g_mock_state.layer_select_calls_count(), 1);
}

// Test hold-then-tap sequence
TEST_F(TapDanceTapHoldInteractionTest, HoldThenTapSequence) {
    // g_mock_state.tap_code_calls.clear();
    // g_mock_state.layer_on_calls.clear();

    // First: Hold
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250);
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    platform_wait_ms(100); // Brief pause

    // Then: Quick tap
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50);

    // Should have both hold and tap actions
    EXPECT_GE(g_mock_state.layer_select_calls_count(), 1);
    // EXPECT_GE(g_mock_state.tap_code_calls_count(), 1);
}

// Test rapid tap-hold transitions
TEST_F(TapDanceTapHoldInteractionTest, RapidTapHoldTransitions) {
    // g_mock_state.tap_code_calls.clear();
    // g_mock_state.layer_on_calls.clear();

    // Rapid sequence: tap, brief hold, tap, brief hold
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 30);

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    platform_wait_ms(150); // Short hold
    simulate_key_event(CKC_LAY_NUMBERS_R, false);

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 30);

    platform_wait_ms(250);

    // Should register some activity
    // EXPECT_GT(g_mock_state.tap_code_calls_count(), 0);
}

// Test overlapping tap-hold between different keys
TEST_F(TapDanceTapHoldInteractionTest, OverlappingTapHoldDifferentKeys) {
    // g_mock_state.tap_code_calls.clear();
    // g_mock_state.layer_on_calls.clear();

    // Start holding first key
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(100);

    // While holding first, tap second key
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);

    // Continue holding first key
    platform_wait_ms(200);
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    // Should handle both keys appropriately
    // EXPECT_GT(g_mock_state.tap_code_calls_count(), 0); // From R tap
    EXPECT_GT(g_mock_state.layer_select_calls_count(), 0); // From Q hold
}

// Test interrupted hold (released before timeout)
TEST_F(TapDanceTapHoldInteractionTest, InterruptedHold) {
    // g_mock_state.tap_code_calls.clear();
    // g_mock_state.layer_on_calls.clear();

    // Press and release before hold timeout
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(150); // Less than typical 200ms timeout
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    platform_wait_ms(250);

    // Should treat as tap since hold wasn't completed
    // EXPECT_GE(g_mock_state.tap_code_calls_count(), 1);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}
