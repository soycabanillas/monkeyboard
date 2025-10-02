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
// 1HoldL1TDKB = hold activation after 1 key count (single hold) activates layer 1 for key B
// 2HoldL1TDKA = hold activation after 2 key count (double hold) activates layer 1 for key A
// 2HoldL1TDKB = hold activation after 2 key count (double hold) activates layer 1 for key B
// RelL1TDKA = release layer 1 for key A
// RelL1TDKB = release layer 1 for key B
// KPKA = key press for key A
// KRKA = key release for key B

const platform_keycode_t TDKA = 2051;
const platform_keycode_t TDKAIP = 2052;
const platform_keycode_t KA = 2053;
const platform_keycode_t TDKAOK1 = 20511;
const platform_keycode_t TDKAOK2 = 20512;
const platform_keycode_t TDKAOK3 = 20513;

const platform_keycode_t TDKB = 2154;
const platform_keycode_t TDKBIP = 2155;
const platform_keycode_t KB = 2156;
const platform_keycode_t TDKBOK1 = 21541;
const platform_keycode_t TDKBOK2 = 21542;
const platform_keycode_t TDKBOK3 = 21543;

const platform_keycode_t TDKC = 2257;
const platform_keycode_t TDKCIP = 2258;
const platform_keycode_t KC = 2259;
const platform_keycode_t TDKCOK1 = 22571;
const platform_keycode_t TDKCOK2 = 22572;
const platform_keycode_t TDKCOK3 = 22573;

const platform_keycode_t TDKD = 2360;
const platform_keycode_t TDKDIP = 2361;
const platform_keycode_t KD = 2362;
const platform_keycode_t TDKDOK1 = 23601;
const platform_keycode_t TDKDOK2 = 23602;
const platform_keycode_t TDKDOK3 = 23603;

std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
    { TDKA, TDKAIP, KA,   2054, 2055,   2056, 2057, 2058,   2059 }
}, {
    { 2151, 2152,   2153, TDKB, TDKBIP, KB,   2157, 2158,   2159 }
}, {
    { 2251, 2252,   2253, 2254, 2255,   2256, TDKC, TDKCIP, KC }
}};

TEST_F(TapDanceNestedTest, ExistingTest_1HoldL1_1TapL1_RelL1_KPKA_KRKA) {
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
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKB, 400);
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

// DuplicateKeys DuplicatePhysicalKeyPresses

// Helper functions for DuplicateKeys DuplicatePhysicalKeyPresses

void test_DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKA, 50);
    keyboard.release_key_at(TDKA, 100);
    keyboard.release_key_at(TDKA, 150);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(TDKAOK1, 100),
        td_release(TDKAOK1, 100)
    };
    test_DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_press(TDKAOK1, 100),
        td_release(TDKAOK1, 100)
    };
    test_DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_press(TDKAOK1, 100),
        td_release(TDKAOK1, 100)
    };
    test_DuplicateKeys_DuplicatePhysicalKeyPresses_Taps_TDKA_TDKA_RelTDKA_RelTDKA(TAP_DANCE_BALANCED, expected_events);
}

void test_DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKA, 50);
    keyboard.release_key_at(TDKA, 250);
    keyboard.release_key_at(TDKA, 300);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 250)
    };
    test_DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 250)
    };
    test_DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 250)
    };
    test_DuplicateKeys_DuplicatePhysicalKeyPresses_Holds_TDKA_TDKA_RelTDKA_RelTDKA(TAP_DANCE_BALANCED, expected_events);
}

// Single Layer Tests (1 layer deep)

// Helper functions for Single Layer Tests

