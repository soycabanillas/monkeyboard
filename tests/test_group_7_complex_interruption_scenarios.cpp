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

class ComplexInterruptionScenariosTest : public ::testing::Test {
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

// Test 7.1: Multiple Sequential Interruptions - TAP_PREFERRED
// Objective: Verify multiple interrupting keys are all ignored with TAP_PREFERRED strategy
// Configuration: TAP_DANCE_KEY = 3000, Strategy: TAP_PREFERRED
// Tap actions: [1: SENDKEY(3001)], Hold actions: [1: CHANGELAYER(1)]
// INTERRUPTING_KEY_1 = 3010, INTERRUPTING_KEY_2 = 3011
// Hold timeout: 200ms, Tap timeout: 200ms
TEST_F(ComplexInterruptionScenariosTest, MultipleSequentialInterruptionsTapPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms (first interrupt)
    press_key(INTERRUPTING_KEY_2, 40);   // t=70ms (second interrupt)
    release_key(INTERRUPTING_KEY_1, 30); // t=100ms
    release_key(INTERRUPTING_KEY_2, 50); // t=150ms
    release_key(TAP_DANCE_KEY, 30);      // t=180ms (before hold timeout)
    wait_ms(200);              // t=380ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30), release(INTERRUPTING_KEY_1, 100),
        press(INTERRUPTING_KEY_2, 70), release(INTERRUPTING_KEY_2, 150),
        press(3001, 380), release(3001, 380)  // Tap action (all interruptions ignored)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.2: Multiple Sequential Interruptions - BALANCED
// Objective: Verify BALANCED strategy triggers hold on first complete press/release cycle
// Configuration: Same as Test 7.1, but Strategy: BALANCED
TEST_F(ComplexInterruptionScenariosTest, MultipleSequentialInterruptionsBalanced) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
    press_key(INTERRUPTING_KEY_2, 20);   // t=50ms
    release_key(INTERRUPTING_KEY_1, 30); // t=80ms (first complete cycle)
    release_key(INTERRUPTING_KEY_2, 40); // t=120ms (second complete cycle)
    release_key(TAP_DANCE_KEY, 30);      // t=150ms
    wait_ms(200);              // t=350ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30), release(INTERRUPTING_KEY_1, 80),
        press(INTERRUPTING_KEY_2, 50), release(INTERRUPTING_KEY_2, 120),
        release(3001, 150)  // Tap action (hold triggered by first complete cycle)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.3: Multiple Sequential Interruptions - HOLD_PREFERRED
// Objective: Verify HOLD_PREFERRED triggers hold on first key press
// Configuration: Same as Test 7.1, but Strategy: HOLD_PREFERRED
TEST_F(ComplexInterruptionScenariosTest, MultipleSequentialInterruptionsHoldPreferred) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms (first interrupt - triggers hold)
    press_key(INTERRUPTING_KEY_2, 20);   // t=50ms (second interrupt - ignored)
    release_key(INTERRUPTING_KEY_1, 30); // t=80ms
    release_key(INTERRUPTING_KEY_2, 40); // t=120ms
    release_key(TAP_DANCE_KEY, 30);      // t=150ms
    wait_ms(200);              // t=350ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30),
        release(INTERRUPTING_KEY_1, 80),
        press(3001, 80), release(3001, 80),  // Hold action triggered immediately
        release(3001, 150)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.4: Rapid Interruption Sequence
// Objective: Verify system handles very rapid interruption patterns
// Configuration: Same as Test 7.1, Strategy: BALANCED
TEST_F(ComplexInterruptionScenariosTest, RapidInterruptionSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    // Rapid fire interruptions
    press_key(INTERRUPTING_KEY_1, 10);   // t=10ms
    release_key(INTERRUPTING_KEY_1, 5);  // t=15ms (very fast complete cycle)
    press_key(INTERRUPTING_KEY_2, 5);    // t=20ms
    release_key(INTERRUPTING_KEY_2, 5);  // t=25ms (second fast cycle)
    release_key(TAP_DANCE_KEY, 25);      // t=50ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 10), release(INTERRUPTING_KEY_1, 15),
        press(INTERRUPTING_KEY_2, 20), release(INTERRUPTING_KEY_2, 25)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0}; // Hold triggered by first rapid cycle
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.5: Overlapping Interruption Windows
// Objective: Verify behavior when interrupting keys have overlapping press/release windows
TEST_F(ComplexInterruptionScenariosTest, OverlappingInterruptionWindows) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][3][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
    press_key(INTERRUPTING_KEY_2, 20);   // t=50ms (overlap begins)
    release_key(INTERRUPTING_KEY_1, 40); // t=90ms (first key releases while second still held)
    release_key(INTERRUPTING_KEY_2, 30); // t=120ms
    release_key(TAP_DANCE_KEY, 30);      // t=150ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30),
        press(INTERRUPTING_KEY_2, 50),
        release(INTERRUPTING_KEY_1, 90),
        release(INTERRUPTING_KEY_2, 120)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold triggered by first complete cycle
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.6: Interruption During Different States
// Objective: Verify interruption behavior during different state machine states
TEST_F(ComplexInterruptionScenariosTest, InterruptionDuringDifferentStates) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // Test interruption during WAITING_FOR_HOLD
    press_key(TAP_DANCE_KEY);            // t=0ms (enter WAITING_FOR_HOLD)
    press_key(INTERRUPTING_KEY_1, 50);   // t=50ms (interrupt during WAITING_FOR_HOLD)
    release_key(INTERRUPTING_KEY_1, 50); // t=100ms (complete cycle)
    release_key(TAP_DANCE_KEY, 50);      // t=150ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 50),
        release(INTERRUPTING_KEY_1, 100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold action triggered
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    reset_mock_state();

    // Test interruption during WAITING_FOR_TAP
    press_key(TAP_DANCE_KEY);            // t=0ms
    release_key(TAP_DANCE_KEY, 100);     // t=100ms (enter WAITING_FOR_TAP)
    press_key(INTERRUPTING_KEY_1, 50);   // t=150ms (interrupt during WAITING_FOR_TAP)
    release_key(INTERRUPTING_KEY_1, 50); // t=200ms
    wait_ms(200);              // t=400ms

    std::vector<key_action_t> expected_keys_2 = {
        press(INTERRUPTING_KEY_1, 150),
        release(INTERRUPTING_KEY_1, 200),
        press(3001, 300), release(3001, 300)  // Original sequence completes normally
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys_2));
}

// Test 7.7: Interruption Race with Timeout
// Objective: Verify interruption vs timeout race conditions
TEST_F(ComplexInterruptionScenariosTest, InterruptionRaceWithTimeout) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 199);  // t=199ms (1ms before hold timeout)
    release_key(INTERRUPTING_KEY_1, 2);  // t=201ms (complete cycle after timeout)
    release_key(TAP_DANCE_KEY, 49);      // t=250ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 199),
        release(INTERRUPTING_KEY_1, 201)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold timeout wins (earlier timestamp)
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.8: Chain of Interruptions with Different Strategies
// Objective: Verify how different strategies handle chains of interruptions
TEST_F(ComplexInterruptionScenariosTest, ChainOfInterruptionsWithDifferentStrategies) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][6][1] = {
        {{ TAP_DANCE_KEY }, { 3010 }, { 3011 }, { 3012 }, { 3013 }, { 3014 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 6, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // HOLD_PREFERRED Chain - should trigger on first key press
    press_key(TAP_DANCE_KEY);            // t=0ms
    for (int i = 0; i < 5; i++) {
        press_key(3010 + i, i * 10);     // Sequential interrupting keys
        release_key(3010 + i, 20);       // All with complete cycles
    }
    release_key(TAP_DANCE_KEY, 50);      // t=150ms

    std::vector<key_action_t> expected_keys = {
        press(3010, 0),                   // First interrupt triggers hold immediately
        release(3010, 20),
        press(3011, 10), release(3011, 30),
        press(3012, 20), release(3012, 40),
        press(3013, 30), release(3013, 50),
        press(3014, 40), release(3014, 60)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold triggered by first key press
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.9: Interruption with Multi-Tap Sequence
// Objective: Verify interruption behavior during multi-tap sequences
TEST_F(ComplexInterruptionScenariosTest, InterruptionWithMultiTapSequence) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms (1st tap)
    release_key(TAP_DANCE_KEY, 50);      // t=50ms
    press_key(TAP_DANCE_KEY, 50);        // t=100ms (2nd tap begins)
    press_key(INTERRUPTING_KEY_1, 30);   // t=130ms (interrupt during 2nd tap)
    release_key(INTERRUPTING_KEY_1, 40); // t=170ms (complete cycle)
    release_key(TAP_DANCE_KEY, 30);      // t=200ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 130),
        release(INTERRUPTING_KEY_1, 170)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {2, 0};  // Hold action for 2nd tap count
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.10: Interruption Timing Precision
// Objective: Verify precise timing of interruption processing
TEST_F(ComplexInterruptionScenariosTest, InterruptionTimingPrecision) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    wait_ms(1000);             // t=1000ms (establish baseline)
    press_key(TAP_DANCE_KEY);            // t=1000ms
    press_key(INTERRUPTING_KEY_1, 50);   // t=1050ms (precise interrupt timing)
    release_key(INTERRUPTING_KEY_1, 50); // t=1100ms
    release_key(TAP_DANCE_KEY, 50);      // t=1150ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 1050),
        release(INTERRUPTING_KEY_1, 1100)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold triggered at exact interrupt time
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.11: Complex Interruption Pattern - Nested Timing
// Objective: Verify handling of complex nested interruption patterns
// Configuration: Same as Test 7.1, Strategy: BALANCED
TEST_F(ComplexInterruptionScenariosTest, ComplexInterruptionPatternNestedTiming) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;
    const uint16_t INTERRUPTING_KEY_2 = 3011;

    static const platform_keycode_t keymaps[1][4][1] = {
        {{ TAP_DANCE_KEY }, { 3001 }, { INTERRUPTING_KEY_1 }, { INTERRUPTING_KEY_2 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 20);   // t=20ms
      press_key(INTERRUPTING_KEY_2, 10); // t=30ms (nested interrupt)
      release_key(INTERRUPTING_KEY_2, 20); // t=50ms (nested complete)
    release_key(INTERRUPTING_KEY_1, 30); // t=80ms (first complete)
    release_key(TAP_DANCE_KEY, 20);      // t=100ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 20),
        press(INTERRUPTING_KEY_2, 30), release(INTERRUPTING_KEY_2, 50),
        release(INTERRUPTING_KEY_1, 80)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0}; // Hold triggered by first complete cycle (nested)
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.12: Interruption State Recovery
// Objective: Verify system properly recovers state after complex interruption sequences
TEST_F(ComplexInterruptionScenariosTest, InterruptionStateRecovery) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    // First sequence with interruptions
    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
    release_key(INTERRUPTING_KEY_1, 40); // t=70ms
    release_key(TAP_DANCE_KEY, 30);      // t=100ms
    wait_ms(200);              // t=300ms (first sequence completes)

    // Second sequence should start clean
    press_key(TAP_DANCE_KEY, 50);        // t=350ms
    wait_ms(250);              // t=600ms (hold timeout)
    release_key(TAP_DANCE_KEY);          // t=600ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 30),
        release(INTERRUPTING_KEY_1, 70)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0, 1, 0};  // First sequence hold, second sequence hold
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.13: Maximum Interruption Load
// Objective: Verify system handles high number of interrupting keys
TEST_F(ComplexInterruptionScenariosTest, MaximumInterruptionLoad) {
    const uint16_t TAP_DANCE_KEY = 3000;

    static const platform_keycode_t keymaps[1][11][1] = {
        {{ TAP_DANCE_KEY }, { 3010 }, { 3011 }, { 3012 }, { 3013 }, { 3014 },
         { 3015 }, { 3016 }, { 3017 }, { 3018 }, { 3019 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 11, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    // 10 rapid interrupting keys
    for (int i = 0; i < 10; i++) {
        press_key(3010 + i, i * 5);      // t=i*5ms (staggered presses)
        release_key(3010 + i, 20);       // t=i*5+20ms (staggered releases)
    }
    release_key(TAP_DANCE_KEY, 100);     // t=150ms

    // All interrupting keys should be processed
    std::vector<key_action_t> expected_keys = {
        press(3010, 0), release(3010, 20)   // First key completes cycle, triggers hold
    };
    // Add remaining keys to expected output
    for (int i = 1; i < 10; i++) {
        expected_keys.push_back(press(3010 + i, i * 5));
        expected_keys.push_back(release(3010 + i, i * 5 + 20));
    }
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold triggered by first complete cycle
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

// Test 7.14: Interruption with Overflow Scenarios
// Objective: Verify interruption behavior during action overflow
TEST_F(ComplexInterruptionScenariosTest, InterruptionWithOverflowScenarios) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)  // Only 1st tap has hold
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_config->length++;

    tap_key(TAP_DANCE_KEY, 20);          // t=0-20ms (1st tap)
    tap_key(TAP_DANCE_KEY, 30, 20);      // t=50-80ms (2nd tap)
    press_key(TAP_DANCE_KEY, 30);        // t=110ms (3rd tap - overflow)
    press_key(INTERRUPTING_KEY_1, 30);   // t=140ms (interrupt during overflow)
    release_key(INTERRUPTING_KEY_1, 30); // t=170ms (complete cycle)
    release_key(TAP_DANCE_KEY, 30);      // t=200ms
    wait_ms(200);              // t=400ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 140),
        release(INTERRUPTING_KEY_1, 170),
        press(3002, 400), release(3002, 400)  // Tap action (no hold available for 3rd tap)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));
}

// Test 7.15: Interruption Edge Case - Simultaneous Events
// Objective: Verify behavior when trigger and interrupt events occur simultaneously
TEST_F(ComplexInterruptionScenariosTest, InterruptionEdgeCaseSimultaneousEvents) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    tap_dance_config->behaviours[tap_dance_config->length] = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_config->length++;

    press_key(TAP_DANCE_KEY);            // t=0ms
    press_key(INTERRUPTING_KEY_1, 0);    // t=0ms (simultaneous with trigger)
    release_key(INTERRUPTING_KEY_1, 50); // t=50ms
    release_key(TAP_DANCE_KEY, 50);      // t=100ms

    std::vector<key_action_t> expected_keys = {
        press(INTERRUPTING_KEY_1, 0),     // Both processed at same time
        release(INTERRUPTING_KEY_1, 50)
    };
    EXPECT_TRUE(g_mock_state.key_actions_match_with_time_gaps(expected_keys));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold triggered immediately
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

