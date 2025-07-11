#pragma once

#include <cstdint>
#include "../src/platform_interface.h"

#ifdef __cplusplus
#include <vector>
#include <set>

// Key event tracking structure
struct key_event_t {
    platform_keycode_t keycode;
    bool pressed;  // true for press, false for release
    platform_time_t timestamp;
};

// Mock platform state structure (C++ only)
struct MockPlatformState {
    platform_time_t timer;
    uint8_t current_layer;
    platform_deferred_token next_token;
    platform_time_t last_key_event_time;

    std::vector<key_event_t> key_events;
    std::set<platform_keycode_t> pressed_keys;
    std::vector<platform_keycode_t> send_key_calls;
    std::vector<platform_keycode_t> register_key_calls;
    std::vector<platform_keycode_t> unregister_key_calls;
    std::vector<uint8_t> layer_select_calls;

    platform_keycode_t last_sent_key;
    platform_keycode_t last_registered_key;
    platform_keycode_t last_unregistered_key;
    uint8_t last_selected_layer;

    MockPlatformState();
    void record_key_event(platform_keycode_t keycode, bool pressed);
    void advance_timer(platform_time_t ms);
    void reset();
    void print_state() const;

    // Convenient accessor methods
    int send_key_calls_count() const;
    int register_key_calls_count() const;
    int unregister_key_calls_count() const;
    int layer_select_calls_count() const;
    int key_events_count() const;
    size_t pressed_keys_count() const;
    bool is_key_pressed(platform_keycode_t keycode) const;
};

// Global mock state (C++ only)
extern MockPlatformState g_mock_state;

extern "C" {
#endif

// Test utilities for controlling the mock platform
void mock_advance_timer(platform_time_t ms);
void mock_reset_timer(void);
void mock_set_layer(uint8_t layer);
void mock_print_state(void);
void reset_mock_state(void);

#ifdef __cplusplus
}
#endif
