#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS 10

typedef struct {
    platform_keycode_t keycode;
    uint8_t press_id; // Unique ID for the key press, used to track presses/releases
    bool ignore_release; // If true, the release of this key will be ignored
} platform_virtual_press_virtual_press_t;

typedef struct {
    platform_virtual_press_virtual_press_t press_buffer[PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS];
    uint8_t press_buffer_pos;
} platform_virtual_press_buffer_t;

// Key buffer safe functions
platform_virtual_press_buffer_t* platform_virtual_press_create(void);
void platform_virtual_press_reset(platform_virtual_press_buffer_t* press_buffer);

platform_virtual_press_virtual_press_t* platform_virtual_press_add_press(platform_virtual_press_buffer_t *virtual_buffer, platform_keycode_t keycode, uint8_t press_id);
bool platform_virtual_press_remove_press(platform_virtual_press_buffer_t *virtual_buffer, uint8_t press_id);
platform_virtual_press_virtual_press_t* platform_virtual_press_get_press_from_press_id(platform_virtual_press_buffer_t *virtual_buffer, uint8_t press_id);
bool platform_virtual_press_ignore_release(platform_virtual_press_buffer_t *virtual_buffer, uint8_t press_id);

#ifdef __cplusplus
}
#endif
