#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "gtest/gtest.h"
#include "platform_types.h"


// Tap dance event type for mixed key and layer events
enum class event_type_t : uint8_t {
    KEY_PRESS = 0,
    KEY_RELEASE = 1,
    LAYER_CHANGE = 2,
    REPORT_PRESS = 3, // For report press events
    REPORT_RELEASE = 4, // For report release events
    REPORT_SEND = 5 // For report send events
};

struct event_t {
    event_type_t type;
    union {
        platform_keycode_t keycode;  // For key events
        uint8_t layer;               // For layer events
    };
    platform_time_t time; // Time gap from previous event (0 = ignore time in comparison)

    bool operator==(const event_t& other) const {
        if (type != other.type) return false;
        switch (type) {
            case event_type_t::KEY_PRESS:
            case event_type_t::KEY_RELEASE:
                return keycode == other.keycode;
            case event_type_t::LAYER_CHANGE:
                return layer == other.layer;
            case event_type_t::REPORT_PRESS:
            case event_type_t::REPORT_RELEASE:
                return keycode == other.keycode; // For report events, we only care about keycode equality
            case event_type_t::REPORT_SEND:
                return true; // For report events, we only care about type equality
        }
        return false;
    }
};

// MockPlatformState class
struct MockPlatformState {
    platform_time_t timer;
    std::vector<event_t> events;

    // Constructor and method declarations
    MockPlatformState();

    void set_timer(platform_time_t time);
    void advance_timer(platform_time_t ms);
    void reset();

    // New comparison methods with Google Test integration
    ::testing::AssertionResult event_actions_match_absolute(const std::vector<event_t>& expected) const;
    ::testing::AssertionResult event_actions_match_relative(const std::vector<event_t>& expected, platform_time_t start_time = 0) const;
};

// External declaration of the global mock state
extern MockPlatformState g_mock_state;

// Helper functions for creating tap dance event sequences
event_t td_press(platform_keycode_t keycode, platform_time_t time = 0);
event_t td_release(platform_keycode_t keycode, platform_time_t time = 0);
event_t td_layer(uint8_t layer, platform_time_t time = 0);
event_t td_report_press(platform_keycode_t keycode, platform_time_t time = 0);
event_t td_report_release(platform_keycode_t keycode, platform_time_t time = 0);
event_t td_report_send(platform_time_t time = 0);
