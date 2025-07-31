#include "../src/platform_interface.h"
#include "../src/platform_layout.h"
#include "gtest/gtest.h"
#include "platform_types.h"
#include "platform_mock.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <sstream>

// Mock implementation of platform interface for testing

// MockPlatformState method implementations
MockPlatformState::MockPlatformState() : timer(0), current_layer(0), next_token(1),
                     last_key_event_time(0), last_sent_key(0),
                     last_registered_key(0), last_unregistered_key(0),
                     last_selected_layer(0) {}

void MockPlatformState::record_key_event(platform_keycode_t keycode, bool pressed) {
    key_events.push_back({keycode, pressed, timer});
    last_key_event_time = timer;
    if (pressed) {
        pressed_keys.insert(keycode);
    } else {
        pressed_keys.erase(keycode);
    }
}

void MockPlatformState::advance_timer(platform_time_t ms) {
    timer += ms;
    for (auto it = g_mock_state.deferred_calls.begin(); it != g_mock_state.deferred_calls.end(); ++it) {
        if (it->execution_time <= timer) {
            // Execute the deferred callback
            it->callback(it->data);
            // Remove the executed deferred call
            it = g_mock_state.deferred_calls.erase(it);
            if (it == g_mock_state.deferred_calls.end()) break; // Avoid invalid iterator
        }
    }
}

void MockPlatformState::reset() {
    timer = 0;
    current_layer = 0;
    next_token = 1;
    last_key_event_time = 0;

    key_events.clear();
    pressed_keys.clear();
    register_key_calls.clear();
    unregister_key_calls.clear();
    layer_select_calls.clear();
    key_actions.clear();
    layer_history.clear();

    last_sent_key = 0;
    last_registered_key = 0;
    last_unregistered_key = 0;
    last_selected_layer = 0;
}

void MockPlatformState::print_state() const {
    printf("\n");
    printf("=== MOCK STATE ===\n");
    printf("Current time: %u ms\n", timer);
    printf("Current layer: %u\n", current_layer);
    printf("Time since last key event: %u ms\n", timer - last_key_event_time);

    printf("Currently pressed keys (%zu): ", pressed_keys.size());
    for (auto keycode : pressed_keys) {
        printf("%u ", keycode);
    }
    printf("\n");

    printf("Key event history (%zu events):\n", key_events.size());
    for (size_t i = 0; i < key_events.size(); i++) {
        const auto& event = key_events[i];
        if (i == 0) {
            printf("  %u ms: Key %u %s (first event)\n",
                   event.timestamp, event.keycode,
                   event.pressed ? "PRESS" : "RELEASE");
        } else {
            platform_time_t time_diff = event.timestamp - key_events[i-1].timestamp;
            printf("  %u ms: Key %u %s (+%u ms)\n",
                   event.timestamp, event.keycode,
                   event.pressed ? "PRESS" : "RELEASE", time_diff);
        }
    }
    printf("==================\n");
    printf("\n");
}

int MockPlatformState::register_key_calls_count() const { return register_key_calls.size(); }
int MockPlatformState::unregister_key_calls_count() const { return unregister_key_calls.size(); }
int MockPlatformState::layer_select_calls_count() const { return layer_select_calls.size(); }
int MockPlatformState::key_events_count() const { return key_events.size(); }
size_t MockPlatformState::pressed_keys_count() const { return pressed_keys.size(); }

bool MockPlatformState::is_key_pressed(platform_keycode_t keycode) const {
    return pressed_keys.find(keycode) != pressed_keys.end();
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

::testing::AssertionResult MockPlatformState::key_actions_match_with_time_gaps(const std::vector<key_action_t>& expected, platform_time_t actual_start_time) const {
    if (key_actions.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Size mismatch: actual=" << key_actions.size()
            << ", expected=" << expected.size();
    }

    if (expected.empty()) {
        return ::testing::AssertionSuccess();
    }

    platform_time_t expected_cumulative_time = 0;
    std::stringstream debug_info;
    debug_info << "Time gap analysis (start time: " << actual_start_time << "):\n";

    for (size_t i = 0; i < expected.size(); i++) {
        // Check keycode and action match
        if (key_actions[i].keycode != expected[i].keycode ||
            key_actions[i].action != expected[i].action) {
            return ::testing::AssertionFailure()
                << "Keycode/action mismatch at position " << i
                << " - actual: keycode=" << key_actions[i].keycode
                << " action=" << static_cast<int>(key_actions[i].action)
                << ", expected: keycode=" << expected[i].keycode
                << " action=" << static_cast<int>(expected[i].action);
        }

        // Add the time gap to get expected absolute time
        expected_cumulative_time += expected[i].time;
        platform_time_t expected_absolute_time = actual_start_time + expected_cumulative_time;

        debug_info << "  Position " << i << ": gap=" << expected[i].time
                   << ", cumulative=" << expected_cumulative_time
                   << ", expected_absolute=" << expected_absolute_time
                   << ", actual=" << key_actions[i].time << "\n";

        if (key_actions[i].time != expected_absolute_time) {
            return ::testing::AssertionFailure()
                << "Time mismatch at position " << i
                << " - actual=" << key_actions[i].time
                << ", expected_absolute=" << expected_absolute_time
                << " (gap=" << expected[i].time
                << ", cumulative=" << expected_cumulative_time << ")\n"
                << debug_info.str();
        }
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
    g_mock_state.register_key_calls.push_back(keycode);
    g_mock_state.unregister_key_calls.push_back(keycode);
    g_mock_state.last_registered_key = keycode;
    g_mock_state.last_unregistered_key = keycode;
    // Record as press then release
    g_mock_state.record_key_event(keycode, true);
    g_mock_state.record_key_event(keycode, false);
}

void platform_register_keycode(platform_keycode_t keycode) {
    printf("MOCK: Register key %u\n", keycode);
    g_mock_state.register_key_calls.push_back(keycode);
    g_mock_state.last_registered_key = keycode;
    g_mock_state.record_key_event(keycode, true);
    g_mock_state.key_actions.push_back({keycode, 0, g_mock_state.timer}); // 0 = press, include timestamp
}

void platform_unregister_keycode(platform_keycode_t keycode) {
    printf("MOCK: Unregister key %u\n", keycode);
    g_mock_state.unregister_key_calls.push_back(keycode);
    g_mock_state.last_unregistered_key = keycode;
    g_mock_state.record_key_event(keycode, false);
    g_mock_state.key_actions.push_back({keycode, 1, g_mock_state.timer}); // 1 = release, include timestamp
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

void platform_layout_set_layer(uint8_t layer) {
    printf("MOCK: Layer select %u\n", layer);
    g_mock_state.layer_select_calls.push_back(layer);
    g_mock_state.current_layer = layer;
    g_mock_state.last_selected_layer = layer;
    g_mock_state.layer_history.push_back(layer);

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

// Mock time operations
void platform_wait_ms(platform_time_t ms) {
    g_mock_state.advance_timer(ms);
}

platform_time_t platform_timer_read(void) {
    return g_mock_state.timer;
}

platform_time_t platform_timer_elapsed(platform_time_t last) {
    return g_mock_state.timer - last;
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
void mock_advance_timer(platform_time_t ms) {
    g_mock_state.advance_timer(ms);
}

void mock_reset_timer(void) {
    g_mock_state.timer = 0;
}

void mock_set_layer(uint8_t layer) {
    g_mock_state.current_layer = layer;
}

void mock_print_state(void) {
    g_mock_state.print_state();
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
