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
#include "tap_dance_test_helpers.hpp"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class TapDanceNestedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

// Mnemonic legend for keys:
// KA = Key A
// KB = Key B
// TDKA = Tap Dance Key 1
// TDKB = Tap Dance Key 2
// TDKAOK = Output key for Tap Dance Key 1
// TDKBOK = Output key for Tap Dance Key 2
// TDKAIP = Interrupting key for Tap Dance Key 1
// TDKBIP = Interrupting key for Tap Dance Key 2

// Mnemonic legend for events:
// 1TapTDKA = tap activation after 1 key count (single tap) for key A
// 2TapTDKB = tap activation after 2 key count (double tap) for key B
// 1HoldL1TDKA = hold activation after 1 key count (single hold) activates layer 1 for key A
// 1HoldL2TDKB = hold activation after 1 key count (single hold) activates layer 2 for key B
// 2HoldL1TDKA = hold activation after 2 key count (double hold) activates layer 1 for key A
// 2HoldL2TDKB = hold activation after 2 key count (double hold) activates layer 2 for key B
// RelL1TDKA = release layer 1 for key A
// RelL2TDKB = release layer 2 for key B
// KPKA = key press for key A
// KRKA = key release for key B

TEST_F(TapDanceNestedTest, 1HoldL1_1TapL1_RelL1_KPKA_KRKA) {
    const platform_keycode_t TDKA = 2001;
    const platform_keycode_t TDKAOK = 2002;
    const platform_keycode_t TDKAIP = 2003;
    const platform_keycode_t KA = 2051;
    const platform_keycode_t TDKB = 2111;
    const platform_keycode_t TDKBOK = 2112;
    const platform_keycode_t TDKBIP = 2113;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TDKA, TDKAIP, KA, 2052 }
    }, {
        { 2151, 2152, TDKB, TDKBIP }
    }, {
        { 2053, 2054, 2055, 2056 }
    }};

    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    // Configure first tap dance key: tap -> OUTPUT_KEY_1, hold -> layer 1
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK}}, {{1, 1}}, 200, 200, TAP_DANCE_HOLD_PREFERRED);
    
    // Configure second tap dance key: tap -> OUTPUT_KEY_2, hold -> layer 2
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK}}, {{1, 2}}, 200, 200, TAP_DANCE_HOLD_PREFERRED);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(2051, 200);
    keyboard.release_key_at(2051, 400);
    keyboard.release_key_at(TDKA, 400);
    keyboard.press_key_at(KA, 400);
    keyboard.release_key_at(KA, 400);

    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(1, 400),
        td_layer(0, 400),
        td_press(KA, 400),
        td_release(KA, 400)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

