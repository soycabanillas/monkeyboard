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

class TapDanceComprehensiveTest : public ::testing::Test {
protected:
    pipeline_tap_dance_global_config_t* tap_dance_config;

    void SetUp() override {
        reset_mock_state();

        // Minimal setup - just initialize the global state
        pipeline_tap_dance_global_state_create();

        // Initialize with empty configuration that tests can customize
        size_t n_elements = 10;
        tap_dance_config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*tap_dance_config) + n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));
        tap_dance_config->length = 0;

        // Create minimal pipeline executor
        pipeline_executor_create_config(1, 0);
        pipeline_executor_add_physical_pipeline(0, &pipeline_tap_dance_callback_process_data, &pipeline_tap_dance_callback_reset, tap_dance_config);
    }

    void TearDown() override {
        // Cleanup allocated memory
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
        if (tap_dance_config) {
            free(tap_dance_config);
            tap_dance_config = nullptr;
        }
    }

    // Generic keypos finder for any keymap size
    platform_keypos_t find_keypos(uint16_t keycode, uint8_t max_rows = 4, uint8_t max_cols = 4) {
        uint8_t layer = platform_layout_get_current_layer();

        for (uint8_t row = 0; row < max_rows; row++) {
            for (uint8_t col = 0; col < max_cols; col++) {
                if (platform_layout_get_keycode_from_layer(layer, {row, col}) == keycode) {
                    return {row, col};
                }
            }
        }
        return {0, 0}; // Default return if not found
    }

    // Simplified key event simulation
    void press_key(uint16_t keycode, uint16_t delay_ms = 0) {
        if (delay_ms > 0) platform_wait_ms(delay_ms);

        platform_keypos_t keypos = find_keypos(keycode, 4, 4);
        abskeyevent_t event = {
            .keypos = keypos,
            .pressed = true,
            .time = static_cast<uint16_t>(platform_timer_read())
        };

        if (pipeline_process_key(event)) {
            platform_register_keycode(keycode);
        }
    }

    void release_key(uint16_t keycode, uint16_t delay_ms = 0) {
        if (delay_ms > 0) platform_wait_ms(delay_ms);

        platform_keypos_t keypos = find_keypos(keycode, 4, 4);
        abskeyevent_t event = {
            .keypos = keypos,
            .pressed = false,
            .time = static_cast<uint16_t>(platform_timer_read())
        };

        if (pipeline_process_key(event)) {
            platform_unregister_keycode(keycode);
        }
    }

    void tap_key(uint16_t keycode, uint16_t hold_ms = 50, uint16_t delay_before_ms = 0) {
        press_key(keycode, delay_before_ms);
        release_key(keycode, hold_ms);
    }

    void simulate_key_event(uint16_t keycode, bool pressed, uint16_t time_offset = 0) {
        if (time_offset > 0) {
            platform_wait_ms(time_offset);
        }

        platform_keypos_t keypos = find_keypos(keycode, 4, 4);

        abskeyevent_t event;
        event.keypos.row = keypos.row;
        event.keypos.col = keypos.col;
        event.pressed = pressed;
        event.time = static_cast<uint16_t>(platform_timer_read());

        if (pipeline_process_key(event) == true) {
            // If the key was not processed, we can simulate a fallback action
            if (pressed) {
                printf("Key %d pressed but not processed, simulating fallback action", keycode);
                platform_register_keycode(keycode);
            } else {
                printf("Key %d released but not processed, simulating fallback action", keycode);
                platform_unregister_keycode(keycode);
            }
        }
    }

    void reset_test_state() {
        reset_mock_state();
        tap_dance_config->length = 0;
    }
};

// ==================== BASIC TAP FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicSingleTap) {
    // Custom setup for this test
    const uint16_t TAP_DANCE_KEY = 2000;
    const uint16_t OUTPUT_KEY = 2001;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {
        {{ TAP_DANCE_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, OUTPUT_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    tap_key(TAP_DANCE_KEY);
    platform_wait_ms(250);  // Wait for timeout

    g_mock_state.print_state();

    EXPECT_GE(g_mock_state.register_key_calls_count(), 1);
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, OUTPUT_KEY);
}

TEST_F(TapDanceComprehensiveTest, KeyRepetitionException) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t OUTPUT_KEY = 3001;
    const uint8_t TARGET_LAYER = 1;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY, 3010 },
            { 3011, 3012 }
        },
        { // TARGET_LAYER
            { 3020, 3021 },
            { 3022, 3023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, OUTPUT_KEY, 0),
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // First tap
    tap_key(TAP_DANCE_KEY);
    EXPECT_GE(g_mock_state.register_key_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);

    // Second tap (should work due to repetition exception)
    tap_key(TAP_DANCE_KEY, 50, 50);
    EXPECT_GE(g_mock_state.register_key_calls_count(), 4);
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);

    // Third tap
    tap_key(TAP_DANCE_KEY, 50, 50);
    EXPECT_GE(g_mock_state.register_key_calls_count(), 6);
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);
}