void test_SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(KB, 200);
    keyboard.release_key_at(KB, 250);
    keyboard.release_key_at(TDKA, 300);
    keyboard.press_key_at(KA, 350);
    keyboard.release_key_at(KA, 400);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(KB, 200),
        td_release(KB, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(KB, 200),
        td_release(KB, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(KB, 200),
        td_release(KB, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_1TapL1_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKB, 250);
    keyboard.release_key_at(TDKA, 300);
    keyboard.press_key_at(KA, 350);
    keyboard.release_key_at(KA, 400);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK1, 250),
        td_release(TDKBOK1, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK1, 250),
        td_release(TDKBOK1, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK1, 250),
        td_release(TDKBOK1, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_1TapTDKB_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.release_key_at(TDKA, 300);
    keyboard.press_key_at(KA, 350);
    keyboard.release_key_at(KA, 400);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_SingleLayer_1HoldL1_NoAction_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

// Double Layer Tests (2 layers deep)

// Helper functions for Double Layer Tests

void test_DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(KC, 400);
    keyboard.release_key_at(KC, 450);
    keyboard.release_key_at(TDKB, 500);
    keyboard.release_key_at(TDKA, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 400),
        td_release(KC, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 400),
        td_release(KC, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 400),
        td_release(KC, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_1TapL2_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}}, {{1, 3}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(TDKC, 400);
    keyboard.release_key_at(TDKC, 450);
    keyboard.release_key_at(TDKB, 500);
    keyboard.release_key_at(TDKA, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_1TapTDKB_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKB, 500);
    keyboard.release_key_at(TDKA, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_DoubleLayer_1HoldL1_1HoldL2_NoAction_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

// Helper functions for Reverse Release Order Tests

void test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKA, 400);
    keyboard.press_key_at(KC, 450);
    keyboard.release_key_at(KC, 500);
    keyboard.release_key_at(TDKB, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 450),
        td_release(KC, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 450),
        td_release(KC, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 450),
        td_release(KC, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapL2_RelL2_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}}, {}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKA, 400);
    keyboard.press_key_at(TDKC, 450);
    keyboard.release_key_at(TDKC, 500);
    keyboard.release_key_at(TDKB, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_RelL1_1TapTDKC_RelL2_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(KC, 400);
    keyboard.release_key_at(KC, 450);
    keyboard.release_key_at(TDKA, 500);
    keyboard.release_key_at(TDKB, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 400),
        td_release(KC, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 400),
        td_release(KC, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(KC, 400),
        td_release(KC, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_1TapL2_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}}, {{1, 3}}, 200, 200, strategy);
    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(TDKC, 400);
    keyboard.release_key_at(TDKC, 450);
    keyboard.release_key_at(TDKA, 500);
    keyboard.release_key_at(TDKB, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK1, 450),
        td_release(TDKCOK1, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_1TapTDKB_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKA, 500);
    keyboard.release_key_at(TDKB, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_ReverseRelease_1HoldL1_1HoldL2_NoAction_RelL1_RelL2_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

// Triple Layer Tests (3 layers deep)

// Helper functions for Triple Layer Tests

void test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}}, {{1, 3}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(TDKC, 400);
    keyboard.press_key_at(KD, 600);
    keyboard.release_key_at(KD, 650);
    keyboard.release_key_at(TDKC, 700);
    keyboard.release_key_at(TDKB, 750);
    keyboard.release_key_at(TDKA, 800);
    keyboard.press_key_at(KA, 850);
    keyboard.release_key_at(KA, 900);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_press(KD, 600),
        td_release(KD, 650),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_press(KD, 600),
        td_release(KD, 650),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_press(KD, 600),
        td_release(KD, 650),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapL3_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}}, {{1, 3}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKD, {{1, TDKDOK1}}, {{1, 4}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(TDKC, 400);
    keyboard.press_key_at(TDKD, 600);
    keyboard.release_key_at(TDKD, 650);
    keyboard.release_key_at(TDKC, 700);
    keyboard.release_key_at(TDKB, 750);
    keyboard.release_key_at(TDKA, 800);
    keyboard.press_key_at(KA, 850);
    keyboard.release_key_at(KA, 900);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_press(TDKD, 650),
        td_release(TDKD, 650),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_press(TDKD, 650),
        td_release(TDKD, 650),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_press(TDKD, 650),
        td_release(TDKD, 650),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_1TapTDKB_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}}, {{1, 3}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(TDKC, 400);
    keyboard.release_key_at(TDKC, 700);
    keyboard.release_key_at(TDKB, 750);
    keyboard.release_key_at(TDKA, 800);
    keyboard.press_key_at(KA, 850);
    keyboard.release_key_at(KA, 900);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_layer(3, 600),
        td_layer(2, 700),
        td_layer(1, 750),
        td_layer(0, 800),
        td_press(KA, 850),
        td_release(KA, 900)
    };
    test_TripleLayer_1HoldL1_1HoldL2_1HoldL3_NoAction_RelL3_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

// Helper functions for Same Layer Activation Tests

