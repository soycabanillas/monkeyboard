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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 50); release_key(INTERRUPTING_KEY, 50); release_key(TAP_DANCE_KEY, 50); platform_wait_ms(200);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 50);   // t=50ms (interrupt)
    release_key(INTERRUPTING_KEY, 50); // t=100ms
    release_key(TAP_DANCE_KEY, 50);    // t=150ms (before hold timeout)
    platform_wait_ms(200);            // Wait for tap timeout

    // Expected: Interrupting key processed normally, tap action (interruption ignored)
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 50), release(INTERRUPTING_KEY, 100),
        press(OUTPUT_KEY, 350), release(OUTPUT_KEY, 350)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 50); release_key(INTERRUPTING_KEY, 50); release_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 50);   // t=50ms (interrupt)
    release_key(INTERRUPTING_KEY, 50); // t=100ms
    platform_wait_ms(150);            // t=250ms (hold timeout exceeded)
    release_key(TAP_DANCE_KEY);        // t=250ms

    // Expected: Hold action at timeout (delayed execution)
    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 30); release_key(INTERRUPTING_KEY, 30); release_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 30);   // t=30ms
    press_key(3003, 20);               // t=50ms (another interruption)
    release_key(INTERRUPTING_KEY, 30); // t=80ms
    release_key(TAP_DANCE_KEY, 50);    // t=100ms
    platform_wait_ms(200);            // Wait for tap timeout

    // Expected: All ignored, tap action executed
    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY, 350), release(OUTPUT_KEY, 350)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 50); release_key(INTERRUPTING_KEY, 50); release_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 50);   // t=50ms
    release_key(INTERRUPTING_KEY, 50); // t=100ms (complete cycle)
    release_key(TAP_DANCE_KEY, 50);    // t=150ms (trigger still held)

    // Expected: Hold triggered by complete cycle
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 50), release(INTERRUPTING_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 50); release_key(TAP_DANCE_KEY, 50); release_key(INTERRUPTING_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 50);   // t=50ms
    release_key(TAP_DANCE_KEY, 50);    // t=100ms (trigger released first)
    release_key(INTERRUPTING_KEY, 50); // t=150ms

    // Expected: Tap triggered when trigger released
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 50), release(INTERRUPTING_KEY, 100),
        press(OUTPUT_KEY, 100), release(OUTPUT_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 50); release_key(TAP_DANCE_KEY, 100); release_key(INTERRUPTING_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 50);   // t=50ms
    release_key(TAP_DANCE_KEY, 100);   // t=100ms (release tap, interrupt still held)
    release_key(INTERRUPTING_KEY, 50); // t=150ms

    // Expected: Tap action executed, hold action pending
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 50), release(INTERRUPTING_KEY, 100),
        press(OUTPUT_KEY, 100), release(OUTPUT_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY_1, 50); release_key(INTERRUPTING_KEY_1, 50); release_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY_1, 50);   // t=50ms
    press_key(INTERRUPTING_KEY_2, 20);   // t=70ms (second interruption)
    release_key(INTERRUPTING_KEY_1, 50); // t=100ms (complete cycle)
    release_key(TAP_DANCE_KEY, 50);    // t=150ms
    release_key(INTERRUPTING_KEY_2, 50); // t=200ms

    // Expected: Hold triggered by first complete cycle
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 50), release(INTERRUPTING_KEY_1, 100),
        press(OUTPUT_KEY, 100), release(OUTPUT_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 190); release_key(INTERRUPTING_KEY, 15); release_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 190);  // t=190ms (close to timeout)
    release_key(INTERRUPTING_KEY, 15); // t=205ms (complete cycle after timeout)
    release_key(TAP_DANCE_KEY, 50);    // t=250ms

    // Expected: Hold triggered by timeout (happens first)
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 190), release(INTERRUPTING_KEY, 205),
        press(OUTPUT_KEY, 250), release(OUTPUT_KEY, 250)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
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
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Input: press_key(TAP_DANCE_KEY); press_key(INTERRUPTING_KEY, 50); release_key(INTERRUPTING_KEY, 50); release_key(TAP_DANCE_KEY, 50);
    press_key(TAP_DANCE_KEY);          // t=0ms
    press_key(INTERRUPTING_KEY, 50);   // t=50ms (immediate hold trigger)
    release_key(INTERRUPTING_KEY, 50); // t=100ms
    release_key(TAP_DANCE_KEY, 50);    // t=150ms

    // Expected: Immediate hold on interrupt press
    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY, 50), release(INTERRUPTING_KEY, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