TEST_F(TapDanceComprehensiveTest, NoActionConfigured) {
    const uint16_t NORMAL_KEY = 4000;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {
        {{ NORMAL_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);
    // End keymap setup
    // No tap dance configuration - empty config

    tap_key(NORMAL_KEY);
    platform_wait_ms(250);

    // Should only have the original key press/release, no tap dance actions
    EXPECT_EQ(g_mock_state.register_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.unregister_key_calls_count(), 1);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0);
}

// ==================== BASIC HOLD FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicHoldTimeout) {
    const uint16_t TAP_DANCE_KEY = 5000;
    const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 1;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY, 5010 },
            { 5011, 5012 }
        },
        { // TARGET_LAYER
            { 5020, 5021 },
            { 5022, 5023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(250);  // Wait for hold timeout
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER);

    release_key(TAP_DANCE_KEY);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_selected_layer, BASE_LAYER);
}

TEST_F(TapDanceComprehensiveTest, HoldReleasedBeforeTimeout) {
    const uint16_t TAP_DANCE_KEY = 6000;
    const uint16_t OUTPUT_KEY = 6001;
    // const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 1;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY, OUTPUT_KEY },
            { 6011, 6012 }
        },
        { // TARGET_LAYER
            { 6020, 6021 },
            { 6022, 6023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, OUTPUT_KEY, 0),
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);   // Press key
    platform_wait_ms(100);  // Wait less than hold timeout
    release_key(TAP_DANCE_KEY); // Release before timeout
    platform_wait_ms(250);  // Wait for tap timeout

    // Should tap OUTPUT_KEY (register + unregister)
    EXPECT_GE(g_mock_state.register_key_calls_count(), 2); // Original press + tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 2); // Original release + tap output
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, OUTPUT_KEY);
}

// ==================== MULTI-TAP SEQUENCES ====================