void test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 1}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(KB, 400);
    keyboard.release_key_at(KB, 450);
    keyboard.release_key_at(TDKA, 500);
    keyboard.release_key_at(TDKB, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(1, 400),
        td_press(KB, 400),
        td_release(KB, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(1, 400),
        td_press(KB, 400),
        td_release(KB, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(1, 400),
        td_press(KB, 400),
        td_release(KB, 450),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKA_RelL1TDKB_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 1}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(KB, 400);
    keyboard.release_key_at(KB, 450);
    keyboard.release_key_at(TDKB, 500);
    keyboard.release_key_at(TDKA, 550);
    keyboard.press_key_at(KA, 600);
    keyboard.release_key_at(KA, 650);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(1, 400),
        td_press(KB, 400),
        td_release(KB, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(1, 400),
        td_press(KB, 400),
        td_release(KB, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(1, 400),
        td_press(KB, 400),
        td_release(KB, 450),
        td_layer(1, 500),
        td_layer(0, 550),
        td_press(KA, 600),
        td_release(KA, 650)
    };
    test_SameLayer_1HoldL1TDKA_1HoldL1TDKB_1TapL1_RelL1TDKB_RelL1TDKA_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}


// Multi-Tap While Holding Tests

// Helper functions for Multi-Tap While Holding Tests

void test_MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}, {2, TDKBOK2}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKB, 250);
    keyboard.press_key_at(TDKB, 300);
    keyboard.release_key_at(TDKB, 350);
    keyboard.release_key_at(TDKA, 400);
    keyboard.press_key_at(KA, 450);
    keyboard.release_key_at(KA, 500);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK2, 300),
        td_release(TDKBOK2, 350),
        td_layer(0, 400),
        td_press(KA, 450),
        td_release(KA, 500)
    };
    test_MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK2, 300),
        td_release(TDKBOK2, 350),
        td_layer(0, 400),
        td_press(KA, 450),
        td_release(KA, 500)
    };
    test_MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK2, 300),
        td_release(TDKBOK2, 350),
        td_layer(0, 400),
        td_press(KA, 450),
        td_release(KA, 500)
    };
    test_MultiTapHolding_1HoldL1_2TapL1_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}, {3, TDKBOK3}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKB, 250);
    keyboard.press_key_at(TDKB, 300);
    keyboard.release_key_at(TDKB, 350);
    keyboard.press_key_at(TDKB, 400);
    keyboard.release_key_at(TDKB, 450);
    keyboard.release_key_at(TDKA, 500);
    keyboard.press_key_at(KA, 550);
    keyboard.release_key_at(KA, 600);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK3, 400),
        td_release(TDKBOK3, 450),
        td_layer(0, 500),
        td_press(KA, 550),
        td_release(KA, 600)
    };
    test_MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK3, 400),
        td_release(TDKBOK3, 450),
        td_layer(0, 500),
        td_press(KA, 550),
        td_release(KA, 600)
    };
    test_MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK3, 400),
        td_release(TDKBOK3, 450),
        td_layer(0, 500),
        td_press(KA, 550),
        td_release(KA, 600)
    };
    test_MultiTapHolding_1HoldL1_3TapL1_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

void test_MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}}, {{1, 2}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKC, {{1, TDKCOK1}, {2, TDKCOK2}}, {{1, 3}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.press_key_at(TDKC, 400);
    keyboard.release_key_at(TDKC, 450);
    keyboard.press_key_at(TDKC, 500);
    keyboard.release_key_at(TDKC, 550);
    keyboard.release_key_at(TDKB, 600);
    keyboard.release_key_at(TDKA, 650);
    keyboard.press_key_at(KA, 700);
    keyboard.release_key_at(KA, 750);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK2, 500),
        td_release(TDKCOK2, 550),
        td_layer(1, 600),
        td_layer(0, 650),
        td_press(KA, 700),
        td_release(KA, 750)
    };
    test_MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK2, 500),
        td_release(TDKCOK2, 550),
        td_layer(1, 600),
        td_layer(0, 650),
        td_press(KA, 700),
        td_release(KA, 750)
    };
    test_MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_layer(2, 400),
        td_press(TDKCOK2, 500),
        td_release(TDKCOK2, 550),
        td_layer(1, 600),
        td_layer(0, 650),
        td_press(KA, 700),
        td_release(KA, 750)
    };
    test_MultiTapHolding_1HoldL1_1HoldL2_2TapL2_RelL2_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}

// This test checks that tapping a tap dance key with several tap actions while holding another tap dance key works as expected

void test_MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA(tap_dance_hold_strategy_t strategy, const std::vector<event_t>& expected_events) {
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    
    config_builder.add_tap_hold(TDKA, {{1, TDKAOK1}}, {{1, 1}}, 200, 200, strategy);
    config_builder.add_tap_hold(TDKB, {{1, TDKBOK1}, {2, TDKBOK2}}, {{1, 2}}, 200, 200, strategy);

    config_builder.add_to_scenario(scenario);
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(TDKA, 0);
    keyboard.press_key_at(TDKB, 200);
    keyboard.release_key_at(TDKB, 250);
    keyboard.release_key_at(TDKA, 300);
    keyboard.press_key_at(KA, 350);
    keyboard.release_key_at(KA, 400);

    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA_TAP_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK1, 200),
        td_release(TDKBOK1, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA(TAP_DANCE_TAP_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA_HOLD_PREFERRED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK1, 200),
        td_release(TDKBOK1, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA(TAP_DANCE_HOLD_PREFERRED, expected_events);
}

TEST_F(TapDanceNestedTest, MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA_BALANCED) {
    std::vector<event_t> expected_events = {
        td_layer(1, 200),
        td_press(TDKBOK1, 200),
        td_release(TDKBOK1, 250),
        td_layer(0, 300),
        td_press(KA, 350),
        td_release(KA, 400)
    };
    test_MultiTapHolding_1HoldL1_1TapL1_RelL1_KPKA_KRKA(TAP_DANCE_BALANCED, expected_events);
}