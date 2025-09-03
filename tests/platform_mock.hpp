#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "gtest/gtest.h"
#include "platform_types.h"


void reset_mock_state(void);
void mock_set_timer(platform_time_t time);
platform_time_t mock_get_timer(void);
void mock_advance_timer(platform_time_t ms);
void mock_reset_timer(void);

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

// Tap dance event type for mixed key and layer events
enum class tap_dance_event_type_t : uint8_t {
    KEY_PRESS = 0,
    KEY_RELEASE = 1,
    LAYER_CHANGE = 2,
    REPORT_PRESS = 3, // For report press events
    REPORT_RELEASE = 4, // For report release events
    REPORT_SEND = 5 // For report send events
};

struct tap_dance_event_t {
    tap_dance_event_type_t type;
    union {
        platform_keycode_t keycode;  // For key events
        uint8_t layer;               // For layer events
    };
    platform_time_t time; // Time gap from previous event (0 = ignore time in comparison)

    bool operator==(const tap_dance_event_t& other) const {
        if (type != other.type) return false;
        switch (type) {
            case tap_dance_event_type_t::KEY_PRESS:
            case tap_dance_event_type_t::KEY_RELEASE:
                return keycode == other.keycode;
            case tap_dance_event_type_t::LAYER_CHANGE:
                return layer == other.layer;
            case tap_dance_event_type_t::REPORT_PRESS:
            case tap_dance_event_type_t::REPORT_RELEASE:
                return keycode == other.keycode; // For report events, we only care about keycode equality
            case tap_dance_event_type_t::REPORT_SEND:
                return true; // For report events, we only care about type equality
        }
        return false;
    }
};

// MockPlatformState class
struct MockPlatformState {
    platform_time_t timer;
    platform_deferred_token next_token;

    std::vector<deferred_call_t> deferred_calls;
    std::vector<key_action_t> key_actions; // Combined press/release history
    std::vector<uint8_t> layer_history;    // Layer change history
    std::vector<tap_dance_event_t> tap_dance_events;

    // Constructor and method declarations
    MockPlatformState();

    void set_timer(platform_time_t time);
    void advance_timer(platform_time_t ms);
    void reset();

    // New comparison methods with Google Test integration
    ::testing::AssertionResult tap_dance_event_actions_match_absolute(const std::vector<tap_dance_event_t>& expected) const;
    ::testing::AssertionResult tap_dance_event_actions_match_relative(const std::vector<tap_dance_event_t>& expected, platform_time_t start_time = 0) const;
    bool layer_history_matches(const std::vector<uint8_t>& expected) const;
};

// External declaration of the global mock state
extern MockPlatformState g_mock_state;

// Helper functions for creating tap dance event sequences
tap_dance_event_t td_press(platform_keycode_t keycode, platform_time_t time = 0);
tap_dance_event_t td_release(platform_keycode_t keycode, platform_time_t time = 0);
tap_dance_event_t td_layer(uint8_t layer, platform_time_t time = 0);
tap_dance_event_t td_report_press(platform_keycode_t keycode, platform_time_t time = 0);
tap_dance_event_t td_report_release(platform_keycode_t keycode, platform_time_t time = 0);
tap_dance_event_t td_report_send(platform_time_t time = 0);
