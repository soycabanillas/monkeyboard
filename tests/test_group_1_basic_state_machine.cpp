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
            malloc(sizeof(*tap_dance_config)));
        tap_dance_config->length = 0;
        tap_dance_config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 150),
        td_release(OUTPUT_KEY, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 0, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 199),
        td_release(OUTPUT_KEY, 199)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 201);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 201)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    // Only tap actions, no hold actions
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 0),
        td_release(OUTPUT_KEY, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Only Hold Action Configured - Timeout Not Reached
// Objective: Verify behavior when only hold action is configured
TEST_F(BasicStateMachineTest, OnlyHoldActionTimeoutNotReached) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_LAYER_1 = 3001;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[2][1][1] = {
        {{ TAP_DANCE_KEY }},
        {{ KEY_LAYER_1 }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 3, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_1),
        createbehaviouraction_hold(1, TARGET_LAYER_1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_tap(2, OUTPUT_KEY_2),
        createbehaviouraction_hold(2, TARGET_LAYER_2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First sequence - tap
    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 150);
    
    // Wait for tap timeout then second sequence - hold
    press_key_at(TAP_DANCE_KEY, 400);
    release_key_at(TAP_DANCE_KEY, 650);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_1, 350),
        td_release(OUTPUT_KEY_1, 350),
        td_layer(TARGET_LAYER_1, 600),
        td_layer(0, 650)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Tap -> Reset -> Tap
// Objective: Verify state machine properly resets between independent sequences
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 3, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_1),
        createbehaviouraction_hold(1, TARGET_LAYER_1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_tap(2, OUTPUT_KEY_2),
        createbehaviouraction_hold(2, TARGET_LAYER_2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First sequence - tap
    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 150);

    // Wait for tap timeout then second sequence - tap
    press_key_at(TAP_DANCE_KEY, 400);
    release_key_at(TAP_DANCE_KEY, 550);
    wait_ms(200); // Ensure tap timeout is reached

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_1, 350),
        td_release(OUTPUT_KEY_1, 350),
        td_press(OUTPUT_KEY_1, 750),
        td_release(OUTPUT_KEY_1, 750)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Hold -> Reset -> Tap
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, HoldResetTap) {
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 3, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_1),
        createbehaviouraction_hold(1, TARGET_LAYER_1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_tap(2, OUTPUT_KEY_2),
        createbehaviouraction_hold(2, TARGET_LAYER_2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First sequence - hold
    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 250);

    // Second sequence - tap
    press_key_at(TAP_DANCE_KEY, 300);
    release_key_at(TAP_DANCE_KEY, 450);
    wait_ms(200); // Ensure tap timeout is reached

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_1, 200),
        td_layer(0, 250),
        td_press(OUTPUT_KEY_1, 650),
        td_release(OUTPUT_KEY_1, 650)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// State Machine Reset Verification - Hold -> Reset -> Hold
// Objective: Verify state machine properly resets between independent sequences
TEST_F(BasicStateMachineTest, HoldResetHold) {
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
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 3, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_1),
        createbehaviouraction_hold(1, TARGET_LAYER_1, TAP_DANCE_HOLD_PREFERRED),
        createbehaviouraction_tap(2, OUTPUT_KEY_2),
        createbehaviouraction_hold(2, TARGET_LAYER_2, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First sequence - hold
    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 250);

    // Second sequence - hold
    press_key_at(TAP_DANCE_KEY, 300);
    release_key_at(TAP_DANCE_KEY, 550);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_1, 200),
        td_layer(0, 250),
        td_layer(TARGET_LAYER_1, 500),
        td_layer(0, 550)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Hold with no actions configured
// Objective: Verify hold functionality when there are no actions
TEST_F(BasicStateMachineTest, HoldWithNoActionsConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t** actions = nullptr;
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 0);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Tap with no actions configured
// Objective: Verify tap functionality when there are no actions
TEST_F(BasicStateMachineTest, TapWithNoActionsConfigured) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t** actions = nullptr;
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 0);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
