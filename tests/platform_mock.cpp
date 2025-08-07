#include "../src/platform_interface.h"
#include "../src/platform_layout.h"
#include "gtest/gtest.h"
#include "platform_types.h"
#include "platform_mock.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>


static void tap_dance_add_layer_event(uint8_t layer) {
    tap_dance_event_t event;
    event.type = tap_dance_event_type_t::LAYER_CHANGE;
    event.layer = layer;
    event.time = g_mock_state.timer; // Use current timer for the event
    g_mock_state.tap_dance_events.push_back(event);
}

static void tap_dance_add_key_event(platform_keycode_t keycode, bool pressed) {
    tap_dance_event_t event;
    event.type = pressed ? tap_dance_event_type_t::KEY_PRESS : tap_dance_event_type_t::KEY_RELEASE;
    event.keycode = keycode;
    event.time = g_mock_state.timer; // Use current timer for the event
    g_mock_state.tap_dance_events.push_back(event);
}

// Mock implementation of platform interface for testing

// MockPlatformState method implementations
MockPlatformState::MockPlatformState() : timer(0), next_token(1) {}

void MockPlatformState::set_timer(platform_time_t time) {
    for (auto it = g_mock_state.deferred_calls.begin(); it != g_mock_state.deferred_calls.end(); ++it) {
        if (it->execution_time <= time) {
            // Execute the deferred callback
            timer = it->execution_time; // Set timer to the execution time of the deferred call
            it->callback(it->data);
            // Remove the executed deferred call
            it = g_mock_state.deferred_calls.erase(it);
            if (it == g_mock_state.deferred_calls.end()) break; // Avoid invalid iterator
        }
    }
    timer = time;
}

void MockPlatformState::advance_timer(platform_time_t ms) {
    set_timer(timer + ms);
}

void MockPlatformState::reset() {
    timer = 0;
    next_token = 1;

    key_actions.clear();
    layer_history.clear();
    tap_dance_events.clear();
}

