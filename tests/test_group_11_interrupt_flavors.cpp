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

// Test Case 1: AABB sequence - Press A, release A, press B, release B
// Sequence: LSFT_T(KC_A) down, LSFT_T(KC_A) up, KC_B down, KC_B up
// All actions happen before hold timeout
// Expected: All flavors should produce tap (KC_A) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, A up, B down, B up (all before hold timeout)
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down
    platform_wait_ms(BEFORE_HOLD_TIMEOUT); // Wait but release before hold timeout
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up
    press_key(KEY_B);                      // KC_B down
    release_key(KEY_B);                    // KC_B up

    // Should produce tap (KC_A) then KC_B
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, BEFORE_HOLD_TIMEOUT),   // A pressed after release
        td_release(OUTPUT_KEY_A, 0),                   // A released
        td_press(KEY_B, 0),                            // B pressed
        td_release(KEY_B, 0)                           // B released
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(BEFORE_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, BEFORE_HOLD_TIMEOUT),
        td_release(OUTPUT_KEY_A, 0),
        td_press(KEY_B, 0),
        td_release(KEY_B, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_NoHold_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(BEFORE_HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, BEFORE_HOLD_TIMEOUT),
        td_release(OUTPUT_KEY_A, 0),
        td_press(KEY_B, 0),
        td_release(KEY_B, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}


// Test Case 2: AABB sequence - Hold A past timeout, then press B
// Sequence: LSFT_T(KC_A) down, wait past hold timeout, LSFT_T(KC_A) up, KC_B down, KC_B up
// Expected: All flavors should produce hold (shift layer) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, wait past hold timeout, A up, B down, B up
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down
    platform_wait_ms(HOLD_TIMEOUT);       // Wait exactly to hold timeout (200ms)
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up
    press_key(KEY_B);                      // KC_B down
    release_key(KEY_B);                    // KC_B up

    // Should produce hold (shift layer) then KC_B
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),    // Shift layer activated at timeout
        td_layer(0, 0),                                // Shift layer deactivated when A released
        td_press(KEY_B, 0),                            // B pressed
        td_release(KEY_B, 0)                           // B released
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),
        td_layer(0, 0),
        td_press(KEY_B, 0),
        td_release(KEY_B, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_AABB_HoldTimeout_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),
        td_layer(0, 0),
        td_press(KEY_B, 0),
        td_release(KEY_B, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Test Case 3: ABBA sequence - Press A, press B, release B, release A (all before hold timeout)
// Sequence: LSFT_T(KC_A) down, KC_B down, KC_B up, LSFT_T(KC_A) up
// All actions happen before hold timeout
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, B down, B up, A up (all before hold timeout)
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down
    press_key(KEY_B);                      // KC_B down
    release_key(KEY_B);                    // KC_B up
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up

    // TAP_PREFERRED: Should produce tap (KC_A) when interrupted
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),     // A pressed immediately when B interrupts
        td_press(KEY_B, 0),            // B pressed
        td_release(KEY_B, 0),          // B released
        td_release(OUTPUT_KEY_A, 0)    // A released
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);
    release_key(TAP_DANCE_KEY);

    // BALANCED: Should produce hold (shift layer) when interrupted
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 0),  // Shift layer activated immediately when B interrupts
        td_press(3012, 0),               // B pressed (on shift layer)
        td_release(3012, 0),             // B released
        td_layer(0, 0)                    // Shift layer deactivated when A released
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_BeforeTimeout_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);
    release_key(TAP_DANCE_KEY);

    // HOLD_PREFERRED: Should produce hold (shift layer) when interrupted
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 0),  // Shift layer activated immediately when B interrupts
        td_press(3012, 0),               // B pressed (on shift layer)
        td_release(3012, 0),             // B released
        td_layer(0, 0)                    // Shift layer deactivated when A released
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Test Case 4: ABBA sequence - Press A, press B, release B, wait for timeout, release A
// Sequence: LSFT_T(KC_A) down, KC_B down, KC_B up, wait for hold timeout, LSFT_T(KC_A) up
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, B down, B up, wait for timeout, A up
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down
    press_key(KEY_B);                      // KC_B down
    release_key(KEY_B);                    // KC_B up
    platform_wait_ms(HOLD_TIMEOUT);       // Wait exactly to hold timeout (200ms)
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up

    // TAP_PREFERRED: Should produce tap (KC_A) when interrupted, even with timeout after
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 0),     // A pressed immediately when B interrupts
        td_press(3012, 0),            // B pressed
        td_release(3012, 0),          // B released
        td_release(OUTPUT_KEY_A, HOLD_TIMEOUT)    // A released after timeout
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);
    platform_wait_ms(HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    // BALANCED: Should produce hold (shift layer) when timeout is reached
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 0),  // Shift layer activated immediately when B interrupts
        td_press(3012, 0),               // B pressed (on shift layer)
        td_release(3012, 0),             // B released
        td_layer(0, HOLD_TIMEOUT)        // Shift layer deactivated when A released after timeout
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_TimeoutAfterBRelease_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    press_key(KEY_B);
    release_key(KEY_B);
    platform_wait_ms(HOLD_TIMEOUT);
    release_key(TAP_DANCE_KEY);

    // HOLD_PREFERRED: Should produce hold (shift layer) when interrupted
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 0),  // Shift layer activated immediately when B interrupts
        td_press(3012, 0),               // B pressed (on shift layer)
        td_release(3012, 0),             // B released
        td_layer(0, HOLD_TIMEOUT)        // Shift layer deactivated when A released after timeout
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Test Case 5: ABBA sequence - Press A, reach hold timeout, press B, release B, release A
// Sequence: LSFT_T(KC_A) down, wait past hold timeout, KC_B down, KC_B up, LSFT_T(KC_A) up
// Expected: All flavors should produce hold (shift layer) then KC_B

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, wait past hold timeout, B down, B up, A up
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down
    platform_wait_ms(HOLD_TIMEOUT);       // Wait exactly to hold timeout (200ms)
    press_key(KEY_B);                      // KC_B down
    release_key(KEY_B);                    // KC_B up
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up

    // Should produce hold (shift layer) then KC_B on shift layer
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),    // Shift layer activated at timeout
        td_press(3012, 0),                             // B pressed (on shift layer)
        td_release(3012, 0),                           // B released
        td_layer(0, 0)                                 // Shift layer deactivated when A released
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(HOLD_TIMEOUT);
    press_key(KEY_B);
    release_key(KEY_B);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),
        td_press(3012, 0),
        td_release(3012, 0),
        td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABBA_AfterTimeout_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(HOLD_TIMEOUT);
    press_key(KEY_B);
    release_key(KEY_B);
    release_key(TAP_DANCE_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),
        td_press(3012, 0),
        td_release(3012, 0),
        td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Test Case 6: ABAB sequence - Press A, press B, release A, release B (before hold timeout)
