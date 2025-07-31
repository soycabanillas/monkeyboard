#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "platform_types.h"


void reset_mock_state(void);
void mock_advance_timer(platform_time_t ms);
void mock_reset_timer(void);
void mock_set_layer(uint8_t layer);
void mock_print_state(void);

// Key event tracking
struct key_event_t {
    platform_keycode_t keycode;
    bool pressed;
    platform_time_t timestamp;
};

// Deferred call structure
struct deferred_call_t {
    platform_deferred_token token; // Token for deferred execution
    uint32_t execution_time;
    void (*callback)(void*);
    void* data;
};

// Key action tracking (0 = press, 1 = release)
struct key_action_t {
    platform_keycode_t keycode;
    uint8_t action; // 0 = press, 1 = release
    platform_time_t time; // Timestamp when action occurred (0 = ignore time in comparison)

    bool operator==(const key_action_t& other) const {
        return keycode == other.keycode && action == other.action;
    }

    bool operator_with_time(const key_action_t& other) const {
        return keycode == other.keycode && action == other.action && time == other.time;
    }
};

// MockPlatformState class
struct MockPlatformState {
    platform_time_t timer;
    uint8_t current_layer;
    platform_deferred_token next_token;
    platform_time_t last_key_event_time;

    std::vector<key_event_t> key_events;
    std::set<platform_keycode_t> pressed_keys;
    std::vector<platform_keycode_t> register_key_calls;
    std::vector<platform_keycode_t> unregister_key_calls;
    std::vector<uint8_t> layer_select_calls;
    std::vector<deferred_call_t> deferred_calls;
    std::vector<key_action_t> key_actions; // Combined press/release history
    std::vector<uint8_t> layer_history;    // Layer change history

    platform_keycode_t last_sent_key;
    platform_keycode_t last_registered_key;
    platform_keycode_t last_unregistered_key;
    uint8_t last_selected_layer;

    // Constructor and method declarations
    MockPlatformState();

    int send_key_calls_count() const;
    int register_key_calls_count() const;
    int unregister_key_calls_count() const;
    int layer_select_calls_count() const;
    int key_events_count() const;
    size_t pressed_keys_count() const;
    bool is_key_pressed(platform_keycode_t keycode) const;

    void advance_timer(platform_time_t ms);
    void reset();
    void print_state() const;
    void record_key_event(platform_keycode_t keycode, bool pressed);

    // New comparison methods with Google Test integration
    ::testing::AssertionResult key_actions_match(const std::vector<key_action_t>& expected) const;
    ::testing::AssertionResult key_actions_match_with_time(const std::vector<key_action_t>& expected) const;
    ::testing::AssertionResult key_actions_match_with_time_gaps(const std::vector<key_action_t>& expected, platform_time_t start_time = 0) const;
    bool layer_history_matches(const std::vector<uint8_t>& expected) const;
    std::vector<key_action_t> get_key_actions_since(size_t start_index) const;
    std::vector<uint8_t> get_layer_history_since(size_t start_index) const;
};

// External declaration of the global mock state
extern MockPlatformState g_mock_state;

// Helper functions for creating expected sequences
key_action_t press(platform_keycode_t keycode, platform_time_t time = 0);
key_action_t release(platform_keycode_t keycode, platform_time_t time = 0);
std::vector<key_action_t> tap_sequence(platform_keycode_t keycode);
