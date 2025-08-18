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
            malloc(sizeof(*tap_dance_config)));
        tap_dance_config->length = 0;
        tap_dance_config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_TAP_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 30);      // t=30ms (first interrupt)
    keyboard.press_key_at(INTERRUPTING_KEY_2, 70);      // t=70ms (second interrupt)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 100);   // t=100ms
    keyboard.release_key_at(INTERRUPTING_KEY_2, 150);   // t=150ms
    keyboard.release_key_at(TAP_DANCE_KEY, 180);        // t=180ms (before hold timeout)
    keyboard.wait_ms(200);                          // t=380ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 30), td_release(INTERRUPTING_KEY_1, 100),
        td_press(INTERRUPTING_KEY_2, 70), td_release(INTERRUPTING_KEY_2, 150),
        td_press(3001, 380), td_release(3001, 380)  // Tap action (all interruptions ignored)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 30);      // t=30ms
    keyboard.press_key_at(INTERRUPTING_KEY_2, 50);      // t=50ms
    keyboard.release_key_at(INTERRUPTING_KEY_1, 80);    // t=80ms (first complete cycle)
    keyboard.release_key_at(INTERRUPTING_KEY_2, 120);   // t=120ms (second complete cycle)
    keyboard.release_key_at(TAP_DANCE_KEY, 150);        // t=150ms
    keyboard.wait_ms(200);                          // t=350ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 30), td_release(INTERRUPTING_KEY_1, 80),
        td_press(INTERRUPTING_KEY_2, 50), td_release(INTERRUPTING_KEY_2, 120),
        td_layer(1, 80), td_layer(0, 150)  // Hold triggered by first complete cycle
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 30);      // t=30ms (first interrupt - triggers hold)
    keyboard.press_key_at(INTERRUPTING_KEY_2, 50);      // t=50ms (second interrupt - ignored)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 80);    // t=80ms
    keyboard.release_key_at(INTERRUPTING_KEY_2, 120);   // t=120ms
    keyboard.release_key_at(TAP_DANCE_KEY, 150);        // t=150ms
    keyboard.wait_ms(200);                          // t=350ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 30),
        td_release(INTERRUPTING_KEY_1, 80),
        td_press(INTERRUPTING_KEY_2, 50), td_release(INTERRUPTING_KEY_2, 120),
        td_layer(1, 30), td_layer(0, 150)  // Hold action triggered immediately
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    // Rapid fire interruptions
    keyboard.press_key_at(INTERRUPTING_KEY_1, 10);      // t=10ms
    keyboard.release_key_at(INTERRUPTING_KEY_1, 15);    // t=15ms (very fast complete cycle)
    keyboard.press_key_at(INTERRUPTING_KEY_2, 20);      // t=20ms
    keyboard.release_key_at(INTERRUPTING_KEY_2, 25);    // t=25ms (second fast cycle)
    keyboard.release_key_at(TAP_DANCE_KEY, 50);         // t=50ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 10), td_release(INTERRUPTING_KEY_1, 15),
        td_press(INTERRUPTING_KEY_2, 20), td_release(INTERRUPTING_KEY_2, 25),
        td_layer(1, 15), td_layer(0, 50)   // Hold triggered by first rapid cycle
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 3, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 30);      // t=30ms
    keyboard.press_key_at(INTERRUPTING_KEY_2, 50);      // t=50ms (overlap begins)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 90);    // t=90ms (first key releases while second still held)
    keyboard.release_key_at(INTERRUPTING_KEY_2, 120);   // t=120ms
    keyboard.release_key_at(TAP_DANCE_KEY, 150);        // t=150ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 30),
        td_press(INTERRUPTING_KEY_2, 50),
        td_release(INTERRUPTING_KEY_1, 90),
        td_release(INTERRUPTING_KEY_2, 120),
        td_layer(1, 90), td_layer(0, 150)  // Hold triggered by first complete cycle
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // Test interruption during WAITING_FOR_HOLD
    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms (enter WAITING_FOR_HOLD)
    keyboard.press_key_at(INTERRUPTING_KEY_1, 50);      // t=50ms (interrupt during WAITING_FOR_HOLD)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 100);   // t=100ms (complete cycle)
    keyboard.release_key_at(TAP_DANCE_KEY, 150);        // t=150ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 50),
        td_release(INTERRUPTING_KEY_1, 100),
        td_layer(1, 100), td_layer(0, 150)  // Hold action triggered
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold action triggered
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));

    reset_mock_state();

    // Test interruption during WAITING_FOR_TAP
    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);        // t=100ms (enter WAITING_FOR_TAP)
    keyboard.press_key_at(INTERRUPTING_KEY_1, 150);     // t=150ms (interrupt during WAITING_FOR_TAP)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 200);   // t=200ms
    keyboard.wait_ms(200);                          // t=400ms

    std::vector<tap_dance_event_t> expected_events_2 = {
        td_press(INTERRUPTING_KEY_1, 150),
        td_release(INTERRUPTING_KEY_1, 200),
        td_press(3001, 300), td_release(3001, 300)  // Original sequence completes normally
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events_2));
}

