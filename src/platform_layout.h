#pragma once

#include <stdint.h>
#include "platform_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

void platform_layout_init_impl(uint8_t num_layers, uint32_t num_positions, platform_keycode_t **external_layouts);
void platform_layout_set_layer_impl(uint8_t layer);
uint8_t platform_layout_get_current_layer_impl(void);
platform_keycode_t platform_layout_get_keycode_impl(uint32_t position);
platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, uint32_t position);

#ifdef __cplusplus
}
#endif
