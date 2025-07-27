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
    bool is_press;
} platform_virtual_buffer_virtual_event_t;

typedef struct {
    platform_virtual_buffer_virtual_event_t press_buffer[PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS];
    uint8_t press_buffer_pos;
} platform_virtual_event_buffer_t;

// Key buffer safe functions
platform_virtual_event_buffer_t* platform_virtual_event_create(void);
void platform_virtual_event_reset(platform_virtual_event_buffer_t* virtual_buffer);

bool platform_virtual_event_add_press(platform_virtual_event_buffer_t *virtual_buffer, platform_keycode_t keycode);
bool platform_virtual_event_add_release(platform_virtual_event_buffer_t *virtual_buffer, platform_keycode_t keycode);

#ifdef __cplusplus
}
#endif
