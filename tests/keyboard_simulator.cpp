// Generic keypos finder for any keymap size
#include <cstdint>
#include <stdint.h>
#include "pipeline_executor.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "keyboard_simulator.hpp"

// KeyboardSimulator class implementation
KeyboardSimulator::KeyboardSimulator(uint8_t num_layers, uint8_t rows, uint8_t cols) : rows(rows), cols(cols) {
}

platform_keypos_t KeyboardSimulator::find_keypos(platform_keycode_t keycode) {
    for (uint8_t layer = 0; layer < num_layers; layer++) {
        for (uint8_t row = 0; row < rows; row++) {
            for (uint8_t col = 0; col < cols; col++) {
                if (platform_layout_get_keycode_from_layer(layer, {row, col}) == keycode) {
                    return {row, col};
                }
            }
        }
    }
    return {255, 255}; // Default return if not found
}

void KeyboardSimulator::press_key(platform_keycode_t keycode, uint16_t delay_ms) {
    if (delay_ms > 0) g_mock_state.advance_timer(delay_ms);

    platform_keypos_t keypos = find_keypos(keycode);
    abskeyevent_t event;
    event.keypos = keypos;
    event.pressed = true;
    event.time = static_cast<uint16_t>(g_mock_state.timer);

    pipeline_process_key(event);
}

void KeyboardSimulator::press_key_at(platform_keycode_t keycode, uint16_t time) {
    g_mock_state.set_timer(time);

    platform_keypos_t keypos = find_keypos(keycode);
    abskeyevent_t event;
    event.keypos = keypos;
    event.pressed = true;
    event.time = time;

    pipeline_process_key(event);
}

void KeyboardSimulator::release_key(platform_keycode_t keycode, uint16_t delay_ms) {
    if (delay_ms > 0) g_mock_state.advance_timer(delay_ms);

    platform_keypos_t keypos = find_keypos(keycode);
    abskeyevent_t event;
    event.keypos = keypos;
    event.pressed = false;
    event.time = static_cast<uint16_t>(g_mock_state.timer);

    pipeline_process_key(event);
}

void KeyboardSimulator::release_key_at(platform_keycode_t keycode, uint16_t time) {
    g_mock_state.set_timer(time);

    platform_keypos_t keypos = find_keypos(keycode);
    abskeyevent_t event;
    event.keypos = keypos;
    event.pressed = false;
    event.time = time;

    pipeline_process_key(event);
}

void KeyboardSimulator::tap_key(platform_keycode_t keycode, uint16_t hold_ms) {
    press_key(keycode);
    release_key(keycode, hold_ms);
}

void KeyboardSimulator::tap_key(platform_keycode_t keycode, uint16_t delay_before_ms, uint16_t hold_ms) {
    press_key(keycode, delay_before_ms);
    release_key(keycode, hold_ms);
}

void KeyboardSimulator::wait_ms(platform_time_t ms) {
    g_mock_state.advance_timer(ms);
}

// Factory function to create a keyboard simulator with layout
KeyboardSimulator create_layout(const platform_keycode_t* keymaps, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    platform_layout_init_2D_keymap(keymaps, num_layers, rows, cols);
    return KeyboardSimulator(num_layers, rows, cols);
}
