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
#include "pipeline_oneshot_modifier.h"
#include "pipeline_oneshot_modifier_initializer.h"
#include "pipeline_executor.h"
}

class OneShotModifier : public ::testing::Test {
protected:
    void SetUp() override {
        reset_mock_state();
    }

    void TearDown() override {
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
    }
};

// Simple One-Shot Modifier
// Objective: Verify basic one-shot modifier functionality
TEST_F(OneShotModifier, SimpleOneShotModifier) {
    const uint16_t ONE_SHOT_KEY = 100;
    const uint16_t OUTPUT_KEY = 101;

    static const platform_keycode_t keymaps[1][1][2] = {{{ ONE_SHOT_KEY, OUTPUT_KEY }}};
    platform_layout_init_2d_keymap((const uint16_t*)keymaps, 1, 1, 2);


    size_t number_of_pairs = 1;
    pipeline_oneshot_modifier_global_status_t* global_status = pipeline_oneshot_modifier_global_state_create();
    pipeline_oneshot_modifier_global_config_t* global_config = static_cast<pipeline_oneshot_modifier_global_config_t*>(malloc(sizeof(*global_config) + sizeof(pipeline_oneshot_modifier_pair_t*) * number_of_pairs));
    global_config->length = number_of_pairs;
    global_config->modifier_pairs[0] = pipeline_oneshot_modifier_create_pairs(ONE_SHOT_KEY, MACRO_KEY_MODIFIER_LEFT_CTRL);
    pipeline_oneshot_modifier_global_t global;
    global.config = global_config;
    global.status = global_status;
    pipeline_executor_create_config(0, 1);
    pipeline_executor_add_virtual_pipeline(0, &pipeline_oneshot_modifier_callback_process_data, &pipeline_oneshot_modifier_callback_reset, &global);


    press_key(ONE_SHOT_KEY);
    release_key(ONE_SHOT_KEY);
    press_key(OUTPUT_KEY);
    release_key(OUTPUT_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(PLATFORM_KC_LEFT_CTRL, 0),
        td_report_press(OUTPUT_KEY, 0),
        td_report_send(0),
        td_report_release(PLATFORM_KC_LEFT_CTRL, 0),
        td_report_send(0),
        td_release(OUTPUT_KEY),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}
