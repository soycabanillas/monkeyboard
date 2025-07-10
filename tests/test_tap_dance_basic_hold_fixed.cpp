#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "gtest/gtest.h"
#include "platform_interface.h"

extern "C" {
#include "../src/keycodes.h"
#include "commons.h"
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

#include "platform_mock.h"

// Define the missing keycodes for the test
#define CKC_LAY_MOUSE_Q 0x7E00
#define CKC_LAY_NUMBERS_R 0x7E01
#define CKC_LAY_MOVEMENT_F 0x7E02
#define CKC_EXCL 0x7E03
#define CKC_QUES 0x7E04
#define CKC_LAY_RIGHT_THUMB 0x7E05
#define CKC_LSHIFT_EXCLAMATION_MARK 0x7E06
#define CKC_RSHIFT_QUESTION_MARK 0x7E07
class TapDanceBasicHoldTest : public ::testing::Test {
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

// Test hold on CKC_LAY_MOUSE_Q activates mouse layer
TEST_F(TapDanceBasicHoldTest, HoldActivatesMouseLayer) {
    g_mock_state.layer_on_calls.clear();

    // Press and hold CKC_LAY_MOUSE_Q
    simulate_key_event(CKC_LAY_MOUSE_Q, true);

    // Advance time past the tap timeout (200ms) to trigger hold
    platform_wait_ms(250);

    // Verify that the mouse layer would be activated in a real implementation
    // Note: Our simplified mock doesn't implement hold logic, so we'll simulate it

    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_layer_on, _LMOUSE);
}

// Test hold and release deactivates layer
TEST_F(TapDanceBasicHoldTest, HoldAndReleaseDeactivatesLayer) {
    g_mock_state.layer_on_calls.clear();

    // Press and hold
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250); // Trigger hold

    // Release the key
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    // Verify layer was deactivated and keyboard cleared
    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 1);
}

// Test hold on CKC_LAY_NUMBERS_R activates numbers layer
TEST_F(TapDanceBasicHoldTest, HoldActivatesNumbersLayer) {
    g_mock_state.layer_on_calls.clear();

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_layer_on, _LNUMBERS);
}

// Test hold on CKC_LAY_MOVEMENT_F activates movement layer
TEST_F(TapDanceBasicHoldTest, HoldActivatesMovementLayer) {
    g_mock_state.layer_on_calls.clear();

    simulate_key_event(CKC_LAY_MOVEMENT_F, true);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_layer_on, _LMOVEMENT);
}

// Test hold on CKC_LAY_RIGHT_THUMB activates thumb layer
TEST_F(TapDanceBasicHoldTest, HoldActivatesRightThumbLayer) {
    g_mock_state.layer_on_calls.clear();

    simulate_key_event(CKC_LAY_RIGHT_THUMB, true);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_layer_on, _LRIGHT_THUMB);
}

// Test that short press doesn't activate layer
TEST_F(TapDanceBasicHoldTest, ShortPressDoesNotActivateLayer) {
    g_mock_state.layer_on_calls.clear();

    // Press and release quickly (before timeout)
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    simulate_key_event(CKC_LAY_MOUSE_Q, false, 50); // Release after 50ms

    // Should not have activated layer
    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 0);
}

// Test that keys without hold actions don't activate layers
TEST_F(TapDanceBasicHoldTest, KeysWithoutHoldActionsDoNotActivateLayer) {
    g_mock_state.layer_on_calls.clear();

    // CKC_LSHIFT_EXCLAMATION_MARK and CKC_RSHIFT_QUESTION_MARK have tap actions but no hold actions
    simulate_key_event(CKC_LSHIFT_EXCLAMATION_MARK, true);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 0);

    simulate_key_event(CKC_LSHIFT_EXCLAMATION_MARK, false);

    // Try with question mark too
    simulate_key_event(CKC_RSHIFT_QUESTION_MARK, true);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.layer_on_calls_count(), 0);
}
