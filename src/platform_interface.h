#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../tests/keycodes.h"  // Assuming this header defines planck_keycodes

#ifdef __cplusplus
extern "C" {
#endif

#define MACRO_KEY_MODIFIER_LEFT_SHIFT  (1 << 0)
#define MACRO_KEY_MODIFIER_RIGHT_SHIFT (1 << 1)
#define MACRO_KEY_MODIFIER_LEFT_CTRL   (1 << 2)
#define MACRO_KEY_MODIFIER_RIGHT_CTRL  (1 << 3)
#define MACRO_KEY_MODIFIER_LEFT_ALT    (1 << 4)
#define MACRO_KEY_MODIFIER_RIGHT_ALT   (1 << 5)
#define MACRO_KEY_MODIFIER_LEFT_GUI    (1 << 6)
#define MACRO_KEY_MODIFIER_RIGHT_GUI   (1 << 7)

typedef enum {
    MODIFIER_LEFT_SHIFT  = (1 << 0),
    MODIFIER_RIGHT_SHIFT = (1 << 1),
    MODIFIER_LEFT_CTRL   = (1 << 2),
    MODIFIER_RIGHT_CTRL  = (1 << 3),
    MODIFIER_LEFT_ALT    = (1 << 4),
    MODIFIER_RIGHT_ALT   = (1 << 5),
    MODIFIER_LEFT_GUI    = (1 << 6),
    MODIFIER_RIGHT_GUI   = (1 << 7)
} modifier_t;

#define PLATFORM_KC_LEFT_SHIFT  KC_LEFT_SHIFT
#define PLATFORM_KC_RIGHT_SHIFT KC_RIGHT_SHIFT
#define PLATFORM_KC_LEFT_CTRL KC_LEFT_CTRL
#define PLATFORM_KC_RIGHT_CTRL KC_RIGHT_CTRL
#define PLATFORM_KC_LEFT_ALT KC_LEFT_ALT
#define PLATFORM_KC_RIGHT_ALT KC_RIGHT_ALT
#define PLATFORM_KC_LEFT_GUI KC_LEFT_GUI
#define PLATFORM_KC_RIGHT_GUI KC_RIGHT_GUI

// Platform-specific type definitions
typedef uint16_t platform_keycode_t;

// Platform-agnostic deferred execution token
typedef uint32_t platform_deferred_token;

// Platform-agnostic time type (milliseconds)
typedef uint32_t platform_time_t;

// Platform-agnostic key position type
typedef struct {
    uint8_t row;
    uint8_t col;
} platform_keypos_t;

// Platform-agnostic key event type
typedef struct {
    platform_keypos_t key;
    bool pressed;
    platform_time_t time;
} abskeyevent_t;

// Platform interface functions that must be implemented by each platform

// Key operations
void platform_send_key(platform_keycode_t keycode);
void platform_register_key(platform_keycode_t keycode);
void platform_unregister_key(platform_keycode_t keycode);

// Layer operations
void platform_layer_select(uint8_t layer);

// Time operations
void platform_wait_ms(platform_time_t ms);
platform_time_t platform_timer_read(void);
platform_time_t platform_timer_elapsed(platform_time_t last);

// Deferred execution
platform_deferred_token platform_defer_exec(uint32_t delay_ms, void (*callback)(void*), void* data);
bool platform_cancel_deferred_exec(platform_deferred_token token);

// Memory operations
void* platform_malloc(size_t size);
void platform_free(void* ptr);

#ifdef __cplusplus
}
#endif
