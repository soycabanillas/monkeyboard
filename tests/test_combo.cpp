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

// Test Case 1: AABB sequence - Press A, release A, press B, release B
// Sequence: LSFT_T(KC_A) down, LSFT_T(KC_A) up, KC_B down, KC_B up
// All actions happen before hold timeout
// Expected: All flavors should produce tap (KC_A) then KC_B

TEST_F(Combo_Basic_Test, FirstTest) {
    // Define keymap using vectors
    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        {{ KEY_A, COMBO_KEY_A, COMBO_KEY_B, KEY_C }}
    }};

    // Create test scenario and build combo configuration
    TestScenario scenario(keymap);
    
    ComboConfigBuilder config_builder;
    config_builder
        .with_strategy(COMBO_STRATEGY_DISCARD_WHEN_ONE_PRESSED_IN_COMMON)
        .add_simple_combo({{0,1}, {0,2}}, KEY_A)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(COMBO_KEY_A, 0);
    keyboard.press_key_at(COMBO_KEY_B, 10);
    keyboard.release_key_at(COMBO_KEY_A, 20);
    keyboard.release_key_at(COMBO_KEY_B, 30);

    // Should produce tap (KC_A) then KC_B
    std::vector<event_t> expected_events = {
        td_press(KEY_A, 10),
        td_release(KEY_A, 30),
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}
