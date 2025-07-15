#pragma once

#include <stdint.h>
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void platform_layout_init_keymap_impl(uint8_t num_layers, uint32_t num_positions, platform_keycode_t **external_layouts);
#if defined(FRAMEWORK_QMK)
void platform_layout_init_qmk_keymap_impl(const uint16_t keymap[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers);
#elif defined(FRAMEWORK_UNIT_TEST)
void platform_layout_init_2d_keymap_impl(const uint16_t* keymap_array, uint8_t num_layers, uint8_t rows, uint8_t cols);
#endif

void platform_layout_set_layer_impl(uint8_t layer);
uint8_t platform_layout_get_current_layer_impl(void);
platform_keycode_t platform_layout_get_keycode_impl(uint32_t position);
platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, uint32_t position);

#ifdef __cplusplus
}
#endif
