#include <chrono>
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

class MultiTapTest : public ::testing::Test {
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
        pipeline_executor_add_physical_pipeline(0, &pipeline_tap_dance_callback_process_data_executor, &pipeline_tap_dance_callback_reset_executor, tap_dance_config);
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

// Test 3.1: Basic Two-Tap Sequence
// Objective: Verify basic two-tap sequence with proper tap count progression
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(MultiTapTest, BasicTwoTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 250);
    keyboard.release_key_at(TAP_DANCE_KEY, 350);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 550),
        td_release(3002, 550)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.2: Three-Tap Sequence
// Objective: Verify three-tap sequence progression
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)], Hold actions: [1: CHANGELAYER(1)]
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(MultiTapTest, ThreeTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 250);
    keyboard.release_key_at(TAP_DANCE_KEY, 350);
    keyboard.press_key_at(TAP_DANCE_KEY, 500);
    keyboard.release_key_at(TAP_DANCE_KEY, 600);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3003, 800),
        td_release(3003, 800)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.3: Sequence Reset - Tap Timeout Expiry
// Objective: Verify sequence resets when tap timeout expires between taps
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, SequenceResetTapTimeout) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 350);
    keyboard.release_key_at(TAP_DANCE_KEY, 450);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 300),
        td_release(3001, 300),
        td_press(3001, 650),
        td_release(3001, 650)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.4: Multi-Tap with Hold Action (First Tap)
// Objective: Verify hold action works correctly during multi-tap sequence (1st tap count)
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapHoldActionFirstTap) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.5: Multi-Tap with Hold Action (Second Tap)
// Objective: Verify hold action at second tap count when configured
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapHoldActionSecondTap) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 150);
    keyboard.release_key_at(TAP_DANCE_KEY, 400);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 350),
        td_layer(0, 400)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.6: Hold Action Not Available for Tap Count
// Objective: Verify behavior when hold action not configured for current tap count
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, HoldActionNotAvailableForTapCount) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 400),
        td_release(3002, 400)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.7: Rapid Tap Sequence - All Within Timeout
// Objective: Verify system handles extremely rapid tap sequences
// Configuration: Same as Test 3.2
TEST_F(MultiTapTest, RapidTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    for (int i = 0; i < 5; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 20 + 10);
        keyboard.release_key_at(TAP_DANCE_KEY, i * 20 + 20);
    }

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 300),
        td_release(3002, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.8: Mixed Tap and Hold in Sequence
// Objective: Verify mix of tap and hold behaviors within single sequence
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MixedTapAndHoldInSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.release_key_at(TAP_DANCE_KEY, 350);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 300),
        td_layer(0, 350)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.9: Tap Count Boundary - Exact Timeout Edge
// Objective: Verify timing precision at tap timeout boundaries
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, TapCountBoundaryExactTimeoutEdge) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);
    keyboard.press_key_at(TAP_DANCE_KEY, 400);
    keyboard.release_key_at(TAP_DANCE_KEY, 450);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 200),
        td_release(3001, 200),
        td_press(3001, 650),
        td_release(3001, 650)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.10: Maximum Practical Tap Count
// Objective: Verify system handles high tap counts correctly
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MaximumPracticalTapCount) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_tap(4, 3004),
        createbehaviouraction_tap(5, 3005)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 5);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    for (int i = 0; i < 5; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 40 + 20);
        keyboard.release_key_at(TAP_DANCE_KEY, i * 40 + 40);
    }

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3005, 400),
        td_release(3005, 400)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.11: Sequence Continuation vs New Sequence
// Objective: Verify clear distinction between sequence continuation and new sequence
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, SequenceContinuationVsNewSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_tap(3, 3003),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(TAP_DANCE_KEY, 299);
    keyboard.release_key_at(TAP_DANCE_KEY, 349);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3001, 300),
        td_release(3001, 300),
        td_press(3001, 549),
        td_release(3001, 549)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.12: Multi-Tap with Strategy Interruption
// Objective: Verify multi-tap behavior combined with hold strategy interruption
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapWithStrategyInterruption) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint8_t TARGET_LAYER = 1;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.press_key_at(3003, 150);
    keyboard.release_key_at(3003, 200);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER, 150),
        td_press(3003, 150),
        td_release(3003, 200),
        td_layer(0, 250)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.13: Tap Count Reset Verification
// Objective: Verify tap count properly resets between independent sequences
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, TapCountResetVerification) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.release_key_at(TAP_DANCE_KEY, 200);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 400),
        td_release(3002, 400)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.14: Very Fast Multi-Tap Sequence
// Objective: Verify system handles extremely rapid tap sequences
// Configuration: Same as Test 3.2
TEST_F(MultiTapTest, VeryFastMultiTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    for (int i = 0; i < 5; i++) {
        keyboard.press_key_at(TAP_DANCE_KEY, i * 20 + 10);
        keyboard.release_key_at(TAP_DANCE_KEY, i * 20 + 20);
    }

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 300),
        td_release(3002, 300)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 3.15: Multi-Tap Overflow Preview
// Objective: Verify behavior approaching overflow conditions (sets up for Group 4)
// Configuration: Same as Test 3.1
TEST_F(MultiTapTest, MultiTapOverflowPreview) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][1][1] = {{{ TAP_DANCE_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 30);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);
    keyboard.press_key_at(TAP_DANCE_KEY, 80);
    keyboard.release_key_at(TAP_DANCE_KEY, 110);
    keyboard.press_key_at(TAP_DANCE_KEY, 160);
    keyboard.release_key_at(TAP_DANCE_KEY, 190);
    keyboard.press_key_at(TAP_DANCE_KEY, 240);
    keyboard.release_key_at(TAP_DANCE_KEY, 270);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3002, 470),
        td_release(3002, 470)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
