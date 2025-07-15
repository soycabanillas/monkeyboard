#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

// Test keycodes - using different keycodes to avoid conflicts
enum KEY_CODES {
    KC_A,
    KC_B,
    KC_C,
    KC_D,
    KC_E,
    KC_F,
    KC_G,
    KC_H,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_M,
    KC_N,
    KC_O,
    KC_P,
    TEST_KEY_TAP_DANCE_1,
    TEST_KEY_TAP_DANCE_2,
    TEST_KEY_TAP_DANCE_3
};

// Layer aliases for readability
enum KEY_LAYERS {
    LAYER_BASE,
    LAYER_SYMBOLS,
    LAYER_NUMBERS,
    LAYER_FUNCTION
};

class TapDanceComprehensiveTest : public ::testing::Test {
protected:
    pipeline_tap_dance_global_config_t* global_config;

    void SetUp() override {
        reset_mock_state();

        // Create pipeline executor
        size_t n_pipelines = 1;
        pipeline_executor_config = static_cast<pipeline_executor_config_t*>(
            malloc(sizeof(pipeline_executor_config_t) + n_pipelines * sizeof(pipeline_t*)));

        pipeline_tap_dance_global_state_create();

        // Create tap dance configuration with enough space for comprehensive tests
        size_t n_elements = 10;
        global_config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*global_config) + n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));
        global_config->length = 0; // Will be set as we add configurations

        platform_keycode_t keymaps[][4][4] = {
            [LAYER_BASE] = {
                { KC_A, KC_B, KC_C, KC_D},
                {KC_E, KC_F, KC_G, KC_H},
                {KC_I, KC_J, KC_K, KC_L},
                {KC_M, KC_N, KC_O, KC_P}
            },
            [LAYER_SYMBOLS] = {
                { KC_A, KC_B, KC_C, KC_D},
                {KC_E, KC_F, KC_G, KC_H},
                {KC_I, KC_J, KC_K, KC_L},
                {KC_M, KC_N, KC_O, KC_P}
            },
            [LAYER_NUMBERS] = {
                { KC_A, KC_B, KC_C, KC_D},
                {KC_E, KC_F, KC_G, KC_H},
                {KC_I, KC_J, KC_K, KC_L},
                {KC_M, KC_N, KC_O, KC_P}
            },
            [LAYER_FUNCTION] = {
                { KC_A, KC_B, KC_C, KC_D },
                {KC_E, KC_F, KC_G, KC_H},
                {KC_I, KC_J, KC_K, KC_L},
                {KC_M, KC_N, KC_O, KC_P}
            }
        };

        platform_layout_init_2d_keymap((const uint16_t*)keymaps, 4, 4, 4);

        pipeline_executor_global_state_create();
        pipeline_executor_config->length = n_pipelines;
        pipeline_executor_config->pipelines[0] = add_pipeline(&pipeline_tap_dance_callback, global_config);
    }

    void TearDown() override {
        // Cleanup allocated memory
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
        if (global_config) {
            free(global_config);
            global_config = nullptr;
        }
    }

    // This function is used to find the key position based on the keycode
    // This assumes a 4x4 grid for simplicity, adjust as needed
    // It returns a platform_keypos_t structure with row and column
    platform_keypos_t get_keypos(uint16_t keycode) {
        uint8_t layer = platform_layout_get_current_layer();
        uint8_t row = 0, col = 0;

        for (uint16_t i = 0; i < 4 * 4; i++) {
            if (platform_layout_get_keycode_from_layer(layer, {row, col}) == keycode) {
                return {row, col};
            }
            col++;
            if (col >= 4) {
                col = 0;
                row++;
            }
        }
        return {0, 0}; // Default return if not found
    }

    void setup_simple_tap_config(uint16_t keycode, uint16_t output_key, uint8_t tap_count = 1) {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction(tap_count, TDCL_TAP_KEY_SENDKEY, output_key, 0)
        };
        global_config->behaviours[global_config->length] = createbehaviour(keycode, actions, 1);
        global_config->length++;
    }

    void setup_simple_hold_config(uint16_t keycode, uint8_t layer, uint8_t tap_count = 1) {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction(tap_count, TDCL_HOLD_KEY_CHANGELAYERTEMPO, keycode, layer)
        };
        global_config->behaviours[global_config->length] = createbehaviour(keycode, actions, 1);
        global_config->length++;
    }

    void setup_tap_and_hold_config(uint16_t keycode, uint16_t tap_key, uint8_t layer, uint8_t tap_count = 1) {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction(tap_count, TDCL_TAP_KEY_SENDKEY, tap_key, 0),
            createbehaviouraction(tap_count, TDCL_HOLD_KEY_CHANGELAYERTEMPO, keycode, layer)
        };
        global_config->behaviours[global_config->length] = createbehaviour(keycode, actions, 2);
        global_config->length++;
    }

    void setup_multi_tap_config(uint16_t keycode, uint16_t key1, uint16_t key2, uint16_t key3 = 0) {
        if (key3 != 0) {
            pipeline_tap_dance_action_config_t* actions[] = {
                createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, key1, 0),
                createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, key2, 0),
                createbehaviouraction(3, TDCL_TAP_KEY_SENDKEY, key3, 0)
            };
            global_config->behaviours[global_config->length] = createbehaviour(keycode, actions, 3);
        } else {
            pipeline_tap_dance_action_config_t* actions[] = {
                createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, key1, 0),
                createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, key2, 0)
            };
            global_config->behaviours[global_config->length] = createbehaviour(keycode, actions, 2);
        }
        global_config->length++;
    }

    void setup_interrupt_config(uint16_t keycode, uint16_t tap_key, uint8_t layer, int16_t interrupt_config, uint8_t tap_count = 1) {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction(tap_count, TDCL_TAP_KEY_SENDKEY, tap_key, 0),
            createbehaviouraction_with_interrupt(tap_count, TDCL_HOLD_KEY_CHANGELAYERTEMPO, keycode, layer, interrupt_config)
        };
        global_config->behaviours[global_config->length] = createbehaviour(keycode, actions, 2);
        global_config->length++;
    }

    void simulate_key_event(uint16_t keycode, bool pressed, uint16_t time_offset = 0) {
        if (time_offset > 0) {
            platform_wait_ms(time_offset);
        }

        platform_keypos_t keypos = get_keypos(keycode);

        abskeyevent_t event;
        event.key.row = keypos.row;
        event.key.col = keypos.col;
        event.pressed = pressed;
        event.time = static_cast<uint16_t>(platform_timer_read());

        pipeline_process_key(event);
    }

    void reset_test_state() {
        reset_mock_state();
        global_config->length = 0;
    }
};

