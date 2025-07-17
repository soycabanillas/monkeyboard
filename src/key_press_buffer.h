#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_KEY_BUFFER_MAX_ELEMENTS 5

typedef struct {
    platform_keypos_t key;
    platform_keycode_t keycode;
    platform_time_t time;
} platform_key_press_key_press_t;

typedef struct {
    platform_key_press_key_press_t press_buffer[PLATFORM_KEY_BUFFER_MAX_ELEMENTS];
    uint8_t press_buffer_pos;
} platform_key_press_buffer_t;

// Key buffer safe functions
platform_key_press_buffer_t* platform_key_press_create(void);
void platform_key_press_reset(platform_key_press_buffer_t* press_buffer);

bool platform_key_press_keycode_is_pressed(platform_key_press_buffer_t *press_buffer, platform_keycode_t keycode);
bool platform_key_press_keypos_is_pressed(platform_key_press_buffer_t *press_buffer, platform_keypos_t keypos);
bool platform_key_press_add_press(platform_key_press_buffer_t *press_buffer, platform_time_t time, uint8_t layer, platform_keypos_t keypos, bool is_press);
void platform_key_press_remove_press(platform_key_press_buffer_t *press_buffer, uint8_t pos);

#ifdef __cplusplus
}
#endif


