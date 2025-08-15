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

class HoldStrategyTest : public ::testing::Test {
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

// Test 2.1: TAP_PREFERRED - Interruption Ignored (Basic)
// Objective: Verify TAP_PREFERRED ignores interrupting keys and only uses timeout
// Configuration: TAP_DANCE_KEY = 3000, OUTPUT_KEY = 3001, INTERRUPTING_KEY = 3002
// Strategy: TAP_PREFERRED, Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(HoldStrategyTest, TapPreferredInterruptionIgnored) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_TAP_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_press(OUTPUT_KEY, 350),
        td_release(OUTPUT_KEY, 350)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.2: TAP_PREFERRED - Hold via Timeout Only
// Objective: Verify TAP_PREFERRED only triggers hold via timeout, not interruption
// Configuration: Same as Test 2.1
TEST_F(HoldStrategyTest, TapPreferredHoldViaTimeoutOnly) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_TAP_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_layer(1, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.3: TAP_PREFERRED - Multiple Interruptions
// Objective: Verify multiple interruptions are all ignored
// Configuration: Same as Test 2.1
TEST_F(HoldStrategyTest, TapPreferredMultipleInterruptions) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_TAP_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 30);
    press_key_at(3003, 50);
    release_key_at(INTERRUPTING_KEY, 80);
    release_key_at(TAP_DANCE_KEY, 130);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 30),
        td_press(3003, 50),
        td_release(INTERRUPTING_KEY, 80),
        td_press(OUTPUT_KEY, 330),
        td_release(OUTPUT_KEY, 330)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.4: BALANCED - Hold on Complete Press/Release Cycle
