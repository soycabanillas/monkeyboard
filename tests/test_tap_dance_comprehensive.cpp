#include <gtest/gtest.h>
#include "tap_dance_test_framework.hpp"

class TapDanceComprehensiveTest : public TapDanceTestFramework {};

// ==================== BASIC TAP FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicSingleTap) {
    auto config = TapDanceTestConfig()
        .add_tap_key(TEST_KEY_TAP_DANCE_1, 1, OUT_KEY_X);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "press tap dance key"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release tap dance key"),
        TestEvent::time_passed(250, "wait for timeout"),
        TestEvent::expect_key_sent(OUT_KEY_X, "should output X")
    });
}

TEST_F(TapDanceComprehensiveTest, KeyRepetitionException) {
    auto config = TapDanceTestConfig()
        .add_tap_key(TEST_KEY_TAP_DANCE_1, 1, OUT_KEY_X)  // Changed from 0 to 1
        .add_hold_key(TEST_KEY_TAP_DANCE_1, 1, LAYER_SYMBOLS);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        // First tap
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_key_sent(OUT_KEY_X, "first tap should output immediately"),

        // Second tap (should work due to repetition exception)
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_key_sent(OUT_KEY_X, "second tap should also output immediately"),

        // Third tap
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_key_sent(OUT_KEY_X, "third tap should also work")
    });
}

TEST_F(TapDanceComprehensiveTest, NoActionConfigured) {
    auto config = TapDanceTestConfig(); // Empty configuration
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_A, "press regular key"),
        TestEvent::key_release(TEST_KEY_A, "release regular key"),
        TestEvent::time_passed(250, "wait"),
        TestEvent::expect_no_event("no tap dance actions should trigger")
    });
}

// ==================== BASIC HOLD FUNCTIONALITY ====================

TEST_F(TapDanceComprehensiveTest, BasicHoldTimeout) {
    auto config = TapDanceTestConfig()
        .add_hold_key(TEST_KEY_TAP_DANCE_1, 1, LAYER_SYMBOLS);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "press and hold"),
        TestEvent::time_passed(250, "wait for hold timeout"),
        TestEvent::expect_layer_select(LAYER_SYMBOLS, "should activate symbols layer"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release key"),
        TestEvent::expect_layer_select(LAYER_BASE, "should return to base layer")
    });
}

TEST_F(TapDanceComprehensiveTest, HoldReleasedBeforeTimeout) {
    auto config = TapDanceTestConfig()
        .add_tap_key(TEST_KEY_TAP_DANCE_1, 1, OUT_KEY_X)  // Changed from 0 to 1
        .add_hold_key(TEST_KEY_TAP_DANCE_1, 1, LAYER_SYMBOLS);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "press key"),
        TestEvent::time_passed(100, "wait less than hold timeout"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release before timeout"),
        TestEvent::time_passed(250, "wait for tap timeout"),
        TestEvent::expect_key_sent(OUT_KEY_X, "should execute tap action"),
        TestEvent::expect_layer_select(LAYER_BASE, "should stay on base layer")
    });
}

// ==================== MULTI-TAP SEQUENCES ====================

TEST_F(TapDanceComprehensiveTest, DoubleTap) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}, {2, OUT_KEY_Y}});  // Changed from {0,1} to {1,2}
    setup_tap_dance(config);

    execute_test_sequence({
        // First tap
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_no_event("should wait for potential second tap"),

        // Second tap
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(250, "wait for timeout"),
        TestEvent::expect_key_sent(OUT_KEY_Y, "should execute double-tap action")
    });
}

TEST_F(TapDanceComprehensiveTest, TripleTap) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}, {2, OUT_KEY_Y}, {3, OUT_KEY_Z}});  // Changed from {0,1,2} to {1,2,3}
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(250),
        TestEvent::expect_key_sent(OUT_KEY_Z, "should execute triple-tap action")
    });
}

TEST_F(TapDanceComprehensiveTest, TapCountExceedsConfiguration) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}, {2, OUT_KEY_Y}});  // Changed from {0,1} to {1,2}
    setup_tap_dance(config);

    execute_test_sequence({
        // Three taps (exceeds configuration)
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_key_sent(OUT_KEY_X, "should reset and execute first tap action")
    });
}

// ==================== INTERRUPT CONFIGURATION ====================

TEST_F(TapDanceComprehensiveTest, InterruptConfigMinus1) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}}, {{1, LAYER_SYMBOLS, -1}});  // Changed from {0} to {1}
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "start hold"),
        TestEvent::key_press(TEST_KEY_A, "interrupt with another key"),
        TestEvent::key_release(TEST_KEY_A, "release interrupting key"),
        TestEvent::expect_layer_select(LAYER_SYMBOLS, "should activate layer on press+release"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release tap dance key")
    });
}

TEST_F(TapDanceComprehensiveTest, InterruptConfigZero) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}}, {{1, LAYER_SYMBOLS, 0}});  // Changed from {0} to {1}
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "start hold"),
        TestEvent::key_press(TEST_KEY_A, "interrupt with another key"),
        TestEvent::expect_layer_select(LAYER_SYMBOLS, "should activate layer immediately on press"),
        TestEvent::key_release(TEST_KEY_A, "release interrupting key"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release tap dance key")
    });
}

TEST_F(TapDanceComprehensiveTest, InterruptConfigPositive) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}}, {{1, LAYER_SYMBOLS, 100}});  // Changed from {0} to {1}
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "start hold"),
        TestEvent::time_passed(50, "wait less than interrupt config time"),
        TestEvent::key_press(TEST_KEY_A, "interrupt early"),
        TestEvent::expect_key_sent(TEST_KEY_TAP_DANCE_1, "should send original key"),
        TestEvent::expect_key_sent(TEST_KEY_A, "should send interrupting key"),
        TestEvent::expect_no_event("hold action should be discarded")
    });
}

