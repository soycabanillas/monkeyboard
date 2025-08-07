// Generic keypos finder for any keymap size
#include <cstdint>
#include "pipeline_executor.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "common_functions.hpp"

platform_keypos_t find_keypos(uint16_t keycode, uint8_t max_rows, uint8_t max_cols) {
    for (uint8_t row = 0; row < max_rows; row++) {
        for (uint8_t col = 0; col < max_cols; col++) {
            if (platform_layout_get_keycode_from_layer(0, {row, col}) == keycode) {
                return {row, col};
            }
        }
    }
    return {255, 255}; // Default return if not found
}

// Simplified key event simulation
void press_key(uint16_t keycode, uint16_t delay_ms) {
    if (delay_ms > 0) mock_advance_timer(delay_ms);

    platform_keypos_t keypos = find_keypos(keycode, 4, 4);
    abskeyevent_t event = {
        .keypos = keypos,
        .pressed = true,
        .time = static_cast<uint16_t>(mock_get_timer())
    };

    pipeline_process_key(event);
}

void press_key_at(uint16_t keycode, uint16_t time) {

    mock_set_timer(time);

    platform_keypos_t keypos = find_keypos(keycode, 4, 4);
    abskeyevent_t event = {
        .keypos = keypos,
        .pressed = true,
        .time = time
    };

    pipeline_process_key(event);
}

void release_key(uint16_t keycode, uint16_t delay_ms) {
    if (delay_ms > 0) mock_advance_timer(delay_ms);

    platform_keypos_t keypos = find_keypos(keycode, 4, 4);
    abskeyevent_t event = {
        .keypos = keypos,
        .pressed = false,
        .time = static_cast<uint16_t>(mock_get_timer())
    };

    pipeline_process_key(event);
}

void release_key_at(uint16_t keycode, uint16_t time) {
    mock_set_timer(time);

    platform_keypos_t keypos = find_keypos(keycode, 4, 4);
    abskeyevent_t event = {
        .keypos = keypos,
        .pressed = false,
        .time = time
    };

    pipeline_process_key(event);
}

void tap_key(uint16_t keycode, uint16_t hold_ms) {
    press_key(keycode);
    release_key(keycode, hold_ms);
}

void tap_key(uint16_t keycode, uint16_t delay_before_ms, uint16_t hold_ms) {
    press_key(keycode, delay_before_ms);
    release_key(keycode, hold_ms);
}

void wait_ms(platform_time_t ms) {
    mock_advance_timer(ms);
}