TEST_F(TapDanceComprehensiveTest, DoubleTap) {
    const uint16_t TAP_DANCE_KEY = 7000;
    const uint16_t SINGLE_TAP_KEY = 7001;
    const uint16_t DOUBLE_TAP_KEY = 7011;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY, 7010 },
            { 7012, 7013 }
        },
        { // TARGET_LAYER
            { 7020, 7021 },
            { 7022, 7023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, SINGLE_TAP_KEY, 0),
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, DOUBLE_TAP_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // First tap
    tap_key(TAP_DANCE_KEY);
    // Should wait for potential second tap, no tap output yet
    EXPECT_EQ(g_mock_state.register_key_calls_count(), 1); // Only original key press
    EXPECT_EQ(g_mock_state.unregister_key_calls_count(), 1); // Only original key release

    // Second tap
    tap_key(TAP_DANCE_KEY, 50);
    platform_wait_ms(250);  // Wait for timeout

    // Should output DOUBLE_TAP_KEY for double tap
    EXPECT_GE(g_mock_state.register_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_EQ(g_mock_state.last_registered_key, DOUBLE_TAP_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, DOUBLE_TAP_KEY);
}

TEST_F(TapDanceComprehensiveTest, TripleTap) {
    const uint16_t TAP_DANCE_KEY = 8000;
    const uint16_t SINGLE_TAP_KEY = 8001;
    const uint16_t DOUBLE_TAP_KEY = 8011;
    const uint16_t TRIPLE_TAP_KEY = 8012;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // Layer 0
            { TAP_DANCE_KEY, 8010 },
            { 8013, 8014 }
        },
        { // Layer 1
            { 8020, 8021 },
            { 8022, 8023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, SINGLE_TAP_KEY, 0),
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, DOUBLE_TAP_KEY, 0),
        createbehaviouraction(3, TDCL_TAP_KEY_SENDKEY, TRIPLE_TAP_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;
    // End tap dance config

    tap_key(TAP_DANCE_KEY);
    tap_key(TAP_DANCE_KEY, 50);
    tap_key(TAP_DANCE_KEY, 50);
    platform_wait_ms(250);

    // Should output TRIPLE_TAP_KEY for triple tap
    EXPECT_GE(g_mock_state.register_key_calls_count(), 4); // 3 original + 1 tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 4); // 3 original + 1 tap output
    EXPECT_EQ(g_mock_state.last_registered_key, TRIPLE_TAP_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, TRIPLE_TAP_KEY);
}

TEST_F(TapDanceComprehensiveTest, TapCountExceedsConfiguration) {
    const uint16_t TAP_DANCE_KEY = 9000;
    const uint16_t SINGLE_TAP_KEY = 9001;
    const uint16_t DOUBLE_TAP_KEY = 9011;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // Layer 0
            { TAP_DANCE_KEY, 9010 },
            { 9012, 9013 }
        },
        { // Layer 1
            { 9020, 9021 },
            { 9022, 9023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config - only single and double tap configured
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, SINGLE_TAP_KEY, 0),
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, DOUBLE_TAP_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // Three taps (exceeds configuration)
    tap_key(TAP_DANCE_KEY);
    tap_key(TAP_DANCE_KEY, 50);
    tap_key(TAP_DANCE_KEY, 50);

    // Should reset and execute first tap action
    EXPECT_GE(g_mock_state.register_key_calls_count(), 4); // 3 original + 1 tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 4); // 3 original + 1 tap output
    EXPECT_EQ(g_mock_state.last_registered_key, SINGLE_TAP_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, SINGLE_TAP_KEY);
}

// ==================== INTERRUPT CONFIGURATION ====================

TEST_F(TapDanceComprehensiveTest, InterruptConfigMinus1) {
    const uint16_t TAP_DANCE_KEY = 10000;
    const uint16_t OUTPUT_KEY = 10001;
    const uint16_t INTERRUPT_KEY = 10002;
    // const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 2;

    // Begin keymap setup
    static const platform_keycode_t keymaps[3][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY, OUTPUT_KEY },
            { INTERRUPT_KEY, 10003 }
        },
        { // Layer 1 (unused)
            { 10100, 10101 },
            { 10102, 10103 }
        },
        { // TARGET_LAYER
            { 10020, 10021 },
            { 10022, 10023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 3);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);  // Start hold
    press_key(INTERRUPT_KEY, 50);        // Interrupt with another key
    release_key(INTERRUPT_KEY, 50);       // Release interrupting key

    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER);

    release_key(TAP_DANCE_KEY); // Release tap dance key
}

TEST_F(TapDanceComprehensiveTest, InterruptConfigZero) {
    const uint16_t TAP_DANCE_KEY = 11000;
    const uint16_t OUTPUT_KEY = 11001;
    const uint8_t TARGET_LAYER = 2;

    // Begin keymap setup
    static const platform_keycode_t keymaps[3][2][2] = {
        { // Layer 0
            { TAP_DANCE_KEY, OUTPUT_KEY },
            { 11002, 11003 }
        },
        { // Layer 1
            { 11120, 11121 },
            { 11122, 11123 }
        },
        { // TARGET_LAYER
            { 11020, 11021 },
            { 11022, 11023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 3);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);  // Start hold
    press_key(11002, 50);        // Interrupt with another key

    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER);

    release_key(11002, 50);       // Release interrupting key
    release_key(TAP_DANCE_KEY); // Release tap dance key
}

TEST_F(TapDanceComprehensiveTest, InterruptConfigPositive) {
    const uint16_t TAP_DANCE_KEY = 12000;
    const uint16_t OUTPUT_KEY = 12001;
    const uint8_t TARGET_LAYER = 2;

    // Begin keymap setup
    static const platform_keycode_t keymaps[3][2][2] = {
        { // Layer 0
            { TAP_DANCE_KEY, OUTPUT_KEY },
            { 12002, 12003 }
        },
        { // Layer 1
            { 12120, 12121 },
            { 12122, 12123 }
        },
        { // TARGET_LAYER
            { 12020, 12021 },
            { 12022, 12023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 3);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);  // Start hold
    platform_wait_ms(50);  // Wait less than interrupt config time
    press_key(12002);            // Interrupt early

    // Should send original key and interrupting key, hold action should be discarded
    EXPECT_GE(g_mock_state.register_key_calls_count(), 2); // At least tap dance + interrupt keys

    release_key(12002);
    release_key(TAP_DANCE_KEY);
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 0); // No layer changes
}

