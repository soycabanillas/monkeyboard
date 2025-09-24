#include "../src/platform_interface.h"
#include "../src/platform_layout.h"
#include "gtest/gtest.h"
#include "monkeyboard_deferred_callbacks.h"
#include "platform_types.h"
#include "platform_mock.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>


static void add_layer_event(uint8_t layer) {
    event_t event;
    event.type = event_type_t::LAYER_CHANGE;
    event.layer = layer;
    event.time = g_mock_state.timer; // Use current timer for the event
    g_mock_state.events.push_back(event);
}

static void add_key_event(platform_keycode_t keycode, bool pressed) {
    event_t event;
    event.type = pressed ? event_type_t::KEY_PRESS : event_type_t::KEY_RELEASE;
    event.keycode = keycode;
    event.time = g_mock_state.timer; // Use current timer for the event
    g_mock_state.events.push_back(event);
}

static void add_report_event(platform_keycode_t keycode, bool pressed) {
    event_t event;
    event.type = pressed ? event_type_t::REPORT_PRESS : event_type_t::REPORT_RELEASE;
    event.keycode = keycode;
    event.time = g_mock_state.timer; // Use current timer for the event
    g_mock_state.events.push_back(event);
}

static void add_report_send(void) {
    event_t event;
    event.type = event_type_t::REPORT_SEND;
    event.time = g_mock_state.timer; // Use current timer for the event
    g_mock_state.events.push_back(event);
}

// Mock implementation of platform interface for testing

// MockPlatformState method implementations
MockPlatformState::MockPlatformState() : timer(0) {}

void MockPlatformState::set_timer(platform_time_t time) {
    execute_deferred_executions();
    deferred_callback_entry_t* entry = get_next_deferred_callback(time);
    while (entry != nullptr && entry->execute_time <= time) {
        // Execute the deferred callback
        timer = entry->execute_time; // Set timer to the execution time of the deferred call
        execute_callback(entry);
        entry = get_next_deferred_callback(time);
    }
    timer = time;
}

void MockPlatformState::advance_timer(platform_time_t ms) {
    set_timer(timer + ms);
}

void MockPlatformState::reset() {
    timer = 0;

    events.clear();
}

// New comparison methods with Google Test integration

// Helper function to format an event into a compact string
static std::string format_event_compact(const event_t& event) {
    switch (event.type) {
        case event_type_t::KEY_PRESS:
            return "KEY_PRESS(" + std::to_string(event.keycode) + ")";
        case event_type_t::KEY_RELEASE:
            return "KEY_RELEASE(" + std::to_string(event.keycode) + ")";
        case event_type_t::LAYER_CHANGE:
            return "LAYER_CHANGE(" + std::to_string(event.layer) + ")";
        case event_type_t::REPORT_PRESS:
            return "REPORT_PRESS(" + std::to_string(event.keycode) + ")";
        case event_type_t::REPORT_RELEASE:
            return "REPORT_RELEASE(" + std::to_string(event.keycode) + ")";
        case event_type_t::REPORT_SEND:
            return "REPORT_SEND";
    }
    return "UNKNOWN";
}

