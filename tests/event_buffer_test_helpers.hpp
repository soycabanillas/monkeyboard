#pragma once

#include <vector>
#include <memory>
#include "platform_interface.h"
#include "platform_types.h"

extern "C" {
#include "key_event_buffer.h"
#include "key_press_buffer.h"
}

class EventBufferManager {
private:
    platform_key_event_buffer_t* event_buffer_;
    bool owns_buffer_;

public:
    EventBufferManager() : owns_buffer_(true) {
        event_buffer_ = platform_key_event_create();
    }

    explicit EventBufferManager(platform_key_event_buffer_t* external_buffer) 
        : event_buffer_(external_buffer), owns_buffer_(false) {
    }

    ~EventBufferManager() {
        if (owns_buffer_ && event_buffer_) {
            platform_key_event_reset(event_buffer_);
            // free(event_buffer_->key_press_buffer);
            // free(event_buffer_);
        }
    }

    // Non-copyable but movable
    EventBufferManager(const EventBufferManager&) = delete;
    EventBufferManager& operator=(const EventBufferManager&) = delete;
    EventBufferManager(EventBufferManager&& other) noexcept 
        : event_buffer_(other.event_buffer_), owns_buffer_(other.owns_buffer_) {
        other.event_buffer_ = nullptr;
        other.owns_buffer_ = false;
    }
    EventBufferManager& operator=(EventBufferManager&& other) noexcept {
        if (this != &other) {
            if (owns_buffer_ && event_buffer_) {
                platform_key_event_reset(event_buffer_);
                // free(event_buffer_->key_press_buffer);
                // free(event_buffer_);
            }
            event_buffer_ = other.event_buffer_;
            owns_buffer_ = other.owns_buffer_;
            other.event_buffer_ = nullptr;
            other.owns_buffer_ = false;
        }
        return *this;
    }

    platform_key_event_buffer_t* get() const { return event_buffer_; }

    // Add physical key events
    uint8_t add_physical_press(platform_time_t time, platform_keypos_t keypos) {
        bool buffer_full = false;
        return platform_key_event_add_physical_press(event_buffer_, time, keypos, &buffer_full);
    }

    bool add_physical_release(platform_time_t time, platform_keypos_t keypos) {
        bool buffer_full = false;
        return platform_key_event_add_physical_release(event_buffer_, time, keypos, &buffer_full);
    }

    // Helper methods for 2D array positions
    uint8_t add_physical_press(platform_time_t time, uint8_t row, uint8_t col) {
        platform_keypos_t keypos = {row, col};
        return add_physical_press(time, keypos);
    }

    bool add_physical_release(platform_time_t time, uint8_t row, uint8_t col) {
        platform_keypos_t keypos = {row, col};
        return add_physical_release(time, keypos);
    }

    // Query methods
    uint8_t get_event_count() const {
        return event_buffer_->event_buffer_pos;
    }

    platform_key_event_t* get_event(uint8_t index) const {
        if (index < event_buffer_->event_buffer_pos) {
            return &event_buffer_->event_buffer[index];
        }
        return nullptr;
    }

    std::vector<platform_key_event_t> get_all_events() const {
        std::vector<platform_key_event_t> events;
        events.reserve(event_buffer_->event_buffer_pos);
        for (uint8_t i = 0; i < event_buffer_->event_buffer_pos; ++i) {
            events.push_back(event_buffer_->event_buffer[i]);
        }
        return events;
    }

    uint8_t get_press_count() const {
        return event_buffer_->key_press_buffer->press_buffer_pos;
    }

    platform_key_press_key_press_t* get_press(uint8_t index) const {
        if (index < event_buffer_->key_press_buffer->press_buffer_pos) {
            return &event_buffer_->key_press_buffer->press_buffer[index];
        }
        return nullptr;
    }

    std::vector<platform_key_press_key_press_t> get_all_presses() const {
        std::vector<platform_key_press_key_press_t> presses;
        presses.reserve(event_buffer_->key_press_buffer->press_buffer_pos);
        for (uint8_t i = 0; i < event_buffer_->key_press_buffer->press_buffer_pos; ++i) {
            presses.push_back(event_buffer_->key_press_buffer->press_buffer[i]);
        }
        return presses;
    }

    // Reset the buffer
    void reset() {
        platform_key_event_reset(event_buffer_);
    }

    // Check if specific events exist
    bool has_event(platform_keycode_t keycode, bool is_press, uint8_t press_id) const {
        for (uint8_t i = 0; i < event_buffer_->event_buffer_pos; ++i) {
            const platform_key_event_t& event = event_buffer_->event_buffer[i];
            if (event.keycode == keycode && event.is_press == is_press && event.press_id == press_id) {
                return true;
            }
        }
        return false;
    }

    bool has_press_for_keypos(platform_keypos_t keypos) const {
        for (uint8_t i = 0; i < event_buffer_->key_press_buffer->press_buffer_pos; ++i) {
            const platform_key_press_key_press_t& press = event_buffer_->key_press_buffer->press_buffer[i];
            if (platform_compare_keyposition(press.keypos, keypos)) {
                return true;
            }
        }
        return false;
    }

    // Helper for 2D array
    bool has_press_for_keypos(uint8_t row, uint8_t col) const {
        platform_keypos_t keypos = {row, col};
        return has_press_for_keypos(keypos);
    }

    // Get events by criteria
    std::vector<platform_key_event_t> get_events_by_press_id(uint8_t press_id) const {
        std::vector<platform_key_event_t> events;
        for (uint8_t i = 0; i < event_buffer_->event_buffer_pos; ++i) {
            if (event_buffer_->event_buffer[i].press_id == press_id) {
                events.push_back(event_buffer_->event_buffer[i]);
            }
        }
        return events;
    }

    std::vector<platform_key_event_t> get_events_by_keycode(platform_keycode_t keycode) const {
        std::vector<platform_key_event_t> events;
        for (uint8_t i = 0; i < event_buffer_->event_buffer_pos; ++i) {
            if (event_buffer_->event_buffer[i].keycode == keycode) {
                events.push_back(event_buffer_->event_buffer[i]);
            }
        }
        return events;
    }
};