// ==================== NESTING BEHAVIOR ====================

TEST_F(TapDanceComprehensiveTest, DifferentKeycodesCanNest) {
    const uint16_t TAP_DANCE_KEY_1 = 13000;
    const uint16_t TAP_DANCE_KEY_2 = 13001;
    const uint16_t OUTPUT_KEY = 13002;
    const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 1;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY_1, TAP_DANCE_KEY_2 },
            { 13010, 13011 }
        },
        { // TARGET_LAYER
            { 13020, 13021 },
            { 13022, 13023 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 2, 2, 2);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions1[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY_1, TARGET_LAYER)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY_1, actions1, 1);
    tap_dance_config->length++;

    pipeline_tap_dance_action_config_t* actions2[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, OUTPUT_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY_2, actions2, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY_1);   // Start first tap dance
    platform_wait_ms(250);  // Activate hold
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER);

    press_key(TAP_DANCE_KEY_2, 50);  // Start nested tap dance
    release_key(TAP_DANCE_KEY_2);     // Complete nested tap
    EXPECT_GE(g_mock_state.register_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 2); // 1 original + 1 tap output
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, OUTPUT_KEY);

    release_key(TAP_DANCE_KEY_1);     // Release first key
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 2);
    EXPECT_EQ(g_mock_state.last_selected_layer, BASE_LAYER);
}

TEST_F(TapDanceComprehensiveTest, SameKeycodeNestingIgnored) {
    const uint16_t TAP_DANCE_KEY = 14000;
    const uint16_t OUTPUT_KEY = 14001;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {
        {{ TAP_DANCE_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, OUTPUT_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);      // First press
    press_key(TAP_DANCE_KEY, 50);  // Second press - should be ignored
    release_key(TAP_DANCE_KEY);     // First release
    release_key(TAP_DANCE_KEY);     // Second release - should be ignored
    platform_wait_ms(250);  // Wait for timeout

    EXPECT_GE(g_mock_state.register_key_calls_count(), 2); // Original + tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 2); // Original + tap output
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, OUTPUT_KEY);
}

// ==================== LAYER STACK MANAGEMENT ====================

TEST_F(TapDanceComprehensiveTest, ComplexLayerStackDependencies) {
    const uint16_t TAP_DANCE_KEY_1 = 15000;
    const uint16_t TAP_DANCE_KEY_2 = 15001;
    const uint16_t TAP_DANCE_KEY_3 = 15002;
    const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER_1 = 1;
    const uint8_t TARGET_LAYER_2 = 2;
    const uint8_t TARGET_LAYER_3 = 3;

    // Begin keymap setup
    static const platform_keycode_t keymaps[4][3][3] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY_1, 15010, 15011 },
            { TAP_DANCE_KEY_2, 15012, 15013 },
            { TAP_DANCE_KEY_3, 15014, 15015 }
        },
        { // TARGET_LAYER_1
            { 15020, 15021, 15022 },
            { 15023, 15024, 15025 },
            { 15026, 15027, 15028 }
        },
        { // TARGET_LAYER_2
            { 15030, 15031, 15032 },
            { 15033, 15034, 15035 },
            { 15036, 15037, 15038 }
        },
        { // TARGET_LAYER_3
            { 15040, 15041, 15042 },
            { 15043, 15044, 15045 },
            { 15046, 15047, 15048 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 3, 3, 4);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions1[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY_1, TARGET_LAYER_1)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY_1, actions1, 1);
    tap_dance_config->length++;

    pipeline_tap_dance_action_config_t* actions2[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY_2, TARGET_LAYER_2)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY_2, actions2, 1);
    tap_dance_config->length++;

    pipeline_tap_dance_action_config_t* actions3[] = {
        createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY_3, TARGET_LAYER_3)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY_3, actions3, 1);
    tap_dance_config->length++;
    // End tap dance config

    // Build up layer stack
    press_key(TAP_DANCE_KEY_1);   // Layer 1
    platform_wait_ms(250);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER_1);

    press_key(TAP_DANCE_KEY_2);   // Layer 2
    platform_wait_ms(250);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER_2);

    press_key(TAP_DANCE_KEY_3);   // Layer 3
    platform_wait_ms(250);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER_3);

    // Release in reverse order
    release_key(TAP_DANCE_KEY_3);  // Release layer 3
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER_2); // Should return to layer 2

    release_key(TAP_DANCE_KEY_2);  // Release layer 2
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER_1); // Should return to layer 1

    release_key(TAP_DANCE_KEY_1);  // Release layer 1
    EXPECT_EQ(g_mock_state.last_selected_layer, BASE_LAYER); // Should return to base
}

