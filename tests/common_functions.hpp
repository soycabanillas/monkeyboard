#pragma once

#include <cstdint>
#include "platform_types.h"

platform_keypos_t find_keypos(uint16_t keycode, uint8_t max_rows = 4, uint8_t max_cols = 4);

// Simplified key event simulation
void press_key(uint16_t keycode, uint16_t delay_ms = 0);
void release_key(uint16_t keycode, uint16_t delay_ms = 0);
void tap_key(uint16_t keycode, uint16_t hold_ms = 0);
void tap_key(uint16_t keycode, uint16_t delay_before_ms, uint16_t hold_ms);
