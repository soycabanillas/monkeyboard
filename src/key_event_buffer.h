#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_KEY_EVENT_MAX_ELEMENTS 10

typedef struct {
    platform_keypos_t keypos;
    platform_keycode_t keycode;
    uint8_t layer;
    bool is_press;
    platform_time_t time;
} platform_key_event_t;

typedef struct {
    platform_key_event_t event_buffer[PLATFORM_KEY_EVENT_MAX_ELEMENTS];
    uint8_t event_buffer_pos;
} platform_key_event_buffer_t;

// Key buffer functions
platform_key_event_buffer_t* platform_key_event_create(void);
void platform_key_event_reset(platform_key_event_buffer_t* event_buffer);

bool platform_key_event_add_event(platform_key_event_buffer_t *event_buffer, platform_time_t time, uint8_t layer, platform_keypos_t keypos, platform_keycode_t keycode, bool is_press);
void platform_key_event_remove_event(platform_key_event_buffer_t *event_buffer, uint8_t pos);

#ifdef DEBUG
void print_key_event_buffer(platform_key_event_buffer_t *event_buffer, size_t n_elements);
#endif

#ifdef __cplusplus
}
#endif