// ==================== TIMING AND STATE MANAGEMENT ====================

TEST_F(TapDanceComprehensiveTest, FastKeySequences) {
    const uint16_t TAP_DANCE_KEY = 16000;
    const uint16_t SINGLE_TAP_KEY = 16001;
    const uint16_t DOUBLE_TAP_KEY = 16002;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {
        {{ TAP_DANCE_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, SINGLE_TAP_KEY, 0),
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, DOUBLE_TAP_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // Very fast double tap
    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(10);  // Very short delay
    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(250);

    // Should still register as double tap
    EXPECT_GE(g_mock_state.register_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_EQ(g_mock_state.last_registered_key, DOUBLE_TAP_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, DOUBLE_TAP_KEY);
}

TEST_F(TapDanceComprehensiveTest, MixedTapHoldSequence) {
    const uint16_t TAP_DANCE_KEY = 17000;
    const uint16_t SINGLE_TAP_KEY = 17001;
    const uint16_t DOUBLE_TAP_KEY = 17002;
    const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 1;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][1][1] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY }
        },
        { // TARGET_LAYER
            { 17010 }
        }
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 2);
    // End keymap setup

    // Begin tap dance config - Setup a complex config: 1 tap = X, 2 taps = Y, 2-tap hold = layer symbols
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, SINGLE_TAP_KEY, 0),
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, DOUBLE_TAP_KEY, 0),
        createbehaviouraction_with_interrupt(2, TDCL_HOLD_KEY_CHANGELAYERTEMPO, TAP_DANCE_KEY, TARGET_LAYER, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;
    // End tap dance config

    // First tap
    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY);

    // Second tap but hold
    press_key(TAP_DANCE_KEY, 50);
    platform_wait_ms(250);  // Hold second tap
    EXPECT_EQ(g_mock_state.layer_select_calls_count(), 1);
    EXPECT_EQ(g_mock_state.last_selected_layer, TARGET_LAYER);

    release_key(TAP_DANCE_KEY);
    EXPECT_EQ(g_mock_state.last_selected_layer, BASE_LAYER);
}

// ==================== EDGE CASES ====================

TEST_F(TapDanceComprehensiveTest, VeryFastTapRelease) {
    const uint16_t TAP_DANCE_KEY = 18000;
    const uint16_t OUTPUT_KEY = 18001;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {
        {{ TAP_DANCE_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);
    // End keymap setup

    // Begin tap dance config
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, OUTPUT_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(1);  // 1ms hold
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(250);

    // Should work even with very fast tap
    EXPECT_GE(g_mock_state.register_key_calls_count(), 2); // Original + tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 2); // Original + tap output
    EXPECT_EQ(g_mock_state.last_registered_key, OUTPUT_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, OUTPUT_KEY);
}

TEST_F(TapDanceComprehensiveTest, ImmediateExecutionOnFinalTapCount) {
    const uint16_t TAP_DANCE_KEY = 19000;
    const uint16_t DOUBLE_TAP_KEY = 19001;

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][1] = {
        {{ TAP_DANCE_KEY }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 1);
    // End keymap setup

    // Begin tap dance config - Only double-tap configured
    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction(2, TDCL_TAP_KEY_SENDKEY, DOUBLE_TAP_KEY, 0)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);
    release_key(TAP_DANCE_KEY);
    press_key(TAP_DANCE_KEY, 50);
    release_key(TAP_DANCE_KEY);

    // Should execute immediately without timeout
    EXPECT_GE(g_mock_state.register_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_GE(g_mock_state.unregister_key_calls_count(), 3); // 2 original + 1 tap output
    EXPECT_EQ(g_mock_state.last_registered_key, DOUBLE_TAP_KEY);
    EXPECT_EQ(g_mock_state.last_unregistered_key, DOUBLE_TAP_KEY);
}
