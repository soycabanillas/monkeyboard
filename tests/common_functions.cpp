// Generic keypos finder for any keymap size
#include <cstdint>
#include "pipeline_executor.h"
#include "platform_interface.h"
#include "platform_types.h"
#include "common_functions.hpp"

platform_keypos_t find_keypos(uint16_t keycode, uint8_t max_rows, uint8_t max_cols) {
    uint8_t layer = platform_layout_get_current_layer();

    for (uint8_t row = 0; row < max_rows; row++) {
        for (uint8_t col = 0; col < max_cols; col++) {
            if (platform_layout_get_keycode_from_layer(layer, {row, col}) == keycode) {
                return {row, col};
            }
        }
    }
    return {0, 0}; // Default return if not found
}

// Simplified key event simulation
void press_key(uint16_t keycode, uint16_t delay_ms) {
    if (delay_ms > 0) platform_wait_ms(delay_ms);

    platform_keypos_t keypos = find_keypos(keycode, 4, 4);
    abskeyevent_t event = {
        .keypos = keypos,
        .pressed = true,
        .time = static_cast<uint16_t>(platform_timer_read())
    };

    if (pipeline_process_key(event)) {
        platform_register_keycode(keycode);
    }
}

void release_key(uint16_t keycode, uint16_t delay_ms) {
    if (delay_ms > 0) platform_wait_ms(delay_ms);

    platform_keypos_t keypos = find_keypos(keycode, 4, 4);
    abskeyevent_t event = {
        .keypos = keypos,
        .pressed = false,
        .time = static_cast<uint16_t>(platform_timer_read())
    };

    if (pipeline_process_key(event)) {
        platform_unregister_keycode(keycode);
    }
}

void tap_key(uint16_t keycode, uint16_t hold_ms) {
    press_key(keycode);
    release_key(keycode, hold_ms);
}

void tap_key(uint16_t keycode, uint16_t delay_before_ms, uint16_t hold_ms) {
    press_key(keycode, delay_before_ms);
    release_key(keycode, hold_ms);
}
