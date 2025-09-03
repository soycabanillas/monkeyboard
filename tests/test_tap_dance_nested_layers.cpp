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

class TapDanceNestedTest : public ::testing::Test {
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

TEST_F(TapDanceNestedTest, SimpleNestingABBA) {
    const uint16_t TAP_DANCE_KEY_1 = 2001;
    const uint16_t OUTPUT_KEY_1 = 2002;
    const uint16_t INTERRUPTING_KEY_1 = 2003;
    const uint16_t TAP_DANCE_KEY_2 = 2111;
    const uint16_t OUTPUT_KEY_2 = 2112;
    const uint16_t INTERRUPTING_KEY_2 = 2113;

    static const platform_keycode_t keymaps[3][1][4] = {
        {{ TAP_DANCE_KEY_1, INTERRUPTING_KEY_1, 2051,            2052 }},
        {{ 2151,            2152,               TAP_DANCE_KEY_2, INTERRUPTING_KEY_2 }},
        {{ 2053,            2054,               2055,            2056 }}
    };

    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 3, 1, 4);

    {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction_tap(1, OUTPUT_KEY_1),
            createbehaviouraction_hold(1, 1, TAP_DANCE_HOLD_PREFERRED)
        };
        pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY_1, actions, 2);
        tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
        tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
        tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
        tap_dance_config->length++;
    }

    {
        pipeline_tap_dance_action_config_t* actions[] = {
            createbehaviouraction_tap(1, OUTPUT_KEY_2),
            createbehaviouraction_hold(1, 2, TAP_DANCE_HOLD_PREFERRED)
        };
        pipeline_tap_dance_behaviour_t* tap_dance_behavior = createbehaviour(TAP_DANCE_KEY_2, actions, 2);
        tap_dance_behavior->config->hold_timeout = 200; // Set hold timeout to 200ms
        tap_dance_behavior->config->tap_timeout = 200; // Set tap timeout to 200ms
        tap_dance_config->behaviours[tap_dance_config->length] = tap_dance_behavior;
        tap_dance_config->length++;
    }

    keyboard.press_key_at(TAP_DANCE_KEY_1, 0);
    keyboard.press_key_at(2051, 200);
    keyboard.release_key_at(2051, 400);
    keyboard.release_key_at(TAP_DANCE_KEY_1, 400);

    std::vector<tap_dance_event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(1, 400),
        td_layer(0, 400)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
