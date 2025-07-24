#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "key_press_buffer.h"
#include "key_virtual_buffer.h"
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_KEY_EVENT_MAX_ELEMENTS 20

typedef struct {
    bool is_physical; // Indicates if the key event is physical or virtual
    platform_keypos_t keypos;
    platform_keycode_t keycode;
    uint8_t layer;
    bool is_press;
    platform_time_t time;
    uint8_t press_id; // Unique ID for the key event, used to track presses/releases
} platform_key_event_t;

typedef struct {
    platform_key_event_t event_buffer[PLATFORM_KEY_EVENT_MAX_ELEMENTS];
    uint8_t event_buffer_pos;
    platform_virtual_press_buffer_t* virtual_press_buffer; // Buffer for virtual key presses
    platform_key_press_buffer_t* key_press_buffer; // Buffer for physical key presses
} platform_key_event_buffer_t;

typedef enum {
    PLATFORM_KEY_EVENT_TYPE_PHYSICAL_NONE_REMOVED,
    PLATFORM_KEY_EVENT_TYPE_PHYSICAL_PRESS_REMOVED,
    PLATFORM_KEY_EVENT_TYPE_PHYSICAL_PRESS_AND_RELEASE_REMOVED
} platform_key_event_remove_type_t;

// Key buffer functions
platform_key_event_buffer_t* platform_key_event_create(void);
void platform_key_event_reset(platform_key_event_buffer_t* event_buffer);

uint8_t platform_key_event_add_physical_press(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos);
uint8_t platform_key_event_add_virtual_press(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keycode_t keycode);
bool platform_key_event_add_physical_release(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos);
platform_key_event_remove_type_t platform_key_event_remove_physical_press_and_release(platform_key_event_buffer_t *event_buffer, platform_keypos_t keypos);
void platform_key_event_update_layer_for_physical_events(platform_key_event_buffer_t *event_buffer, uint8_t layer, uint8_t pos);

#ifdef DEBUG
void print_key_event_buffer(platform_key_event_buffer_t *event_buffer, size_t n_elements);
#endif

#ifdef __cplusplus
}
#endif