// Test 7.7: Interruption Race with Timeout
// Objective: Verify interruption vs timeout race conditions
TEST_F(ComplexInterruptionScenariosTest, InterruptionRaceWithTimeout) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 199);     // t=199ms (1ms before hold timeout)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 201);   // t=201ms (complete cycle after timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 250);        // t=250ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 199),
        td_release(INTERRUPTING_KEY_1, 201),
        td_layer(1, 200), td_layer(0, 250)  // Hold timeout wins (earlier timestamp)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 6, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // HOLD_PREFERRED Chain - should trigger on first key press
    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    for (int i = 0; i < 5; i++) {
        keyboard.press_key_at(3010 + i, i * 10);        // Sequential interrupting keys
        keyboard.release_key_at(3010 + i, i * 10 + 20); // All with complete cycles
    }
    keyboard.release_key_at(TAP_DANCE_KEY, 150);        // t=150ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(3010, 0),           // First interrupt triggers hold immediately
        td_release(3010, 20),
        td_layer(1, 0), td_layer(0, 150)    // Hold triggered by first key press
    };
    // Add remaining keys to expected output
    for (int i = 1; i < 5; i++) {
        expected_events.push_back(td_press(3010 + i, i * 10));
        expected_events.push_back(td_release(3010 + i, i * 10 + 20));
    }
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED),
        createbehaviouraction_hold(2, 2, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 4);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms (1st tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 50);         // t=50ms
    keyboard.press_key_at(TAP_DANCE_KEY, 100);          // t=100ms (2nd tap begins)
    keyboard.press_key_at(INTERRUPTING_KEY_1, 130);     // t=130ms (interrupt during 2nd tap)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 170);   // t=170ms (complete cycle)
    keyboard.release_key_at(TAP_DANCE_KEY, 200);        // t=200ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 130),
        td_release(INTERRUPTING_KEY_1, 170),
        td_layer(2, 170), td_layer(0, 200)  // Hold action for 2nd tap count
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.wait_ms(1000);                         // t=1000ms (establish baseline)
    keyboard.press_key_at(TAP_DANCE_KEY, 1000);         // t=1000ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 1050);    // t=1050ms (precise interrupt timing)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 1100);  // t=1100ms
    keyboard.release_key_at(TAP_DANCE_KEY, 1150);       // t=1150ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 1050),
        td_release(INTERRUPTING_KEY_1, 1100),
        td_layer(1, 1050), td_layer(0, 1150)  // Hold triggered at exact interrupt time
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 4, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 20);      // t=20ms
      keyboard.press_key_at(INTERRUPTING_KEY_2, 30);     // t=30ms (nested interrupt)
      keyboard.release_key_at(INTERRUPTING_KEY_2, 50);   // t=50ms (nested complete)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 80);    // t=80ms (first complete)
    keyboard.release_key_at(TAP_DANCE_KEY, 100);        // t=100ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 20),
        td_press(INTERRUPTING_KEY_2, 30), td_release(INTERRUPTING_KEY_2, 50),
        td_release(INTERRUPTING_KEY_1, 80),
        td_layer(1, 50), td_layer(0, 100)  // Hold triggered by first complete cycle (nested)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    // First sequence with interruptions
    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 30);      // t=30ms
    keyboard.release_key_at(INTERRUPTING_KEY_1, 70);    // t=70ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);        // t=100ms
    keyboard.wait_ms(200);                          // t=300ms (first sequence completes)

    // Second sequence should start clean
    keyboard.press_key_at(TAP_DANCE_KEY, 350);          // t=350ms
    keyboard.wait_ms(250);                          // t=600ms (hold timeout)
    keyboard.release_key_at(TAP_DANCE_KEY, 600);        // t=600ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 30),
        td_release(INTERRUPTING_KEY_1, 70),
        td_layer(1, 30), td_layer(0, 100),   // First sequence hold
        td_layer(1, 550), td_layer(0, 600)   // Second sequence hold
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 11, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    // 10 rapid interrupting keys
    for (int i = 0; i < 10; i++) {
        keyboard.press_key_at(3010 + i, i * 5);         // t=i*5ms (staggered presses)
        keyboard.release_key_at(3010 + i, i * 5 + 20);  // t=i*5+20ms (staggered releases)
    }
    keyboard.release_key_at(TAP_DANCE_KEY, 150);        // t=150ms

    // All interrupting keys should be processed
    std::vector<tap_dance_event_t> expected_events = {
        td_press(3010, 0), td_release(3010, 20),   // First key completes cycle, triggers hold
        td_layer(1, 20), td_layer(0, 150)
    };
    // Add remaining keys to expected output
    for (int i = 1; i < 10; i++) {
        expected_events.insert(expected_events.end() - 2, td_press(3010 + i, i * 5));
        expected_events.insert(expected_events.end() - 2, td_release(3010 + i, i * 5 + 20));
    }
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

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
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_tap(2, 3002),
        createbehaviouraction_hold(1, 1, TAP_DANCE_BALANCED)  // Only 1st tap has hold
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 3);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);          // t=0ms (1st tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 20);       // t=20ms
    keyboard.press_key_at(TAP_DANCE_KEY, 50);         // t=50ms (2nd tap)
    keyboard.release_key_at(TAP_DANCE_KEY, 80);       // t=80ms
    keyboard.press_key_at(TAP_DANCE_KEY, 110);        // t=110ms (3rd tap - overflow)
    keyboard.press_key_at(INTERRUPTING_KEY_1, 140);   // t=140ms (interrupt during overflow)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 170); // t=170ms (complete cycle)
    keyboard.release_key_at(TAP_DANCE_KEY, 200);      // t=200ms
    keyboard.wait_ms(200);                        // t=400ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 140),
        td_release(INTERRUPTING_KEY_1, 170),
        td_press(3002, 400), td_release(3002, 400)  // Tap action (no hold available for 3rd tap)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