// ==================== BASIC TAP FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicSingleTap) {
    setup_simple_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);   // Press
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);  // Release
    platform_wait_ms(250);  // Wait for timeout

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
}

TEST_F(TapDanceComprehensiveTest, KeyRepetitionException) {
    setup_tap_and_hold_config(TEST_KEY_TAP_DANCE_1, KC_A, LAYER_SYMBOLS, 1);

    // First tap
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);

    // Second tap (should work due to repetition exception)
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);

    // Third tap
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 3);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
}

TEST_F(TapDanceComprehensiveTest, NoActionConfigured) {
    // Empty configuration - no tap dance behaviors set up

    simulate_key_event(KC_B, true);   // Press regular key
    simulate_key_event(KC_B, false);  // Release regular key
    platform_wait_ms(250);  // Wait

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 0);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}

// ==================== BASIC HOLD FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicHoldTimeout) {
    setup_simple_hold_config(TEST_KEY_TAP_DANCE_1, LAYER_SYMBOLS, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);   // Press and hold
    platform_wait_ms(250);  // Wait for hold timeout
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);  // Release key
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_BASE);
}

TEST_F(TapDanceComprehensiveTest, HoldReleasedBeforeTimeout) {
    setup_tap_and_hold_config(TEST_KEY_TAP_DANCE_1, KC_A, LAYER_SYMBOLS, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);   // Press key
    platform_wait_ms(100);  // Wait less than hold timeout
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false); // Release before timeout
    platform_wait_ms(250);  // Wait for tap timeout

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_BASE);
}

// ==================== MULTI-TAP SEQUENCES ====================

TEST_F(TapDanceComprehensiveTest, DoubleTap) {
    setup_multi_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, KC_C);

    // First tap
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 0); // Should wait for potential second tap

    // Second tap
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    platform_wait_ms(250);  // Wait for timeout

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_C);
}

TEST_F(TapDanceComprehensiveTest, TripleTap) {
    setup_multi_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, KC_C, KC_D);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_D);
}

TEST_F(TapDanceComprehensiveTest, TapCountExceedsConfiguration) {
    setup_multi_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, KC_C);

    // Three taps (exceeds configuration)
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A); // Should reset and execute first tap action
}

// ==================== INTERRUPT CONFIGURATION ====================

TEST_F(TapDanceComprehensiveTest, InterruptConfigMinus1) {
    setup_interrupt_config(TEST_KEY_TAP_DANCE_1, KC_A, LAYER_SYMBOLS, -1, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);  // Start hold
    simulate_key_event(KC_B, true, 50);        // Interrupt with another key
    simulate_key_event(KC_B, false, 50);       // Release interrupting key

    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, false); // Release tap dance key
}

TEST_F(TapDanceComprehensiveTest, InterruptConfigZero) {
    setup_interrupt_config(TEST_KEY_TAP_DANCE_1, KC_A, LAYER_SYMBOLS, 0, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);  // Start hold
    simulate_key_event(KC_B, true, 50);        // Interrupt with another key

    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS);

    simulate_key_event(KC_B, false, 50);       // Release interrupting key
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false); // Release tap dance key
}