// ==================== NESTING BEHAVIOR ====================

TEST_F(TapDanceComprehensiveTest, DifferentKeycodesCanNest) {
    auto config = TapDanceTestConfig()
        .add_hold_key(TEST_KEY_TAP_DANCE_1, 1, LAYER_SYMBOLS)  // Changed from 0 to 1
        .add_tap_key(TEST_KEY_TAP_DANCE_2, 1, OUT_KEY_X);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "start first tap dance"),
        TestEvent::time_passed(250, "activate hold"),
        TestEvent::expect_layer_select(LAYER_SYMBOLS, "first layer activated"),

        TestEvent::key_press(TEST_KEY_TAP_DANCE_2, "start nested tap dance"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_2, "complete nested tap"),
        TestEvent::expect_key_sent(OUT_KEY_X, "nested tap should work"),

        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release first key"),
        TestEvent::expect_layer_select(LAYER_BASE, "should return to base layer")
    });
}

TEST_F(TapDanceComprehensiveTest, SameKeycodeNestingIgnored) {
    auto config = TapDanceTestConfig()
        .add_tap_key(TEST_KEY_TAP_DANCE_1, 1, OUT_KEY_X);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "first press"),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "second press - should be ignored"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "first release"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "second release - should be ignored"),
        TestEvent::time_passed(250, "wait for timeout"),
        TestEvent::expect_key_sent(OUT_KEY_X, "should only get one output")
    });
}

// ==================== LAYER STACK MANAGEMENT ====================

TEST_F(TapDanceComprehensiveTest, ComplexLayerStackDependencies) {
    auto config = TapDanceTestConfig()
        .add_hold_key(TEST_KEY_TAP_DANCE_1, 1, LAYER_SYMBOLS)  // Changed from 0 to 1
        .add_hold_key(TEST_KEY_TAP_DANCE_2, 1, LAYER_NUMBERS)  // Changed from 0 to 1
        .add_hold_key(TEST_KEY_TAP_DANCE_3, 1, LAYER_FUNCTION);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        // Build up layer stack
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1, "layer 1"),
        TestEvent::time_passed(250),
        TestEvent::expect_layer_select(LAYER_SYMBOLS),

        TestEvent::key_press(TEST_KEY_TAP_DANCE_2, "layer 2"),
        TestEvent::time_passed(250),
        TestEvent::expect_layer_select(LAYER_NUMBERS),

        TestEvent::key_press(TEST_KEY_TAP_DANCE_3, "layer 3"),
        TestEvent::time_passed(250),
        TestEvent::expect_layer_select(LAYER_FUNCTION),

        // Release in reverse order
        TestEvent::key_release(TEST_KEY_TAP_DANCE_3, "release layer 3"),
        TestEvent::expect_layer_select(LAYER_NUMBERS, "should return to layer 2"),

        TestEvent::key_release(TEST_KEY_TAP_DANCE_2, "release layer 2"),
        TestEvent::expect_layer_select(LAYER_SYMBOLS, "should return to layer 1"),

        TestEvent::key_release(TEST_KEY_TAP_DANCE_1, "release layer 1"),
        TestEvent::expect_layer_select(LAYER_BASE, "should return to base")
    });
}

// ==================== TIMING AND STATE MANAGEMENT ====================

TEST_F(TapDanceComprehensiveTest, FastKeySequences) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}, {2, OUT_KEY_Y}});  // Changed from {0,1} to {1,2}
    setup_tap_dance(config);

    execute_test_sequence({
        // Very fast double tap
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(10, "very short delay"),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(250),
        TestEvent::expect_key_sent(OUT_KEY_Y, "should still register as double tap")
    });
}

TEST_F(TapDanceComprehensiveTest, MixedTapHoldSequence) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{1, OUT_KEY_X}, {2, OUT_KEY_Y}}, {{2, LAYER_SYMBOLS, 0}});  // Changed from {0,1},{1} to {1,2},{2}
    setup_tap_dance(config);

    execute_test_sequence({
        // First tap
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),

        // Second tap but hold
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(250, "hold second tap"),
        TestEvent::expect_layer_select(LAYER_SYMBOLS, "should activate layer"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_layer_select(LAYER_BASE)
    });
}

// ==================== EDGE CASES ====================

TEST_F(TapDanceComprehensiveTest, VeryFastTapRelease) {
    auto config = TapDanceTestConfig()
        .add_tap_key(TEST_KEY_TAP_DANCE_1, 1, OUT_KEY_X);  // Changed from 0 to 1
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(1, "1ms hold"),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::time_passed(250),
        TestEvent::expect_key_sent(OUT_KEY_X, "should work even with very fast tap")
    });
}

TEST_F(TapDanceComprehensiveTest, ImmediateExecutionOnFinalTapCount) {
    auto config = TapDanceTestConfig()
        .add_tap_dance(TEST_KEY_TAP_DANCE_1, {{2, OUT_KEY_Y}});  // Changed from {1} to {2} - only double-tap configured
    setup_tap_dance(config);

    execute_test_sequence({
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_press(TEST_KEY_TAP_DANCE_1),
        TestEvent::key_release(TEST_KEY_TAP_DANCE_1),
        TestEvent::expect_key_sent(OUT_KEY_Y, "should execute immediately without timeout")
    });
}