// Sequence: LSFT_T(KC_A) down, KC_B down, LSFT_T(KC_A) up, KC_B up
// All actions happen before hold timeout (based on the first table in the image)
// Expected behavior varies by flavor

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, B down, A up, B up (all before hold timeout)
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down at time 0
    platform_wait_ms(110);                // Wait 110ms
    press_key(KEY_B);                      // KC_B down at time 110
    platform_wait_ms(20);                 // Wait 20ms (total 130ms)
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up at time 130
    platform_wait_ms(10);                 // Wait 10ms (total 140ms)
    release_key(KEY_B);                    // KC_B up at time 140

    // DEFAULT/TAP_PREFERRED: Should produce "ab" - tap action followed by B
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 110 + 20),  // A pressed when A is released (at 130ms)
        td_press(KEY_B, 110),              // B pressed at 110ms
        td_release(OUTPUT_KEY_A, 0),       // A released immediately after press
        td_release(KEY_B, 10)              // B released at 140ms (10ms after A release)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(110);
    press_key(KEY_B);
    platform_wait_ms(20);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(10);
    release_key(KEY_B);

    // PERMISSIVE_HOLD/BALANCED: Should produce "ab" - same as TAP_PREFERRED
    std::vector<tap_dance_event_t> expected_events = {
        td_press(OUTPUT_KEY_A, 110 + 20),
        td_press(KEY_B, 110),
        td_release(OUTPUT_KEY_A, 0),
        td_release(KEY_B, 10)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_BeforeTimeout_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(110);
    press_key(KEY_B);
    platform_wait_ms(20);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(10);
    release_key(KEY_B);

    // HOLD_ON_OTHER_KEY_PRESS/HOLD_PREFERRED: Should produce "B" - hold action with B on shift layer
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, 110),  // Shift layer activated when B is pressed (at 110ms)
        td_press(3012, 0),                  // B pressed on shift layer
        td_layer(0, 20),                    // Shift layer deactivated when A released (at 130ms)
        td_release(3012, 10)                // B released (at 140ms)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

