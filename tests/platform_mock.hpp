#pragma once

#include "../src/platform_interface.h"
#include <vector>
#include <set>


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

// MockPlatformState class
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
};

// External declaration of the global mock state
extern MockPlatformState g_mock_state;
