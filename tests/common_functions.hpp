#pragma once

#include <cstdint>
#include "platform_types.h"

#define BEFORE_TAP_TIMEOUT 100
#define JUST_BEFORE_TAP_TIMEOUT 199   // 1ms before tap timeout
#define TAP_TIMEOUT 200
#define JUST_AFTER_TAP_TIMEOUT 201     // 1ms after tap timeout
#define AFTER_TAP_TIMEOUT 250

#define BEFORE_HOLD_TIMEOUT 100
#define JUST_BEFORE_HOLD_TIMEOUT 199   // 1ms before hold timeout
#define HOLD_TIMEOUT 200
#define JUST_AFTER_HOLD_TIMEOUT 201    // 1ms after hold timeout
#define AFTER_HOLD_TIMEOUT 250

platform_keypos_t find_keypos(uint16_t keycode, uint8_t max_rows = 4, uint8_t max_cols = 4);

// Simplified key event simulation
void press_key(uint16_t keycode, uint16_t delay_ms = 0);
void press_key_at(uint16_t keycode, uint16_t time);
void release_key(uint16_t keycode, uint16_t delay_ms = 0);
void release_key_at(uint16_t keycode, uint16_t time);
void tap_key(uint16_t keycode, uint16_t hold_ms = 0);
void tap_key(uint16_t keycode, uint16_t delay_before_ms, uint16_t hold_ms);
void wait_ms(platform_time_t ms);
