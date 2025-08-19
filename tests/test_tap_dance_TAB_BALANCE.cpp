#include <sys/wait.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
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

class TapDanceTAPBALANCETest : public ::testing::Test {
protected:
    pipeline_tap_dance_global_config_t* tap_dance_config;

    void SetUp() override {
        reset_mock_state();

        // Minimal setup - just initialize the global state
        pipeline_tap_dance_global_state_create();

        // Initialize with empty configuration that tests can customize
        size_t n_elements = 10;
        tap_dance_config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(*tap_dance_config)));
        tap_dance_config->length = 0;
        tap_dance_config->behaviours = static_cast<pipeline_tap_dance_behaviour_t**>(
            malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*)));

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

    const uint16_t PREVIOUS_KEY_A = 2000;
    const uint16_t PREVIOUS_KEY_B = 2001;
    const uint16_t TAP_DANCE_KEY = 2002;
    const uint16_t OUTPUT_KEY = 2003;
    const uint16_t INTERRUPTING_KEY = 2004;

static KeyboardSimulator set_scenario_1hold(pipeline_tap_dance_global_config_t* tap_dance_config, tap_dance_hold_strategy_t hold_strategy) {


    static const platform_keycode_t keymaps[2][1][4] = {
        {{ PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }},
        {{ 2100, 2101, 2102, 2103 }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 2, 1, 4);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_hold(1, 1, hold_strategy)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 1);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;
    
    return keyboard;
}

static KeyboardSimulator set_scenario_1tap_1hold(pipeline_tap_dance_global_config_t* tap_dance_config, tap_dance_hold_strategy_t hold_strategy) {


    static const platform_keycode_t keymaps[2][1][4] = {
        {{ PREVIOUS_KEY_A, PREVIOUS_KEY_B, TAP_DANCE_KEY, INTERRUPTING_KEY }},
        {{ 2100, 2101, 2102, 2103 }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 2, 1, 4);

    pipeline_tap_dance_action_config_t* actions[] = {
        createbehaviouraction_tap(1, OUTPUT_KEY),
        createbehaviouraction_hold(1, 1, hold_strategy)
    };
    pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY, actions, 2);
    tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
    tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
    tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
    tap_dance_config->length++;
    
    return keyboard;
}

TEST_F(TapDanceTAPBALANCETest, 1Tap1Hold_PressA_PressTDK_ReleaseA_ReleaseTDKNoHold) {
    KeyboardSimulator keyboard = set_scenario_1tap_1hold(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_press(OUTPUT_KEY, 30),
        td_release(PREVIOUS_KEY_A, 30),
        td_release(OUTPUT_KEY, 30),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceTAPBALANCETest, 1Tap1Hold_PressA_PressTDK_ReleaseA_ReleaseTDKHold) {
    KeyboardSimulator keyboard = set_scenario_1tap_1hold(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceTAPBALANCETest, 1Hold_PressA_PressTDK_ReleaseA_ReleaseTDKNoHold) {
    KeyboardSimulator keyboard = set_scenario_1hold(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 30);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_release(PREVIOUS_KEY_A, 30),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceTAPBALANCETest, 1Hold_PressA_PressTDK_ReleaseA_ReleaseTDKHold) {
    KeyboardSimulator keyboard = set_scenario_1hold(tap_dance_config, TAP_DANCE_TAP_PREFERRED);

    keyboard.press_key_at(PREVIOUS_KEY_A, 0);
    keyboard.press_key_at(TAP_DANCE_KEY, 10);
    keyboard.release_key_at(PREVIOUS_KEY_A, 20);
    keyboard.release_key_at(TAP_DANCE_KEY, 210);

    std::vector<tap_dance_event_t> expected_events = {
        td_press(PREVIOUS_KEY_A, 0),
        td_layer(1, 210),
        td_release(PREVIOUS_KEY_A, 210),
        td_layer(0, 0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}