// Test 7.15: Interruption Edge Case - Simultaneous Events
// Objective: Verify behavior when trigger and interrupt events occur simultaneously
TEST_F(ComplexInterruptionScenariosTest, InterruptionEdgeCaseSimultaneousEvents) {
    const uint16_t TAP_DANCE_KEY = 3000;
    const uint16_t INTERRUPTING_KEY_1 = 3010;

    static const platform_keycode_t keymaps[1][2][1] = {
        {{ TAP_DANCE_KEY }, { INTERRUPTING_KEY_1 }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 2, 1);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, 3001),
        createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;

    keyboard.press_key_at(TAP_DANCE_KEY, 0);            // t=0ms
    keyboard.press_key_at(INTERRUPTING_KEY_1, 0);       // t=0ms (simultaneous with trigger)
    keyboard.release_key_at(INTERRUPTING_KEY_1, 50);    // t=50ms
    keyboard.release_key_at(TAP_DANCE_KEY, 100);        // t=100ms

    std::vector<tap_dance_event_t> expected_events = {
        td_press(INTERRUPTING_KEY_1, 0),     // Both processed at same time
        td_release(INTERRUPTING_KEY_1, 50),
        td_layer(1, 0), td_layer(0, 100)  // Hold triggered immediately
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));

    std::vector<uint8_t> expected_layers = {1, 0};  // Hold triggered immediately
    EXPECT_TRUE(g_mock_state.layer_history_matches(expected_layers));
}

