#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_KEY_BUFFER_MAX_ELEMENTS 10

typedef struct {
    platform_keypos_t keypos;
    uint8_t press_id; // Unique ID for the key press, used to track presses/releases
    platform_keycode_t keycode; // Keycode associated with the last key press. Ensures that a release will always have the same keycode as the press
    bool ignore_release; // If true, the release of this key will be ignored
} platform_key_press_key_press_t;

typedef struct {
    platform_key_press_key_press_t press_buffer[PLATFORM_KEY_BUFFER_MAX_ELEMENTS];
    uint8_t press_buffer_pos;
} platform_key_press_buffer_t;

// Key buffer safe functions
platform_key_press_buffer_t* platform_key_press_create(void);
void platform_key_press_reset(platform_key_press_buffer_t* press_buffer);

platform_key_press_key_press_t* platform_key_press_add_press(platform_key_press_buffer_t *press_buffer, platform_keypos_t keypos, platform_keycode_t keycode, uint8_t press_id);
bool platform_key_press_remove_press(platform_key_press_buffer_t *press_buffer, platform_keypos_t keypos);

platform_key_press_key_press_t* platform_key_press_get_press_from_keypos(platform_key_press_buffer_t *press_buffer, platform_keypos_t keypos);
platform_key_press_key_press_t* platform_key_press_get_press_from_press_id(platform_key_press_buffer_t *press_buffer, uint8_t press_id);
bool platform_key_press_ignore_release_by_press_id(platform_key_press_buffer_t *press_buffer, uint8_t press_id);

#ifdef DEBUG
void print_key_press_buffer(platform_key_press_buffer_t *event_buffer);
#endif

#ifdef __cplusplus
}
#endif