TEST_F(TapDanceComprehensiveTest, InterruptConfigPositive) {
    setup_interrupt_config(TEST_KEY_TAP_DANCE_1, KC_A, LAYER_SYMBOLS, 100, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);  // Start hold
    platform_wait_ms(50);  // Wait less than interrupt config time
    simulate_key_event(KC_B, true);            // Interrupt early

    // Should send original key and interrupting key, hold action should be discarded
    EXPECT_GE(g_mock_state.send_key_calls_count(), 1);

    simulate_key_event(KC_B, false);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0); // No layer changes
}

// ==================== NESTING BEHAVIOR ====================

TEST_F(TapDanceComprehensiveTest, DifferentKeycodesCanNest) {
    reset_test_state();
    setup_simple_hold_config(TEST_KEY_TAP_DANCE_1, LAYER_SYMBOLS, 1);
    setup_simple_tap_config(TEST_KEY_TAP_DANCE_2, KC_A, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);   // Start first tap dance
    platform_wait_ms(250);  // Activate hold
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS);

    simulate_key_event(TEST_KEY_TAP_DANCE_2, true, 50);  // Start nested tap dance
    simulate_key_event(TEST_KEY_TAP_DANCE_2, false);     // Complete nested tap
    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);     // Release first key
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_BASE);
}

TEST_F(TapDanceComprehensiveTest, SameKeycodeNestingIgnored) {
    reset_test_state();
    setup_simple_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);      // First press
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);  // Second press - should be ignored
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);     // First release
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);     // Second release - should be ignored
    platform_wait_ms(250);  // Wait for timeout

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A);
}

// ==================== LAYER STACK MANAGEMENT ====================

TEST_F(TapDanceComprehensiveTest, ComplexLayerStackDependencies) {
    reset_test_state();
    setup_simple_hold_config(TEST_KEY_TAP_DANCE_1, LAYER_SYMBOLS, 1);
    setup_simple_hold_config(TEST_KEY_TAP_DANCE_2, LAYER_NUMBERS, 1);
    setup_simple_hold_config(TEST_KEY_TAP_DANCE_3, LAYER_FUNCTION, 1);

    // Build up layer stack
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);   // Layer 1
    platform_wait_ms(250);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS);

    simulate_key_event(TEST_KEY_TAP_DANCE_2, true);   // Layer 2
    platform_wait_ms(250);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_NUMBERS);

    simulate_key_event(TEST_KEY_TAP_DANCE_3, true);   // Layer 3
    platform_wait_ms(250);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_FUNCTION);

    // Release in reverse order
    simulate_key_event(TEST_KEY_TAP_DANCE_3, false);  // Release layer 3
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_NUMBERS); // Should return to layer 2

    simulate_key_event(TEST_KEY_TAP_DANCE_2, false);  // Release layer 2
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS); // Should return to layer 1

    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);  // Release layer 1
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_BASE); // Should return to base
}

// ==================== TIMING AND STATE MANAGEMENT ====================

TEST_F(TapDanceComprehensiveTest, FastKeySequences) {
    reset_test_state();
    setup_multi_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, KC_C);

    // Very fast double tap
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    platform_wait_ms(10);  // Very short delay
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_C); // Should still register as double tap
}

TEST_F(TapDanceComprehensiveTest, MixedTapHoldSequence) {
    reset_test_state();
    // Setup a complex config: 1 tap = X, 2 taps = Y, 2-tap hold = layer symbols
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, KC_A, 0),
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, KC_C, 0),
        createbehaviouraction_with_interrupt(2, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TEST_KEY_TAP_DANCE_1, LAYER_SYMBOLS, 0)
    };
    global_config->behaviours[global_config->length] = createbehaviour(TEST_KEY_TAP_DANCE_1, actions, 3);
    global_config->length++;

    // First tap
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);

    // Second tap but hold
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    platform_wait_ms(250);  // Hold second tap
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_SYMBOLS);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    EXPECT_EQ(g_mock_state.last_selected_layer, LAYER_BASE);
}

// ==================== EDGE CASES ====================

TEST_F(TapDanceComprehensiveTest, VeryFastTapRelease) {
    reset_test_state();
    setup_simple_tap_config(TEST_KEY_TAP_DANCE_1, KC_A, 1);

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    platform_wait_ms(1);  // 1ms hold
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    platform_wait_ms(250);

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_A); // Should work even with very fast tap
}

TEST_F(TapDanceComprehensiveTest, ImmediateExecutionOnFinalTapCount) {
    reset_test_state();
    // Only double-tap configured
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, KC_C, 0)
    };
    global_config->behaviours[global_config->length] = createbehaviour(TEST_KEY_TAP_DANCE_1, actions, 1);
    global_config->length++;

    simulate_key_event(TEST_KEY_TAP_DANCE_1, true);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, true, 50);
    simulate_key_event(TEST_KEY_TAP_DANCE_1, false);

    EXPECT_EQ(g_mock_state.send_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_sent_key, KC_C); // Should execute immediately without timeout
}