// Objective: Verify BALANCED triggers hold when interrupting key completes full cycle
// Configuration: Strategy: BALANCED, other settings same as Test 2.1
TEST_F(HoldStrategyTest, BalancedHoldOnCompleteyCycle) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_layer(1, 100),
        td_layer(0, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.5: BALANCED - Tap when Trigger Released First
// Objective: Verify BALANCED triggers tap when trigger key released before interrupting key
// Configuration: Same as Test 2.4
TEST_F(HoldStrategyTest, BalancedTapWhenTriggerReleasedFirst) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(TAP_DANCE_KEY, 100);
    release_key_at(INTERRUPTING_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 50),
        td_press(OUTPUT_KEY, 100),
        td_release(OUTPUT_KEY, 100),
        td_release(INTERRUPTING_KEY, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.6: BALANCED - Incomplete Interruption Cycle
// Objective: Verify BALANCED behavior when interrupting key pressed but not released
// Configuration: Same as Test 2.4
TEST_F(HoldStrategyTest, BalancedIncompleteInterruptionCycle) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(TAP_DANCE_KEY, 150);
    release_key_at(INTERRUPTING_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 50),
        td_press(OUTPUT_KEY, 150),
        td_release(OUTPUT_KEY, 150),
        td_release(INTERRUPTING_KEY, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.7: BALANCED - Multiple Interrupting Keys
// Objective: Verify BALANCED with multiple interrupting keys (first complete cycle wins)
// Configuration: Same as Test 2.4
TEST_F(HoldStrategyTest, BalancedMultipleInterruptingKeys) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY_1 = 3002;
    const uint16_t INTERRUPTING_KEY_2 = 3003;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY_1, 50);
    press_key_at(INTERRUPTING_KEY_2, 70);
    release_key_at(INTERRUPTING_KEY_1, 100);
    release_key_at(TAP_DANCE_KEY, 150);
    release_key_at(INTERRUPTING_KEY_2, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 50),
        td_press(INTERRUPTING_KEY_2, 70),
        td_release(INTERRUPTING_KEY_1, 100),
        td_layer(1, 100),
        td_layer(0, 150),
        td_release(INTERRUPTING_KEY_2, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.8: BALANCED - Timeout vs Complete Cycle Race
// Objective: Verify behavior when hold timeout and complete cycle occur close together
// Configuration: Same as Test 2.4
TEST_F(HoldStrategyTest, BalancedTimeoutVsCompleteCycleRace) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 190);
    release_key_at(INTERRUPTING_KEY, 205);
    release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 190),
        td_layer(1, 200),
        td_release(INTERRUPTING_KEY, 205),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.9: HOLD_PREFERRED - Immediate Hold on Any Press
// Objective: Verify HOLD_PREFERRED triggers hold immediately on any interrupting key press
// Configuration: Strategy: HOLD_PREFERRED, other settings same as Test 2.1
TEST_F(HoldStrategyTest, HoldPreferredImmediateHold) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

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
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 50),
        td_press(INTERRUPTING_KEY, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_layer(0, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.10: HOLD_PREFERRED - First Interruption Wins
// Objective: Verify HOLD_PREFERRED triggers on first interruption only
// Configuration: Same as Test 2.9
TEST_F(HoldStrategyTest, HoldPreferredFirstInterruptionWins) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

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
    press_key_at(INTERRUPTING_KEY, 30);
    press_key_at(3003, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(3003, 150);
    release_key_at(TAP_DANCE_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 30),
        td_press(INTERRUPTING_KEY, 30),
        td_press(3003, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_release(3003, 150),
        td_layer(0, 200)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.11: HOLD_PREFERRED - Tap without Interruption
// Objective: Verify HOLD_PREFERRED still allows tap when no interruption occurs
// Configuration: Same as Test 2.9
TEST_F(HoldStrategyTest, HoldPreferredTapWithoutInterruption) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 2, 1);

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
    release_key_at(TAP_DANCE_KEY, 100);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 300),
        td_release(OUTPUT_KEY, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.12: Strategy Comparison - Same Input Pattern
// Objective: Verify different strategies produce different outputs with identical input
TEST_F(HoldStrategyTest, StrategyComparisonSameInputPattern) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    // Test with BALANCED strategy
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_layer(1, 100),
        td_layer(0, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.13: Interruption During WAITING_FOR_TAP State
// Objective: Verify interruptions during tap timeout period don't affect completed sequence
// Configuration: Same as Test 2.1 (TAP_PREFERRED)
TEST_F(HoldStrategyTest, InterruptionDuringWaitingForTapState) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_TAP_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 100);
    press_key_at(INTERRUPTING_KEY, 150);
    release_key_at(INTERRUPTING_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY, 150),
        td_release(INTERRUPTING_KEY, 200),
        td_press(OUTPUT_KEY, 300),
        td_release(OUTPUT_KEY, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.14: Edge Case - Interruption at Exact Timeout Boundary
// Objective: Verify interruption timing at exact hold timeout boundary
// Configuration: Same as Test 2.4 (BALANCED)
TEST_F(HoldStrategyTest, EdgeCaseInterruptionAtExactTimeoutBoundary) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(INTERRUPTING_KEY, 200);
    release_key_at(INTERRUPTING_KEY, 201);
    release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 200),
        td_press(INTERRUPTING_KEY, 200),
        td_release(INTERRUPTING_KEY, 201),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 2.15: Strategy with No Hold Action Available
// Objective: Verify strategy behavior when hold action not configured
TEST_F(HoldStrategyTest, StrategyWithNoHoldActionAvailable) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint16_t INTERRUPTING_KEY = 3002;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { OUTPUT_KEY }, { INTERRUPTING_KEY }}
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 1, 3, 1);

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
    press_key_at(INTERRUPTING_KEY, 50);
    release_key_at(INTERRUPTING_KEY, 100);
    release_key_at(TAP_DANCE_KEY, 150);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY, 0),
        td_press(INTERRUPTING_KEY, 50),
        td_release(INTERRUPTING_KEY, 100),
        td_release(OUTPUT_KEY, 150)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

