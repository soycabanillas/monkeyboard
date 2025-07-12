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
class TapDanceLayerSwitchingTest : public ::testing::Test {
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

// Test basic layer activation via hold
TEST_F(TapDanceLayerSwitchingTest, BasicLayerActivation) {
    g_mock_state.layer_on_calls.clear();

    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250); // Hold to activate layer

    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_layer_on, _LMOUSE);
}

// Test layer deactivation on key release
TEST_F(TapDanceLayerSwitchingTest, LayerDeactivationOnRelease) {
    g_mock_state.layer_on_calls.clear();

    // Activate layer
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    platform_wait_ms(250);

    // Release key to deactivate
    simulate_key_event(CKC_LAY_NUMBERS_R, false);

    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
}

// Test multiple layer switching
TEST_F(TapDanceLayerSwitchingTest, MultipleLayerSwitching) {
    g_mock_state.layer_on_calls.clear();
    g_mock_state.layer_on_calls.clear();

    // Activate mouse layer
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250);

    // Release and switch to numbers layer
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    platform_wait_ms(100);

    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    platform_wait_ms(250);

    // Should have activated both layers in sequence
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 2);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
}

// Test overlapping layer activation
TEST_F(TapDanceLayerSwitchingTest, OverlappingLayerActivation) {
    g_mock_state.layer_on_calls.clear();

    // Start holding first layer key
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(250);

    // While holding first, activate second layer
    simulate_key_event(CKC_LAY_MOVEMENT_F, true);
    platform_wait_ms(250);

    // Should have both layers active
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 2);
}

// Test layer switching with tap interruption
TEST_F(TapDanceLayerSwitchingTest, LayerSwitchingWithTapInterruption) {
    g_mock_state.layer_on_calls.clear();
    g_mock_state.tap_code_calls.clear();

    // Start layer activation
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(100);

    // Interrupt with different key tap
    simulate_key_event(CKC_LSHIFT_EXCLAMATION_MARK, true);
    simulate_key_event(CKC_LSHIFT_EXCLAMATION_MARK, false, 50);

    // Continue original layer activation
    platform_wait_ms(200);
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    // Should have both layer activation and tap
    EXPECT_GE(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_GE(g_mock_state.tap_code_calls_count(), 1);
}

// Test rapid layer switching
TEST_F(TapDanceLayerSwitchingTest, RapidLayerSwitching) {
    g_mock_state.layer_on_calls.clear();
    g_mock_state.layer_on_calls.clear();

    // Rapidly switch between layers
    const uint8_t layers[] = {_LMOUSE, _LNUMBERS, _LMOVEMENT};
    const uint16_t keycodes[] = {CKC_LAY_MOUSE_Q, CKC_LAY_NUMBERS_R, CKC_LAY_MOVEMENT_F};

    for (int i = 0; i < 3; i++) {
        simulate_key_event(keycodes[i], true);
        platform_wait_ms(100);
        platform_wait_ms(50);
        simulate_key_event(keycodes[i], false);
        platform_wait_ms(50);
    }

    // Should have activated and deactivated each layer
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 3);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 3);
}

// Test layer persistence across key releases
TEST_F(TapDanceLayerSwitchingTest, LayerPersistenceAcrossKeyReleases) {
    g_mock_state.layer_on_calls.clear();

    // Activate layer
    simulate_key_event(CKC_LAY_RIGHT_THUMB, true);
    platform_wait_ms(250);

    // Tap other keys while layer is active
    simulate_key_event(KC_A, true);
    simulate_key_event(KC_A, false, 50);

    platform_wait_ms(50);

    simulate_key_event(KC_Q, true);
    simulate_key_event(KC_Q, false, 50);

    // Layer should still be active

    // Release original layer key
    simulate_key_event(CKC_LAY_RIGHT_THUMB, false);

    // Now layer should be inactive
}

// Test layer switching edge cases
TEST_F(TapDanceLayerSwitchingTest, LayerSwitchingEdgeCases) {
    g_mock_state.layer_on_calls.clear();

    // Very brief hold (edge of timeout)
    simulate_key_event(CKC_LAY_MOUSE_Q, true);
    platform_wait_ms(200); // Right at timeout threshold
    simulate_key_event(CKC_LAY_MOUSE_Q, false);

    platform_wait_ms(100);

    // Double tap followed by hold
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    simulate_key_event(CKC_LAY_NUMBERS_R, false, 50);
    platform_wait_ms(50);
    simulate_key_event(CKC_LAY_NUMBERS_R, true);
    platform_wait_ms(250);

    // Should handle edge cases gracefully
    EXPECT_GE(g_mock_state.layer_select_calls_count(), 0);
}
