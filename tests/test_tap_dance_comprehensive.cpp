#include <sys/wait.h>
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

    void tap_key(uint16_t keycode, uint16_t hold_ms = 0) {
        press_key(keycode);
        release_key(keycode, hold_ms);
    }

    void tap_key(uint16_t keycode, uint16_t delay_before_ms, uint16_t hold_ms) {
        press_key(keycode, delay_before_ms);
        release_key(keycode, hold_ms);
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
        createbehaviouraction_tap(1, OUTPUT_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    tap_key(TAP_DANCE_KEY);
    platform_wait_ms(250);  // Wait for timeout

    g_mock_state.print_state();

    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY), release(OUTPUT_KEY)         // Tap dance output
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
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
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // First tap
    tap_key(TAP_DANCE_KEY);

    // Second tap
    tap_key(TAP_DANCE_KEY, 50, 50);

    // Third tap
    tap_key(TAP_DANCE_KEY, 50, 50);

    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY), release(OUTPUT_KEY),
        press(OUTPUT_KEY), release(OUTPUT_KEY),
        press(OUTPUT_KEY), release(OUTPUT_KEY)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
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
    std::vector<key_action_t> expected_keys = {
        press(NORMAL_KEY), release(NORMAL_KEY)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

    std::vector<uint8_t> expected_layers = {}; // No layer changes
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
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
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);
    platform_wait_ms(250);  // Wait for hold timeout

    release_key(TAP_DANCE_KEY);

    std::vector<key_action_t> expected_keys = {
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

    std::vector<uint8_t> expected_layers = {TARGET_LAYER, BASE_LAYER};
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

TEST_F(TapDanceComprehensiveTest, HoldReleasedBeforeTimeout) {
    const uint16_t TAP_DANCE_KEY = 6000;
    const uint16_t OUTPUT_KEY = 6001;
    // const uint8_t BASE_LAYER = 0;
    const uint8_t TARGET_LAYER = 1;

    // Begin keymap setup
    static const platform_keycode_t keymaps[2][2][2] = {
        { // BASE_LAYER
            { TAP_DANCE_KEY, 6010 },
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
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, TARGET_LAYER, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    press_key(TAP_DANCE_KEY);   // Press key
    platform_wait_ms(100);  // Wait less than hold timeout
    release_key(TAP_DANCE_KEY); // Release before timeout
    platform_wait_ms(250);  // Wait for tap timeout

    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY), release(OUTPUT_KEY)         // Tap output
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
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
        createbehaviouraction_tap(1, SINGLE_TAP_KEY),
        createbehaviouraction_tap(2, DOUBLE_TAP_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // First tap
    tap_key(TAP_DANCE_KEY);
    // Should wait for potential second tap, no tap output yet
    std::vector<key_action_t> expected_keys = {
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));

    // Second tap
    tap_key(TAP_DANCE_KEY, 50);
    platform_wait_ms(250);  // Wait for timeout

    expected_keys = {
        press(DOUBLE_TAP_KEY), release(DOUBLE_TAP_KEY)   // Double tap output
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
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
        createbehaviouraction_tap(1, SINGLE_TAP_KEY),
        createbehaviouraction_tap(2, DOUBLE_TAP_KEY),
        createbehaviouraction_tap(3, TRIPLE_TAP_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;
    // End tap dance config

    tap_key(TAP_DANCE_KEY);
    tap_key(TAP_DANCE_KEY, 50);
    tap_key(TAP_DANCE_KEY, 50);
    platform_wait_ms(250);

    std::vector<key_action_t> expected_keys = {
        press(TRIPLE_TAP_KEY), release(TRIPLE_TAP_KEY)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
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
        createbehaviouraction_tap(1, SINGLE_TAP_KEY),
        createbehaviouraction_tap(2, DOUBLE_TAP_KEY)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;
    // End tap dance config

    // Three taps (exceeds configuration)
    tap_key(TAP_DANCE_KEY);
    tap_key(TAP_DANCE_KEY, 50);
    tap_key(TAP_DANCE_KEY, 50);

    platform_wait_ms(250);

    std::vector<key_action_t> expected_keys = {
        press(DOUBLE_TAP_KEY), release(DOUBLE_TAP_KEY),
        press(SINGLE_TAP_KEY), release(SINGLE_TAP_KEY)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match(expected_keys));
}
