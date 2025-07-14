#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

// Platform-specific keycode definitions - only when needed
#ifdef PLATFORM_QMK
    #include "quantum.h"
    #define PLATFORM_KC_LEFT_SHIFT  KC_LSFT
    #define PLATFORM_KC_RIGHT_SHIFT KC_RSFT
    #define PLATFORM_KC_LEFT_CTRL   KC_LCTL
    #define PLATFORM_KC_RIGHT_CTRL  KC_RCTL
    #define PLATFORM_KC_LEFT_ALT    KC_LALT
    #define PLATFORM_KC_RIGHT_ALT   KC_RALT
    #define PLATFORM_KC_LEFT_GUI    KC_LGUI
    #define PLATFORM_KC_RIGHT_GUI   KC_RGUI
#elif defined(PLATFORM_ZMK)
    #include <zmk/keys.h>
    #define PLATFORM_KC_LEFT_SHIFT  LSHIFT
    #define PLATFORM_KC_RIGHT_SHIFT RSHIFT
    #define PLATFORM_KC_LEFT_CTRL   LCTRL
    #define PLATFORM_KC_RIGHT_CTRL  RCTRL
    #define PLATFORM_KC_LEFT_ALT    LALT
    #define PLATFORM_KC_RIGHT_ALT   RALT
    #define PLATFORM_KC_LEFT_GUI    LGUI
    #define PLATFORM_KC_RIGHT_GUI   RGUI
#elif defined(UNIT_TESTING)
    #define PLATFORM_KC_LEFT_SHIFT  0xE1
    #define PLATFORM_KC_RIGHT_SHIFT 0xE5
    #define PLATFORM_KC_LEFT_CTRL   0xE0
    #define PLATFORM_KC_RIGHT_CTRL  0xE4
    #define PLATFORM_KC_LEFT_ALT    0xE2
    #define PLATFORM_KC_RIGHT_ALT   0xE6
    #define PLATFORM_KC_LEFT_GUI    0xE3
    #define PLATFORM_KC_RIGHT_GUI   0xE7
#endif

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

typedef struct {
    uint8_t num_layers;
    uint32_t num_positions;
    uint8_t current_layer;
    platform_keycode_t **layouts;  // Pointer to array of layout pointers
} custom_layout_t;

// Platform interface functions that must be implemented by each platform

// Key operations
void platform_tap_keycode(platform_keycode_t keycode);
void platform_register_keycode(platform_keycode_t keycode);
void platform_unregister_keycode(platform_keycode_t keycode);
bool platform_compare_keyposition(platform_keypos_t key1, platform_keypos_t key2);

// Layer operations
void platform_layout_init(uint8_t num_layers, uint32_t num_positions, platform_keycode_t **external_layouts);
void platform_layout_set_layer(uint8_t layer);
uint8_t platform_layout_get_current_layer(void);
platform_keycode_t platform_layout_get_keycode(uint32_t position);
platform_keycode_t platform_layout_get_keycode_from_layer(uint8_t layer, uint32_t position);


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
