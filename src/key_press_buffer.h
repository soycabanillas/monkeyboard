// The press buffer stores the real state of the keyboard and is updated in real-time as keys are pressed and released, while the event buffer stores the history of key events and is used for processing key events in order.
// Only one press buffer with the same key position can exist at a time on the press buffer, while several presses and several releases can exist in the event buffer with the same key position.
//
// The event buffer allows to replay key events several times by the pipelines, so it can check the sequence of events against several patterns (hold-tap, combo).
// The press buffer is used to:
// - Ignoring misfires (two key presses without a key release in between, two key releases without a key press in between or a release without a previous press).
// - To relate the key press and the key release events of the event buffer through a press id.
//
// A press can exist in the press buffer without a corresponding press in the event buffer (same press id) when the event press has been processed. Storing the press_id on the press buffer too allows to assign the same press_id to the corresponding release event even if the press event has been removed from the event buffer.
// A press can not exist in the press buffer at the same time than its corresponding (same press id) release in the event buffer.
// 
// When a press is triggered, it looks at the press buffer to see if the key is already pressed (same key position):
// - If found: it ignores it.
// - If not found: it adds a new press event to the event buffer and a press to the press buffer with the same press_id.
// When a release is triggered, it looks at the press buffer to find the corresponding press event (same key position):
// - If found: it adds a new release event to the event buffer with the same press_id than the press from the press buffer (the press from the event buffer might already be processed and removed) and removes the press from the press buffer.
// - If not found: it ignores it.

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

#ifdef MONKEYBOARD_DEBUG
void print_key_press_buffer(platform_key_press_buffer_t *event_buffer);
#endif

#ifdef __cplusplus
}
#endif