// Helper function to compare event content (shared between relative and absolute time functions)
::testing::AssertionResult compare_event_content(const event_t& actual, const event_t& expected, size_t position) {
    if (!(actual == expected)) {
        return ::testing::AssertionFailure()
            << "Event mismatch at position " << position
            << " - actual: " << format_event_compact(actual)
            << ", expected: " << format_event_compact(expected);
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult MockPlatformState::event_actions_match_absolute(const std::vector<event_t>& expected) const {
    // Generate detailed comparison table
    std::stringstream debug_table;
    debug_table << "\nTap dance event comparison (absolute time):\n";
    debug_table << "Pos | Error | Expected Event        | Exp Time | Actual Event          | Act Time\n";
    debug_table << "----|-------|-----------------------|----------|-----------------------|----------\n";
    
    size_t max_size = std::max(events.size(), expected.size());
    bool has_mismatch = false;
    size_t first_mismatch_pos = 0;
    
    for (size_t i = 0; i < max_size; i++) {
        bool has_expected = i < expected.size();
        bool has_actual = i < events.size();
        
        std::string expected_event_str = has_expected ? format_event_compact(expected[i]) : "MISSING";
        std::string actual_event_str = has_actual ? format_event_compact(events[i]) : "MISSING";
        
        platform_time_t expected_time = has_expected ? expected[i].time : 0;
        platform_time_t actual_time = has_actual ? events[i].time : 0;
        
        bool content_match = has_expected && has_actual && (events[i] == expected[i]);
        bool time_match = expected_time == actual_time;
        bool row_match = content_match && time_match && has_expected && has_actual;
        
        if (!has_mismatch && !row_match) {
            has_mismatch = true;
            first_mismatch_pos = i;
        }
        
        std::string error_marker = "";
        if (!row_match && i == first_mismatch_pos) {
            error_marker = "  -> ";
        } else {
            error_marker = "     ";
        }
        
        debug_table << std::setw(3) << i << " | "
                   << error_marker << " | "
                   << std::setw(21) << std::left << expected_event_str << " | "
                   << std::setw(8) << std::right << (has_expected ? std::to_string(expected_time) : "-") << " | "
                   << std::setw(21) << std::left << actual_event_str << " | "
                   << std::setw(9) << std::right << (has_actual ? std::to_string(actual_time) : "-") << "\n";
    }

    if (events.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Event count mismatch: actual=" << events.size()
            << ", expected=" << expected.size()
            << debug_table.str();
    }

    if (has_mismatch) {
        return ::testing::AssertionFailure()
            << "First mismatch at position " << first_mismatch_pos
            << debug_table.str();
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult MockPlatformState::event_actions_match_relative(const std::vector<event_t>& expected, platform_time_t start_time) const {
    // Generate detailed comparison table
    std::stringstream debug_table;
    debug_table << "\nTap dance event comparison (relative time, start: " << start_time << "):\n";
    debug_table << "Pos | Error | Expected Event        | Exp Gap  | Exp Abs  | Actual Event          | Act Gap  | Act Abs\n";
    debug_table << "----|-------|-----------------------|----------|----------|-----------------------|----------|----------\n";
    
    size_t max_size = std::max(events.size(), expected.size());
    bool has_mismatch = false;
    size_t first_mismatch_pos = 0;
    
    platform_time_t expected_cumulative_time = 0;
    platform_time_t previous_actual_time = start_time;
    
    for (size_t i = 0; i < max_size; i++) {
        bool has_expected = i < expected.size();
        bool has_actual = i < events.size();
        
        std::string expected_event_str = has_expected ? format_event_compact(expected[i]) : "MISSING";
        std::string actual_event_str = has_actual ? format_event_compact(events[i]) : "MISSING";
        
        platform_time_t expected_gap = has_expected ? expected[i].time : 0;
        platform_time_t actual_gap = has_actual ? events[i].time - previous_actual_time : 0;
        
        if (has_expected) {
            expected_cumulative_time += expected_gap;
        }
        platform_time_t expected_absolute = start_time + expected_cumulative_time;
        platform_time_t actual_absolute = has_actual ? events[i].time : 0;
        
        bool content_match = has_expected && has_actual && (events[i] == expected[i]);
        bool time_match = expected_absolute == actual_absolute;
        bool row_match = content_match && time_match && has_expected && has_actual;
        
        if (!has_mismatch && !row_match) {
            has_mismatch = true;
            first_mismatch_pos = i;
        }
        
        std::string error_marker = "";
        if (!row_match && i == first_mismatch_pos) {
            error_marker = "  -> ";
        } else {
            error_marker = "     ";
        }
        
        debug_table << std::setw(3) << i << " | "
                   << error_marker << " | "
                   << std::setw(21) << std::left << expected_event_str << " | "
                   << std::setw(8) << std::right << (has_expected ? std::to_string(expected_gap) : "-") << " | "
                   << std::setw(8) << std::right << (has_expected ? std::to_string(expected_absolute) : "-") << " | "
                   << std::setw(21) << std::left << actual_event_str << " | "
                   << std::setw(8) << std::right << (has_actual ? std::to_string(actual_gap) : "-") << " | "
                   << std::setw(9) << std::right << (has_actual ? std::to_string(actual_absolute) : "-") << "\n";
        
        if (has_actual) {
            previous_actual_time = events[i].time;
        }
    }

    if (events.size() != expected.size()) {
        return ::testing::AssertionFailure()
            << "Event count mismatch: actual=" << events.size()
            << ", expected=" << expected.size()
            << debug_table.str();
    }

    if (has_mismatch) {
        return ::testing::AssertionFailure()
            << "First mismatch at position " << first_mismatch_pos
            << debug_table.str();
    }

    return ::testing::AssertionSuccess();
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
    add_key_event(keycode, true);
}

void platform_unregister_keycode(platform_keycode_t keycode) {
    printf("MOCK: Unregister key %u\n", keycode);
    add_key_event(keycode, false);
}

void platform_add_key(platform_keycode_t keycode) {
    printf("MOCK: Add key %u\n", keycode);
    add_report_event(keycode, true);
}

void platform_del_key(platform_keycode_t keycode) {
    printf("MOCK: Del key %u\n", keycode);
    add_report_event(keycode, false);
}

void platform_send_report(void) {
    printf("MOCK: Send report\n");
    add_report_send();
}

bool platform_compare_keyposition(platform_keypos_t key1, platform_keypos_t key2) {
    return (key1.row == key2.row && key1.col == key2.col);
}

// Mock layer operations

#if defined(FRAMEWORK_QMK)
void platform_layout_init_qmk_keymap(const uint16_t layers[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers) {
    platform_layout_init_qmk_keymap_impl(layers, num_layers);
}
#elif defined(FRAMEWORK_ZMK)
void platform_layout_init_zmk_keymap(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
    platform_layout_init_zmk_keymap_impl(layers, num_layers, key_map, num_keys);
}
#endif
#if defined(AGNOSTIC_USE_1D_ARRAY)
void platform_layout_init_1d_keymap(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
    platform_layout_init_1d_keymap_impl(layers, num_layers, key_map, num_keys);
}
#elif defined(AGNOSTIC_USE_2D_ARRAY)
void platform_layout_init_2D_keymap(const uint16_t* layers, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    platform_layout_init_2d_keymap_impl(layers,  num_layers, rows, cols);
}
#endif

bool platform_layout_is_valid_layer(uint8_t layer) {
    return platform_layout_is_valid_layer_impl(layer);
}

void platform_layout_set_layer(uint8_t layer) {
    printf("MOCK: Layer select %u\n", layer);
    add_layer_event(layer);

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
    platform_deferred_token deferred_token = schedule_deferred_callback(delay_ms, callback, data);
    printf("MOCK: Defer exec token %u for %u ms\n", deferred_token, delay_ms);
    return deferred_token;
}

bool platform_cancel_deferred_exec(platform_deferred_token token) {
    printf("MOCK: Cancel deferred exec token %u\n", token);
    return cancel_deferred_callback(token);
}

// Mock timer
platform_time_t monkeyboard_get_time_32(void) {
    return g_mock_state.timer;
}

// Helper functions for creating tap dance event sequences
event_t td_press(platform_keycode_t keycode, platform_time_t time) {
    event_t event;
    event.type = event_type_t::KEY_PRESS;
    event.keycode = keycode;
    event.time = time;
    return event;
}

event_t td_release(platform_keycode_t keycode, platform_time_t time) {
    event_t event;
    event.type = event_type_t::KEY_RELEASE;
    event.keycode = keycode;
    event.time = time;
    return event;
}

event_t td_layer(uint8_t layer, platform_time_t time) {
    event_t event;
    event.type = event_type_t::LAYER_CHANGE;
    event.layer = layer;
    event.time = time;
    return event;
}

event_t td_report_press(platform_keycode_t keycode, platform_time_t time) {
    event_t event;
    event.type = event_type_t::REPORT_PRESS;
    event.keycode = keycode;
    event.time = time;
    return event;
}

event_t td_report_release(platform_keycode_t keycode, platform_time_t time) {
    event_t event;
    event.type = event_type_t::REPORT_RELEASE;
    event.keycode = keycode;
    event.time = time;
    return event;
}

event_t td_report_send(platform_time_t time) {
    event_t event;
    event.type = event_type_t::REPORT_SEND;
    event.time = time;
    return event;
}
