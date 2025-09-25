#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "monkeyboard_platform_detection.h"

#ifdef __cplusplus
extern "C" {
#endif

// Platform-specific keycode definitions - only when needed
#if defined(FRAMEWORK_QMK)

    #include "keycodes.h"
    #define PLATFORM_KC_LEFT_SHIFT  KC_LSFT
    #define PLATFORM_KC_RIGHT_SHIFT KC_RSFT
    #define PLATFORM_KC_LEFT_CTRL   KC_LCTL
    #define PLATFORM_KC_RIGHT_CTRL  KC_RCTL
    #define PLATFORM_KC_LEFT_ALT    KC_LALT
    #define PLATFORM_KC_RIGHT_ALT   KC_RALT
    #define PLATFORM_KC_LEFT_GUI    KC_LGUI
    #define PLATFORM_KC_RIGHT_GUI   KC_RGUI

#elif defined(FRAMEWORK_ZMK)

    #include <zmk/keys.h>
    #define PLATFORM_KC_LEFT_SHIFT  LSHIFT
    #define PLATFORM_KC_RIGHT_SHIFT RSHIFT
    #define PLATFORM_KC_LEFT_CTRL   LCTRL
    #define PLATFORM_KC_RIGHT_CTRL  RCTRL
    #define PLATFORM_KC_LEFT_ALT    LALT
    #define PLATFORM_KC_RIGHT_ALT   RALT
    #define PLATFORM_KC_LEFT_GUI    LGUI
    #define PLATFORM_KC_RIGHT_GUI   RGUI

#elif defined(FRAMEWORK_UNIT_TEST)

    #define PLATFORM_KC_LEFT_SHIFT  0xE1
    #define PLATFORM_KC_RIGHT_SHIFT 0xE5
    #define PLATFORM_KC_LEFT_CTRL   0xE0
    #define PLATFORM_KC_RIGHT_CTRL  0xE4
    #define PLATFORM_KC_LEFT_ALT    0xE2
    #define PLATFORM_KC_RIGHT_ALT   0xE6
    #define PLATFORM_KC_LEFT_GUI    0xE3
    #define PLATFORM_KC_RIGHT_GUI   0xE7

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

// Platform-specific type definitions
typedef uint32_t platform_keycode_t;

// Platform-agnostic deferred execution token
typedef uint32_t platform_deferred_token;

// Platform-agnostic time type (milliseconds)
typedef uint32_t platform_time_t;
#define PLATFORM_TIME_MAX UINT32_MAX

// Platform-agnostic key position type

#if defined(AGNOSTIC_USE_1D_ARRAY)      // Prioritize explicit selection over implicit framework detection
#elif defined(AGNOSTIC_USE_2D_ARRAY)    // Prioritize explicit selection over implicit framework detection
#elif defined(FRAMEWORK_QMK)            // If not explicitly defined, use framework detection. In this case, QMK uses a 2D array.
    #define AGNOSTIC_USE_2D_ARRAY
#elif defined(FRAMEWORK_ZMK)            // If not explicitly defined, use framework detection. In this case, ZMK uses a 1D array.
    #define AGNOSTIC_USE_1D_ARRAY
#elif defined(FRAMEWORK_UNIT_TEST)      // If not explicitly defined, use framework detection. In this case, Unit Test uses a 2D array.
    #define AGNOSTIC_USE_2D_ARRAY
#endif

#if defined(AGNOSTIC_USE_1D_ARRAY)
    typedef uint16_t platform_keypos_t;
    extern platform_keypos_t dummy_keypos; // Declaration only
#elif defined(AGNOSTIC_USE_2D_ARRAY)
    typedef struct {
        uint8_t row;
        uint8_t col;
    } platform_keypos_t;
    extern platform_keypos_t dummy_keypos; // Declaration only
#endif

// Platform-agnostic key event type
typedef struct {
    platform_keypos_t keypos;
    bool pressed;
    platform_time_t time;
} abskeyevent_t;

#if defined(AGNOSTIC_USE_1D_ARRAY)
typedef struct {
    uint8_t num_layers;
    uint32_t num_positions;
    uint8_t current_layer;
    platform_keycode_t **layouts;  // Pointer to array of 1D layout pointers
} custom_layout_t;
#elif defined(AGNOSTIC_USE_2D_ARRAY)
typedef struct {
    uint8_t num_layers;
    uint8_t rows;
    uint8_t cols;
    uint8_t current_layer;
    const platform_keycode_t *layouts;  // Pointer to flattened 2D layout data (layers * rows * cols)
} custom_layout_t;
#endif

#ifdef __cplusplus
}
#endif
