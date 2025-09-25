#pragma once

#include <cstdint>
#include "platform_types.h"

// Keyboard simulation class
class KeyboardSimulator {
private:
    uint8_t rows;
    uint8_t cols;
    platform_keypos_t find_keypos(platform_keycode_t keycode);

public:
    // Constructor
    KeyboardSimulator(uint8_t rows = 4, uint8_t cols = 4);
    
    // Simplified key event simulation
    void press_key(platform_keycode_t keycode, uint16_t delay_ms = 0);
    void press_key_at(platform_keycode_t keycode, uint16_t time);
    void release_key(platform_keycode_t keycode, uint16_t delay_ms = 0);
    void release_key_at(platform_keycode_t keycode, uint16_t time);
    void tap_key(platform_keycode_t keycode, uint16_t hold_ms = 0);
    void tap_key(platform_keycode_t keycode, uint16_t delay_before_ms, uint16_t hold_ms);
    void wait_ms(platform_time_t ms);
};

// Factory function to create a keyboard simulator with layout
KeyboardSimulator create_layout(const platform_keycode_t* keymaps, uint8_t num_layers, uint8_t rows, uint8_t cols);
