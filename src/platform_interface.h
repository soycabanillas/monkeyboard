#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Platform interface functions that must be implemented by each platform

// Key operations
void platform_tap_keycode(platform_keycode_t keycode);
void platform_register_keycode(platform_keycode_t keycode);
void platform_unregister_keycode(platform_keycode_t keycode);
bool platform_compare_keyposition(platform_keypos_t key1, platform_keypos_t key2);

// Layer operations
void platform_layout_init_keymap(uint8_t num_layers, uint32_t num_positions, platform_keycode_t **external_layouts);
#if defined(FRAMEWORK_QMK)
void platform_layout_init_qmk_keymap(const uint16_t keymap[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers);
#elif defined(FRAMEWORK_UNIT_TEST)
void platform_layout_init_2d_keymap(const uint16_t* keymap_array, uint8_t num_layers, uint8_t rows, uint8_t cols);
#endif
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
