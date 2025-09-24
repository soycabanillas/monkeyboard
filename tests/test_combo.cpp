#include <stdint.h>
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "keyboard_simulator.hpp"
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "test_scenario.hpp"
#include "combo_test_helpers.hpp"

extern "C" {
#include "pipeline_combo.h"
#include "pipeline_combo_initializer.h"
#include "pipeline_executor.h"
}

class Combo_Basic_Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};


const uint16_t COMBO_KEY_A = 3000;
const uint16_t COMBO_KEY_B = 3001;
const uint16_t COMBO_KEY_C = 3002;
const uint16_t COMBO_KEY_D = 3003;
const uint16_t COMBO_KEY_E = 3004;
const uint16_t COMBO_KEY_F = 3005;
const uint16_t COMBO_KEY_G = 3006;
const uint16_t COMBO_KEY_H = 3007;
const uint16_t KEY_A = 4;
const uint16_t KEY_B = 5;
const uint16_t KEY_C = 6;
const uint16_t KEY_D = 7;
const uint16_t KEY_E = 8;
const uint16_t KEY_F = 9;
const uint16_t KEY_G = 10;
const uint16_t KEY_H = 11;

TEST_F(Combo_Basic_Test, CB1AP_CB1BP_CB1AR_CB1BR) {
    // Define keymap using vectors
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        {{ KEY_A, COMBO_KEY_A, COMBO_KEY_B, KEY_C }}
    }};

    // Create test scenario and build combo configuration
    TestScenario scenario(keymap);
    
    ComboConfigBuilder config_builder;
    std::vector<ComboKeyBuilder> combo_keys = {
        ComboKeyBuilder({0,1}),
        ComboKeyBuilder({0,2})
    };
    pipeline_combo_key_translation_t press_action = create_combo_key_action(COMBO_KEY_ACTION_REGISTER, KEY_A);
    pipeline_combo_key_translation_t release_action = create_combo_key_action(COMBO_KEY_ACTION_UNREGISTER, KEY_A);
    
    config_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_combo(combo_keys, press_action, release_action)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 10);
    keyboard.release_key_at(COMBO_KEY_A, 20);
    keyboard.release_key_at(COMBO_KEY_B, 30);

    // Should produce tap (KEY_A)
    std::vector<event_t> expected_events = {
        td_press(KEY_A, 10),
        td_release(KEY_A, 30),
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(Combo_Basic_Test, TestOld) {
    pipeline_executor_create_config(1, 0);

    // Begin keymap setup
    static const platform_keycode_t keymaps[1][1][2] = {
        {{ COMBO_KEY_A, COMBO_KEY_B }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 2);

    pipeline_combo_global_state_create();
    size_t n_elements = 1;
    pipeline_combo_global_config_t* combo_config = (pipeline_combo_global_config_t*)malloc(sizeof(*combo_config));
    combo_config->length = n_elements;
    combo_config->combos = (pipeline_combo_config_t**)malloc(n_elements * sizeof(pipeline_combo_config_t*));
    combo_config->strategy = COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON;

    pipeline_executor_add_physical_pipeline(0, &pipeline_combo_callback_process_data_executor, &pipeline_combo_callback_reset_executor, combo_config);

    pipeline_combo_key_translation_t press_action_none = create_combo_key_action(COMBO_KEY_ACTION_NONE, 0);
    pipeline_combo_key_translation_t release_action_none = create_combo_key_action(COMBO_KEY_ACTION_NONE, 0);

    // Define a combo: pressing 'A' and 'S' together sends 'ESC'
    platform_keypos_t keypos_a = { 0, 0 }; // Example key position
    pipeline_combo_key_t* combo_key_a = create_combo_key(keypos_a, press_action_none, release_action_none);

    platform_keypos_t keypos_b = { 0, 1 }; // Example key position
    pipeline_combo_key_t* combo_key_b = create_combo_key(keypos_b, press_action_none, release_action_none);

    pipeline_combo_key_t* combo1_keys[] = { combo_key_a, combo_key_b };

    pipeline_combo_key_translation_t press_action_combo1 = create_combo_key_action(COMBO_KEY_ACTION_REGISTER, KEY_C);
    pipeline_combo_key_translation_t release_action_combo1 = create_combo_key_action(COMBO_KEY_ACTION_UNREGISTER, KEY_C);

    pipeline_combo_config_t* combo1 = create_combo(2, combo1_keys, press_action_combo1, release_action_combo1);
    combo_config->combos[0] = combo1;


    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 10);
    keyboard.release_key_at(COMBO_KEY_A, 20);
    keyboard.release_key_at(COMBO_KEY_B, 30);

    std::vector<event_t> expected_events = {
        td_press(KEY_C, 10),
        td_release(KEY_C, 30)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));

}