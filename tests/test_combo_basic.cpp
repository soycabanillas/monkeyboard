#include <stdint.h>
#include <sys/types.h>
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
#include "pipeline_combo.h"
#include "pipeline_combo_initializer.h"
#include "pipeline_executor.h"
}

class Combo_Basic_Test : public ::testing::Test {
protected:
    pipeline_combo_global_config_t* combo_config;

    void SetUp() override {
        reset_mock_state();
        pipeline_combo_global_state_create();

        size_t n_elements = 10;
        combo_config = static_cast<pipeline_combo_global_config_t*>(
            malloc(sizeof(*combo_config)));
        combo_config->length = 0;
        combo_config->combos = static_cast<pipeline_combo_config_t**>(
            malloc( n_elements * sizeof(pipeline_combo_config_t*)));

        pipeline_executor_create_config(1, 0);
        pipeline_executor_add_physical_pipeline(0, &pipeline_combo_callback_process_data, &pipeline_combo_callback_reset, combo_config);



    }

    void TearDown() override {
        if (pipeline_executor_config) {
            free(pipeline_executor_config);
            pipeline_executor_config = nullptr;
        }
        if (combo_config) {
            free(combo_config);
            combo_config = nullptr;
        }
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
const uint16_t KEY_A = 3010;
const uint16_t KEY_B = 3011;
const uint16_t KEY_C = 3012;
const uint16_t KEY_D = 3013;
const uint16_t KEY_E = 3014;
const uint16_t KEY_F = 3015;
const uint16_t KEY_G = 3016;
const uint16_t KEY_H = 3017;

static KeyboardSimulator set_scenario(pipeline_combo_global_config_t* combo_config) {
    static const platform_keycode_t keymaps[1][1][4] = {
        {{ KEY_A, COMBO_KEY_A, COMBO_KEY_B, KEY_C }}
    };
    KeyboardSimulator keyboard = create_layout((const uint16_t*)keymaps, 1, 1, 4);

    pipeline_combo_key_t* keys[] = { 
         create_combo_key(platform_keypos_t{0,1}, create_combo_key_action(COMBO_KEY_ACTION_NONE, 0), create_combo_key_action(COMBO_KEY_ACTION_NONE, 0)),
         create_combo_key(platform_keypos_t{0,2}, create_combo_key_action(COMBO_KEY_ACTION_NONE, 0), create_combo_key_action(COMBO_KEY_ACTION_NONE, 0))
    };

    combo_config->combos[0] = create_combo(2, keys, create_combo_key_action(COMBO_KEY_ACTION_NONE, KEY_A), create_combo_key_action(COMBO_KEY_ACTION_UNREGISTER, KEY_A));
    combo_config->length++;
    
    return keyboard;
}


// Test Case 1: AABB sequence - Press A, release A, press B, release B
// Sequence: LSFT_T(KC_A) down, LSFT_T(KC_A) up, KC_B down, KC_B up
// All actions happen before hold timeout
// Expected: All flavors should produce tap (KC_A) then KC_B

TEST_F(Combo_Basic_Test, FirstTest) {
    KeyboardSimulator keyboard = set_scenario(combo_config);

    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 0);
    keyboard.release_key_at(COMBO_KEY_A, 1);
    keyboard.release_key_at(COMBO_KEY_B, 1);

    // Should produce tap (KC_A) then KC_B
    std::vector<tap_dance_event_t> expected_events = {
        td_press(KEY_A, 0),
        td_release(KEY_A, 199),
    };
    EXPECT_TRUE(g_mock_state.tap_dance_event_actions_match_absolute(expected_events));
}
