#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Platform-agnostic key code type
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
void platform_layer_on(uint8_t layer);
void platform_layer_off(uint8_t layer);
void platform_layer_set(uint8_t layer);
void platform_layer_select(uint8_t layer);  // Alias for platform_layer_set
uint8_t platform_get_highest_layer(void);

// Time operations
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
