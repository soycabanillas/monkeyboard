#include <sys/wait.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "common_functions.hpp"

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

    std::vector<key_action_t> expected_keys = {
        press(OUTPUT_KEY, 0), release(OUTPUT_KEY, 0)         // Tap dance output
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
        press(OUTPUT_KEY, 0), release(OUTPUT_KEY, 0),
        press(OUTPUT_KEY, 100), release(OUTPUT_KEY, 0),
        press(OUTPUT_KEY, 100), release(OUTPUT_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
        press(NORMAL_KEY, 0), release(NORMAL_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

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
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

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
        press(OUTPUT_KEY, 100), release(OUTPUT_KEY, 0)         // Tap output
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    // Second tap
    tap_key(TAP_DANCE_KEY, 50);
    platform_wait_ms(250);  // Wait for timeout

    expected_keys = {
        press(DOUBLE_TAP_KEY, 0), release(DOUBLE_TAP_KEY, 50)   // Double tap output
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
        press(TRIPLE_TAP_KEY, 50), release(TRIPLE_TAP_KEY, 50)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
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
        press(DOUBLE_TAP_KEY, 0), release(DOUBLE_TAP_KEY, 50),
        press(SINGLE_TAP_KEY, 50 + g_tap_timeout), release(SINGLE_TAP_KEY, 0)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

