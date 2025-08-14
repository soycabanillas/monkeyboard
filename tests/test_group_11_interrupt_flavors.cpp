#include <sys/types.h>
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

class InterruptFlavorsTest : public ::testing::Test {
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


const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
const uint16_t KEY_B = 3010;          // KC_B
const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

static void set_scenario(pipeline_tap_dance_global_config_t* tap_dance_config, tap_dance_hold_strategy_t hold_strategy) {


    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2D_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, hold_strategy)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[0] = tap_dance_behavior;
    tap_dance_config->length++;
}


// Test Case 1: AABB sequence - Press A, release A, press B, release B
// Sequence: LSFT_T(KC_A) down, LSFT_T(KC_A) up, KC_B down, KC_B up
// All actions happen before hold timeout
// Expected: All flavors should produce tap (KC_A) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 199);
    press_key_at(KEY_B, 210);
    release_key_at(KEY_B, 220);

    // Should produce tap (KC_A) then KC_B
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),
        td_release(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 210),
        td_release(KEY_B, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 199);
    press_key_at(KEY_B, 210);
    release_key_at(KEY_B, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),
        td_release(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 210),
        td_release(KEY_B, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 199);
    press_key_at(KEY_B, 210);
    release_key_at(KEY_B, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),
        td_release(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 210),
        td_release(KEY_B, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}


// Test Case 2: AABB sequence - Hold A past timeout, then press B
// Sequence: LSFT_T(KC_A) down, wait past hold timeout, LSFT_T(KC_A) up, KC_B down, KC_B up
// Expected: All flavors should produce hold (shift layer) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 201);
    press_key_at(KEY_B, 205);
    release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_layer(0, 201),
        td_press(KEY_B, 205),
        td_release(KEY_B, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 201);
    press_key_at(KEY_B, 205);
    release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_layer(0, 201),
        td_press(KEY_B, 205),
        td_release(KEY_B, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    release_key_at(TAP_DANCE_KEY, 201);
    press_key_at(KEY_B, 205);
    release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_layer(0, 201),
        td_press(KEY_B, 205),
        td_release(KEY_B, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test Case 3: ABBA sequence - Press A, press B, release B, release A (all before hold timeout)
// Sequence: LSFT_T(KC_A) down, KC_B down, KC_B up, LSFT_T(KC_A) up
// All actions happen before hold timeout
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(KEY_B, 120);
    release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 199),
        td_press(KEY_B, 199),
        td_release(KEY_B, 199),
        td_release(OUTPUT_KEY_A, 199)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(KEY_B, 120);
    release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 120),
        td_press(3012, 120),
        td_release(3012, 120),
        td_layer(0, 199)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(KEY_B, 120);
    release_key_at(TAP_DANCE_KEY, 199);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_release(3012, 120),
        td_layer(0, 199)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test Case 4: ABBA sequence - Press A, press B, release B, wait for timeout, release A
// Sequence: LSFT_T(KC_A) down, KC_B down, KC_B up, wait for hold timeout, LSFT_T(KC_A) up
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    // Sequence: A down, B down, B up, wait for timeout, A up
    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(KEY_B, 120);
    release_key_at(TAP_DANCE_KEY, 210);

    // TAP_PREFERRED: Should produce tap (KC_A) when interrupted, even with timeout after
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 200),
        td_release(3012, 200),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(KEY_B, 120);
    release_key_at(TAP_DANCE_KEY, 210);

    // BALANCED: Should produce hold (shift layer) when timeout is reached
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 120),
        td_press(3012, 120),
        td_release(3012, 120),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(KEY_B, 120);
    release_key_at(TAP_DANCE_KEY, 210);

    // HOLD_PREFERRED: Should produce hold (shift layer) when interrupted
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_release(3012, 120),
        td_layer(0, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test Case 5: ABBA sequence - Press A, reach hold timeout, press B, release B, release A
// Sequence: LSFT_T(KC_A) down, wait past hold timeout, KC_B down, KC_B up, LSFT_T(KC_A) up
// Expected: All flavors should produce hold (shift layer) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 205);
    release_key_at(KEY_B, 210);
    release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 205),
        td_release(3012, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 205);
    release_key_at(KEY_B, 210);
    release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 205),
        td_release(3012, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 205);
    release_key_at(KEY_B, 210);
    release_key_at(TAP_DANCE_KEY, 220);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 205),
        td_release(3012, 210),
        td_layer(0, 220)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test Case 6: ABAB sequence - Press A, press B, release A, release B (before hold timeout)
// Sequence: LSFT_T(KC_A) down, KC_B down, LSFT_T(KC_A) up, KC_B up
// All actions happen before hold timeout (based on the first table in the image)
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(TAP_DANCE_KEY, 130);
    release_key_at(KEY_B, 140);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 130),
        td_press(KEY_B, 130),
        td_release(OUTPUT_KEY_A, 130),
        td_release(KEY_B, 140)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(TAP_DANCE_KEY, 130);
    release_key_at(KEY_B, 140);

    // PERMISSIVE_HOLD/BALANCED: Should produce "ab" - same as TAP_PREFERRED
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 130),
        td_press(KEY_B, 130),
        td_release(OUTPUT_KEY_A, 130),
        td_release(KEY_B, 140)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(TAP_DANCE_KEY, 130);
    release_key_at(KEY_B, 140);

    // HOLD_ON_OTHER_KEY_PRESS/HOLD_PREFERRED: Should produce "B" - hold action with B on shift layer
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),  // Shift layer activated when B is pressed (at 110ms)
        td_press(3012, 110),                  // B pressed on shift layer
        td_layer(0, 130),                    // Shift layer deactivated when A released (at 130ms)
        td_release(3012, 140)                // B released (at 140ms)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test Case 7: ABAB sequence - Press A, press B, hold timeout reached, release A, release B
// Sequence: LSFT_T(KC_A) down, KC_B down, LSFT_T(KC_A) held, LSFT_T(KC_A) up, KC_B up
// Hold timeout is reached before A is released
// Expected: All flavors should produce "B" - hold action with B on shift layer

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_TapPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(TAP_DANCE_KEY, 205);
    release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 200),
        td_layer(0, 205),
        td_release(3012, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_Balanced) {
    set_scenario(tap_dance_config, TAP_DANCE_BALANCED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(TAP_DANCE_KEY, 205);
    release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 200),
        td_press(3012, 200),
        td_layer(0, 205),
        td_release(3012, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_HoldPreferred) {
    set_scenario(tap_dance_config, TAP_DANCE_HOLD_PREFERRED);

    press_key_at(TAP_DANCE_KEY, 0);
    press_key_at(KEY_B, 110);
    release_key_at(TAP_DANCE_KEY, 205);
    release_key_at(KEY_B, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),
        td_press(3012, 110),
        td_layer(0, 205),
        td_release(3012, 210)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
