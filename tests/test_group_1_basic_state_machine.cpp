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

class BasicStateMachineTest : public ::testing::Test {
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

// Simple Tap
// Objective: Verify basic tap sequence with release before hold timeout
TEST_F(BasicStateMachineTest, SimpleTap) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY, BEFORE_TAP_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, BEFORE_TAP_TIMEOUT), td_release(OUTPUT_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Simple Hold
// Objective: Verify basic hold sequence with timeout triggering hold action
TEST_F(BasicStateMachineTest, SimpleHold) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t OUTPUT_KEY = 3002;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(AFTER_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, AFTER_HOLD_TIMEOUT), td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));

}

// Hold Timeout Boundary - Just Before
// Objective: Verify tap behavior when released exactly at hold timeout boundary
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryJustBefore) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t OUTPUT_KEY = 3002;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 0, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY, JUST_BEFORE_HOLD_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, JUST_BEFORE_HOLD_TIMEOUT), td_release(OUTPUT_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Hold Timeout Boundary - Exactly At
// Objective: Verify hold behavior when timeout occurs exactly at boundary
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryExactlyAt) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t OUTPUT_KEY = 3002;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, HOLD_TIMEOUT), td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Hold Timeout Boundary - Just After
// Objective: Verify hold behavior when held past timeout
TEST_F(BasicStateMachineTest, HoldTimeoutBoundaryJustAfter) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t OUTPUT_KEY = 3002;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY, JUST_AFTER_HOLD_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, JUST_AFTER_HOLD_TIMEOUT), td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// No Hold Action Configured - Immediate Execution
// Objective: Verify immediate execution when no hold action available
TEST_F(BasicStateMachineTest, NoHoldActionConfiguredImmediateExecution) {
     const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t OUTPUT_KEY = 3002;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);

    // Only tap actions, no hold actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY, BEFORE_HOLD_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 0), td_release(OUTPUT_KEY, BEFORE_HOLD_TIMEOUT)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Only Hold Action Configured - Timeout Not Reached
// Objective: Verify behavior when only hold action is configured
TEST_F(BasicStateMachineTest, OnlyHoldActionConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);


    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY, BEFORE_HOLD_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Only Hold Action - Timeout Reached
// Objective: Verify hold action executes when only hold configured and timeout reached
TEST_F(BasicStateMachineTest, OnlyHoldActionTimeoutReached) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(AFTER_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, AFTER_HOLD_TIMEOUT), td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// State Machine Reset Verification - Tap -> Reset -> Hold
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, TapResetHold) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t KEY_LAYER_2 = 3002;
    const uint16_t OUTPUT_KEY_1 = 3003;
    const uint16_t OUTPUT_KEY_2 = 3004;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;

    static const platform_keycode_t keymaps[3][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }},
        {{ KEY_LAYER_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 3, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_1),
        createbehaviouraction_hold(1, TARGET_LAYER_1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_tap(2, OUTPUT_KEY_2),
        createbehaviouraction_hold(2, TARGET_LAYER_2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_config->length++;

    // First sequence - tap
    press_key(TAP_DANCE_KEY);
    platform_wait_ms(BEFORE_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    platform_wait_ms(TAP_TIMEOUT);

    // Second sequence - hold
    press_key(TAP_DANCE_KEY);
    platform_wait_ms(AFTER_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_1, BEFORE_HOLD_TIMEOUT + TAP_TIMEOUT), td_release(OUTPUT_KEY_1, 0),
        td_layer(TARGET_LAYER_1, AFTER_HOLD_TIMEOUT), td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// State Machine Reset Verification - Tap -> Reset -> Tap
// Objective: Verify internal state properly resets after each sequence
TEST_F(BasicStateMachineTest, TapResetTap) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint16_t KEY_LAYER_2 = 3002;
    const uint16_t OUTPUT_KEY_1 = 3003;
    const uint16_t OUTPUT_KEY_2 = 3004;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;

    static const platform_keycode_t keymaps[3][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }},
        {{ KEY_LAYER_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 3, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_1),
        createbehaviouraction_hold(1, TARGET_LAYER_1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_tap(2, OUTPUT_KEY_2),
        createbehaviouraction_hold(2, TARGET_LAYER_2, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_config->length++;

    // First sequence - tap
    press_key(TAP_DANCE_KEY);
    platform_wait_ms(BEFORE_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    platform_wait_ms(TAP_TIMEOUT);

    // Second sequence - tap
    press_key(TAP_DANCE_KEY);
    platform_wait_ms(BEFORE_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    platform_wait_ms(TAP_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_1, BEFORE_HOLD_TIMEOUT + TAP_TIMEOUT), td_release(OUTPUT_KEY_1, 0),
        td_press(OUTPUT_KEY_1, BEFORE_HOLD_TIMEOUT + TAP_TIMEOUT), td_release(OUTPUT_KEY_1, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Hold with no actions configured
// Objective: Verify hold functionality when there are no actions
TEST_F(BasicStateMachineTest, HoldWithNoActionsConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t** actions = nullptr;
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 0);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(AFTER_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Tap with no actions configured
// Objective: Verify tap functionality when there are no actions
TEST_F(BasicStateMachineTest, TapWithNoActionsConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t** actions = nullptr;
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 0);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY, BEFORE_TAP_TIMEOUT);

    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}
