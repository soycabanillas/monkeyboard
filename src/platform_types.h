#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "monkeyboard_platform_detection.h"

#ifdef __cplusplus
extern "C" {
#endif

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

// Internal structure to manage layouts and layers
typedef platform_keycode_t (*get_keycode_from_layer_def)(uint8_t layer, platform_keypos_t position);
typedef struct {
    uint8_t num_layers;
    uint8_t current_layer;
    #if defined(AGNOSTIC_USE_1D_ARRAY)
    uint32_t num_positions;
    platform_keycode_t **layouts;  // Pointer to array of 1D layout pointers
    #elif defined(AGNOSTIC_USE_2D_ARRAY)
    uint8_t rows;
    uint8_t cols;
    const platform_keycode_t *layouts;  // Pointer to flattened 2D layout data (layers * rows * cols)
    #endif
    get_keycode_from_layer_def get_keycode_from_layer_fn;
} custom_layout_t;

#ifdef __cplusplus
}
#endif