// Test Case 7: ABAB sequence - Press A, press B, hold timeout reached, release A, release B
// Sequence: LSFT_T(KC_A) down, KC_B down, LSFT_T(KC_A) held, LSFT_T(KC_A) up, KC_B up
// Hold timeout is reached before A is released
// Expected: All flavors should produce "B" - hold action with B on shift layer

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_TapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;  // LSFT_T(KC_A)
    const uint16_t KEY_B = 3010;          // KC_B
    const uint16_t OUTPUT_KEY_A = 3003;   // KC_A output
    const uint8_t TARGET_LAYER_SHIFT = 1; // LSFT layer

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}  // Shift layer
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_TAP_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Sequence: A down, B down, hold timeout reached, A up, B up
    press_key(TAP_DANCE_KEY);              // LSFT_T(KC_A) down at time 0
    platform_wait_ms(110);                // Wait 110ms
    press_key(KEY_B);                      // KC_B down at time 110
    platform_wait_ms(90);                 // Wait 90ms (total 200ms - hold timeout reached)
    release_key(TAP_DANCE_KEY);            // LSFT_T(KC_A) up at time 205
    platform_wait_ms(5);                  // Wait 5ms (total 210ms)
    release_key(KEY_B);                    // KC_B up at time 210

    // Should produce "B" - hold action with B on shift layer
    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),  // Shift layer activated at hold timeout (200ms)
        td_press(3012, 110),                         // B pressed on shift layer (at 110ms)
        td_layer(0, 5),                              // Shift layer deactivated when A released (at 205ms)
        td_release(3012, 5)                          // B released (at 210ms)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_Balanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(110);
    press_key(KEY_B);
    platform_wait_ms(90);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(5);
    release_key(KEY_B);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),
        td_press(3012, 110),
        td_layer(0, 5),
        td_release(3012, 5)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}

TEST_F(InterruptFlavorsTest, TapHold_ABAB_WithTimeout_HoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t KEY_B = 3010;
    const uint16_t OUTPUT_KEY_A = 3003;
    const uint8_t TARGET_LAYER_SHIFT = 1;

    static const platform_keycode_t keymaps[2][1][2] = {
        {{ TAP_DANCE_KEY, KEY_B }},
        {{ 3011, 3012 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 1, 2);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY_A),
        createbehaviouraction_hold(1, TARGET_LAYER_SHIFT, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(110);
    press_key(KEY_B);
    platform_wait_ms(90);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(5);
    release_key(KEY_B);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(TARGET_LAYER_SHIFT, HOLD_TIMEOUT),
        td_press(3012, 110),
        td_layer(0, 5),
        td_release(3012, 5)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match(expected_events));
}