// New comparison methods with Google Test integration
::testing::AssertionResult MockPlatformState::key_actions_match(const std::vector<key_action_t>& expected) const {
    if (key_actions.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Size mismatch: actual=" << key_actions.size()
            << ", expected=" << expected.size();
    }

    for (size_t i = 0; i < expected.size(); i++) {
        if (!(key_actions[i] == expected[i])) {
            return ::testing::AssertionFailure()
                << "Mismatch at position " << i
                << " - actual: keycode=" << key_actions[i].keycode
                << " action=" << static_cast<int>(key_actions[i].action)
                << ", expected: keycode=" << expected[i].keycode
                << " action=" << static_cast<int>(expected[i].action);
        }
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult MockPlatformState::key_actions_match_with_time(const std::vector<key_action_t>& expected) const {
    if (key_actions.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Size mismatch: actual=" << key_actions.size()
            << ", expected=" << expected.size();
    }

    for (size_t i = 0; i < expected.size(); i++) {
        if (!key_actions[i].operator_with_time(expected[i])) {
            return ::testing::AssertionFailure()
                << "Mismatch at position " << i
                << " - actual: keycode=" << key_actions[i].keycode
                << " action=" << static_cast<int>(key_actions[i].action)
                << " time=" << key_actions[i].time
                << ", expected: keycode=" << expected[i].keycode
                << " action=" << static_cast<int>(expected[i].action)
                << " time=" << expected[i].time;
        }
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult MockPlatformState::key_actions_match_with_time_gaps(const std::vector<key_action_t>& expected, platform_time_t start_time) const {
    if (key_actions.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Size mismatch: actual=" << key_actions.size()
            << ", expected=" << expected.size();
    }

    if (expected.empty()) {
        return ::testing::AssertionSuccess();
    }

    platform_time_t expected_cumulative_time = 0;
    platform_time_t previous_actual_time = 0;
    std::stringstream debug_info;
    debug_info << "Time gap analysis (start time: " << start_time << "):\n";

    for (size_t i = 0; i < expected.size(); i++) {
        // Check keycode and action match
        if (key_actions[i].keycode != expected[i].keycode ||
            key_actions[i].action != expected[i].action) {
            return ::testing::AssertionFailure()
                << "Keycode/action mismatch at position " << i
                << " - actual: keycode=" << key_actions[i].keycode
                << " action=" << (key_actions[i].action == 0 ? "press" : "release")
                << ", expected: keycode=" << expected[i].keycode
                << " action=" << (expected[i].action == 0 ? "press" : "release");
        }

        // Add the time gap to get expected absolute time
        expected_cumulative_time += expected[i].time;
        platform_time_t expected_absolute_time = start_time + expected_cumulative_time;

        debug_info << "  Position " << i
                   << ": expected_gap=" << expected[i].time
                   << ", actual_gap=" << key_actions[i].time - previous_actual_time
                   << ", expected_absolute=" << expected_absolute_time
                   << ", actual_absolute=" << key_actions[i].time
                   << "\n";

        if (key_actions[i].time != expected_absolute_time) {
            return ::testing::AssertionFailure()
                << "\n" << "Time mismatch at position " << i
                << " - "
                << ": expected_gap=" << expected[i].time
                << ", actual_gap=" << key_actions[i].time - previous_actual_time
                << ", expected_absolute=" << expected_absolute_time
                << ", actual_absolute=" << key_actions[i].time
                << "\n"
                << debug_info.str();
        }

        previous_actual_time = key_actions[i].time;
    }

    return ::testing::AssertionSuccess();
}

// Helper function to compare event content (shared between relative and absolute time functions)
::testing::AssertionResult compare_event_content(const tap_dance_event_t& actual, const tap_dance_event_t& expected, size_t position) {
    if (!(actual == expected)) {
        std::string actual_desc, expected_desc;
        switch (actual.type) {
            case tap_dance_event_type_t::KEY_PRESS:
                actual_desc = "KEY_PRESS(" + std::to_string(actual.keycode) + ")";
                break;
            case tap_dance_event_type_t::KEY_RELEASE:
                actual_desc = "KEY_RELEASE(" + std::to_string(actual.keycode) + ")";
                break;
            case tap_dance_event_type_t::LAYER_CHANGE:
                actual_desc = "LAYER_CHANGE(" + std::to_string(actual.layer) + ")";
                break;
        }
        switch (expected.type) {
            case tap_dance_event_type_t::KEY_PRESS:
                expected_desc = "KEY_PRESS(" + std::to_string(expected.keycode) + ")";
                break;
            case tap_dance_event_type_t::KEY_RELEASE:
                expected_desc = "KEY_RELEASE(" + std::to_string(expected.keycode) + ")";
                break;
            case tap_dance_event_type_t::LAYER_CHANGE:
                expected_desc = "LAYER_CHANGE(" + std::to_string(expected.layer) + ")";
                break;
        }

        return ::testing::AssertionFailure()
            << "Event mismatch at position " << position
            << " - actual: " << actual_desc
            << ", expected: " << expected_desc;
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult MockPlatformState::tap_dance_event_actions_match_absolute(const std::vector<tap_dance_event_t>& expected) const {
    if (expected.empty()) {
        if (!key_actions.empty() || !layer_history.empty()) {
            return ::testing::AssertionFailure()
                << "Expected empty event sequence but found "
                << key_actions.size() << " key actions and "
                << layer_history.size() << " layer changes";
        }
        return ::testing::AssertionSuccess();
    }

    if (tap_dance_events.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Event count mismatch: actual=" << tap_dance_events.size()
            << ", expected=" << expected.size();
    }

    std::stringstream debug_info;
    debug_info << "Tap dance event analysis (absolute time):\n";

    for (size_t i = 0; i < expected.size(); i++) {
        const auto& actual_event = tap_dance_events[i];
        const auto& expected_event = expected[i];

        // Check event type and data match using shared helper
        auto content_result = compare_event_content(actual_event, expected_event, i);
        if (!content_result) {
            return content_result;
        }

        debug_info << "  Position " << i
                   << ": expected_time=" << expected_event.time
                   << ", actual_time=" << actual_event.time
                   << "\n";

        if (expected_event.time > 0 && actual_event.time != expected_event.time) {
            return ::testing::AssertionFailure()
                << "\n" << "Time mismatch at position " << i
                << " - expected_time=" << expected_event.time
                << ", actual_time=" << actual_event.time
                << "\n"
                << debug_info.str();
        }
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult MockPlatformState::tap_dance_event_actions_match_relative(const std::vector<tap_dance_event_t>& expected, platform_time_t start_time) const {
    if (expected.empty()) {
        if (!key_actions.empty() || !layer_history.empty()) {
            return ::testing::AssertionFailure()
                << "Expected empty event sequence but found "
                << key_actions.size() << " key actions and "
                << layer_history.size() << " layer changes";
        }
        return ::testing::AssertionSuccess();
    }

    if (tap_dance_events.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Event count mismatch: actual=" << tap_dance_events.size()
            << ", expected=" << expected.size();
    }

    platform_time_t expected_cumulative_time = 0;
    platform_time_t previous_actual_time = start_time;
    std::stringstream debug_info;
    debug_info << "Tap dance event analysis (start time: " << start_time << "):\n";

    for (size_t i = 0; i < expected.size(); i++) {
        const auto& actual_event = tap_dance_events[i];
        const auto& expected_event = expected[i];

        // Check event type and data match using shared helper
        auto content_result = compare_event_content(actual_event, expected_event, i);
        if (!content_result) {
            return content_result;
        }

        // Add the time gap to get expected absolute time
        expected_cumulative_time += expected_event.time;
        platform_time_t expected_absolute_time = start_time + expected_cumulative_time;

        debug_info << "  Position " << i
                   << ": expected_gap=" << expected_event.time
                   << ", actual_gap=" << actual_event.time - previous_actual_time
                   << ", expected_absolute=" << expected_absolute_time
                   << ", actual_absolute=" << actual_event.time
                   << "\n";

        if (expected_event.time > 0 && actual_event.time != expected_absolute_time) {
            return ::testing::AssertionFailure()
                << "\n" << "Time mismatch at position " << i
                << " - expected_gap=" << expected_event.time
                << ", actual_gap=" << actual_event.time - previous_actual_time
                << ", expected_absolute=" << expected_absolute_time
                << ", actual_absolute=" << actual_event.time
                << "\n"
                << debug_info.str();
        }

        previous_actual_time = actual_event.time;
    }

    return ::testing::AssertionSuccess();
}

bool MockPlatformState::layer_history_matches(const std::vector<uint8_t>& expected) const {
    return layer_history == expected;
}

std::vector<key_action_t> MockPlatformState::get_key_actions_since(size_t start_index) const {
    if (start_index >= key_actions.size()) {
        return {};
    }
    return std::vector<key_action_t>(key_actions.begin() + start_index, key_actions.end());
}

std::vector<uint8_t> MockPlatformState::get_layer_history_since(size_t start_index) const {
    if (start_index >= layer_history.size()) {
        return {};
    }
    return std::vector<uint8_t>(layer_history.begin() + start_index, layer_history.end());
}

// Global mock state
MockPlatformState g_mock_state;

// Mock key operations
void platform_tap_keycode(platform_keycode_t keycode) {
    printf("MOCK: Tap key %u (register + unregister)\n", keycode);
    // Tap is implemented as register followed by unregister
    platform_register_keycode(keycode);
    platform_unregister_keycode(keycode);
}

void platform_register_keycode(platform_keycode_t keycode) {
    printf("MOCK: Register key %u\n", keycode);
    g_mock_state.key_actions.push_back({keycode, 0, g_mock_state.timer}); // 0 = press, include timestamp
    tap_dance_add_key_event(keycode, true);
}

void platform_unregister_keycode(platform_keycode_t keycode) {
    printf("MOCK: Unregister key %u\n", keycode);
    g_mock_state.key_actions.push_back({keycode, 1, g_mock_state.timer}); // 1 = release, include timestamp
    tap_dance_add_key_event(keycode, false);
}

bool platform_compare_keyposition(platform_keypos_t key1, platform_keypos_t key2) {
    return (key1.row == key2.row && key1.col == key2.col);
}

// Mock layer operations

#if defined(FRAMEWORK_QMK)
void platform_layout_init_qmk_keymap(const uint16_t layers[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers) {
    platform_layout_init_qmk_keymap_impl(layers, num_layers) {
}
#elif defined(FRAMEWORK_ZMK)
void platform_layout_init_zmk_keymap(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
    platform_layout_init_zmk_keymap_impl(layers, num_layers, key_map, num_keys)
}
#endif
#if defined(AGNOSTIC_USE_1D_ARRAY)
void platform_layout_init_1d_keymap(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
    platform_layout_init_1d_keymap_impl(layers, num_layers, key_map, num_keys);
}
#elif defined(AGNOSTIC_USE_2D_ARRAY)
void platform_layout_init_2d_keymap(const uint16_t* layers, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    platform_layout_init_2d_keymap_impl(layers,  num_layers, rows, cols);
}
#endif

bool platform_layout_is_valid_layer(uint8_t layer) {
    return platform_layout_is_valid_layer_impl(layer);
}

void platform_layout_set_layer(uint8_t layer) {
    printf("MOCK: Layer select %u\n", layer);
    g_mock_state.layer_history.push_back(layer);
    tap_dance_add_layer_event(layer);

    platform_layout_set_layer_impl(layer);
}

uint8_t platform_layout_get_current_layer(void) {
    return platform_layout_get_current_layer_impl();
}

platform_keycode_t platform_layout_get_keycode(platform_keypos_t position) {
    return platform_layout_get_keycode_impl(position);
}

platform_keycode_t platform_layout_get_keycode_from_layer(uint8_t layer, platform_keypos_t position) {
    return platform_layout_get_keycode_from_layer_impl(layer, position);
}

// Mock deferred execution
platform_deferred_token platform_defer_exec(uint32_t delay_ms, void (*callback)(void*), void* data) {
    g_mock_state.next_token++;
    printf("MOCK: Defer exec token %u for %u ms\n", g_mock_state.next_token, delay_ms);
    g_mock_state.deferred_calls.push_back({g_mock_state.next_token, g_mock_state.timer + delay_ms, callback, data});
    return g_mock_state.next_token;
}

bool platform_cancel_deferred_exec(platform_deferred_token token) {
    printf("MOCK: Cancel deferred exec token %u\n", token);
    for (auto it = g_mock_state.deferred_calls.begin(); it != g_mock_state.deferred_calls.end(); ++it) {
        if (it->token == token) {
            g_mock_state.deferred_calls.erase(it);
            return true;
        }
    }
    return true;
}

// Mock memory operations
void* platform_malloc(size_t size) {
    return malloc(size);
}

void platform_free(void* ptr) {
    free(ptr);
}

// Test utilities
void mock_set_timer(platform_time_t time) {
    g_mock_state.set_timer(time);
}

platform_time_t mock_get_timer(void) {
    return g_mock_state.timer;
}

void mock_advance_timer(platform_time_t ms) {
    g_mock_state.advance_timer(ms);
}

void mock_reset_timer(void) {
    g_mock_state.timer = 0;
}

void reset_mock_state(void) {
    g_mock_state.reset();
}

// Helper functions for creating expected sequences
key_action_t press(platform_keycode_t keycode, platform_time_t time) {
    return {keycode, 0, time};
}

key_action_t release(platform_keycode_t keycode, platform_time_t time) {
    return {keycode, 1, time};
}

std::vector<key_action_t> tap_sequence(platform_keycode_t keycode) {
    return {press(keycode), release(keycode)};
}

// Helper functions for creating tap dance event sequences
tap_dance_event_t td_press(platform_keycode_t keycode, platform_time_t time) {
    tap_dance_event_t event;
    event.type = tap_dance_event_type_t::KEY_PRESS;
    event.keycode = keycode;
    event.time = time;
    return event;
}

tap_dance_event_t td_release(platform_keycode_t keycode, platform_time_t time) {
    tap_dance_event_t event;
    event.type = tap_dance_event_type_t::KEY_RELEASE;
    event.keycode = keycode;
    event.time = time;
    return event;
}

tap_dance_event_t td_layer(uint8_t layer, platform_time_t time) {
    tap_dance_event_t event;
    event.type = tap_dance_event_type_t::LAYER_CHANGE;
    event.layer = layer;
    event.time = time;
    return event;
}
