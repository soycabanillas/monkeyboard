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
#include "pipeline_key_replacer.h"
#include "pipeline_key_replacer_initializer.h"
#include "pipeline_executor.h"
}

class KeyReplacer : public ::testing::Test {
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

// Simple Key Replacer
// Objective: Verify key replacer functionality with a single output
TEST_F(KeyReplacer, SimpleKeyReplacerWithSingleOutput) {
    const uint16_t ONE_SHOT_KEY = 100;
    const uint16_t OUTPUT_KEY1 = 101;
    const uint16_t OUTPUT_KEY2 = 102;

    static const platform_keycode_t keymaps[1][1][2] = {{{ ONE_SHOT_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);


    size_t number_of_pairs = 1;
    pipeline_key_replacer_global_config_t global_config = {};
    global_config.length = number_of_pairs;
    global_config.modifier_pairs = static_cast<pipeline_key_replacer_pair_t**>(malloc(sizeof(pipeline_key_replacer_pair_t*) * number_of_pairs));
    platform_key_replacer_event_buffer_t* press_event_buffer = static_cast<platform_key_replacer_event_buffer_t*>(malloc(sizeof(platform_key_replacer_event_buffer_t)));
    press_event_buffer->buffer_length = 1;
    press_event_buffer->buffer[0].keycode = OUTPUT_KEY1;
    platform_key_replacer_event_buffer_t* release_event_buffer = static_cast<platform_key_replacer_event_buffer_t*>(malloc(sizeof(platform_key_replacer_event_buffer_t)));
    release_event_buffer->buffer_length = 1;
    release_event_buffer->buffer[0].keycode = OUTPUT_KEY2;
    global_config.modifier_pairs[0] = pipeline_key_replacer_create_pairs(ONE_SHOT_KEY, press_event_buffer, release_event_buffer);
    pipeline_executor_create_config(0, 1);
    pipeline_executor_add_virtual_pipeline(0, &pipeline_key_replacer_callback_process_data, &pipeline_key_replacer_callback_reset, &global_config);


    keyboard.press_key(ONE_SHOT_KEY);
    keyboard.release_key(ONE_SHOT_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(OUTPUT_KEY1, 0),
        td_report_send(0),
        td_report_release(OUTPUT_KEY2, 0),
        td_report_send(0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}

// Simple Key Replacer
// Objective: Verify key replacer functionality with multiple outputs
TEST_F(KeyReplacer, SimpleKeyReplacerWithMultipleOutputs) {
    const uint16_t ONE_SHOT_KEY = 100;
    const uint16_t OUTPUT_KEY1 = 101;
    const uint16_t OUTPUT_KEY2 = 102;
    const uint16_t OUTPUT_KEY3 = 103;
    const uint16_t OUTPUT_KEY4 = 104;

    static const platform_keycode_t keymaps[1][1][2] = {{{ ONE_SHOT_KEY }}};
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 1);


    size_t number_of_pairs = 1;
    pipeline_key_replacer_global_config_t global_config = {};
    global_config.length = number_of_pairs;
    global_config.modifier_pairs = static_cast<pipeline_key_replacer_pair_t**>(malloc(sizeof(pipeline_key_replacer_pair_t*) * number_of_pairs));
    platform_key_replacer_event_buffer_t* press_event_buffer = static_cast<platform_key_replacer_event_buffer_t*>(malloc(sizeof(platform_key_replacer_event_buffer_t)));
    press_event_buffer->buffer_length = 2;
    press_event_buffer->buffer[0].keycode = OUTPUT_KEY1;
    press_event_buffer->buffer[1].keycode = OUTPUT_KEY2;
    platform_key_replacer_event_buffer_t* release_event_buffer = static_cast<platform_key_replacer_event_buffer_t*>(malloc(sizeof(platform_key_replacer_event_buffer_t)));
    release_event_buffer->buffer_length = 2;
    release_event_buffer->buffer[0].keycode = OUTPUT_KEY3;
    release_event_buffer->buffer[1].keycode = OUTPUT_KEY4;
    global_config.modifier_pairs[0] = pipeline_key_replacer_create_pairs(ONE_SHOT_KEY, press_event_buffer, release_event_buffer);
    pipeline_executor_create_config(0, 1);
    pipeline_executor_add_virtual_pipeline(0, &pipeline_key_replacer_callback_process_data, &pipeline_key_replacer_callback_reset, &global_config);


    keyboard.press_key(ONE_SHOT_KEY);
    keyboard.release_key(ONE_SHOT_KEY);

    std::vector<tap_dance_event_t> expected_events = {
        td_report_press(OUTPUT_KEY1, 0),
        td_report_press(OUTPUT_KEY2, 0),
        td_report_send(0),
        td_report_release(OUTPUT_KEY3, 0),
        td_report_release(OUTPUT_KEY4, 0),
        td_report_send(0)
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_relative(expected_events));
}
