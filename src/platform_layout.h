#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(AGNOSTIC_USE_1D_ARRAY)
void platform_layout_init_1d_keymap_impl(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys);
#elif defined(AGNOSTIC_USE_2D_ARRAY)
void platform_layout_init_2d_keymap_impl(const uint16_t* layers, uint8_t num_layers, uint8_t rows, uint8_t cols);
#endif
#if defined(FRAMEWORK_QMK)
#include "info_config.h"
void platform_layout_init_qmk_keymap_impl(const uint16_t layers[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers);
#elif defined(FRAMEWORK_ZMK)
void platform_layout_init_zmk_keymap_impl(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys);
#endif

bool platform_layout_is_valid_layer_impl(uint8_t layer);
void platform_layout_set_layer_impl(uint8_t layer);
uint8_t platform_layout_get_current_layer_impl(void);
platform_keycode_t platform_layout_get_keycode_impl(platform_keypos_t position);
platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, platform_keypos_t);

#ifdef __cplusplus
}
#endif
