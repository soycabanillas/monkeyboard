#include "platform_layout.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "platform_types.h"

static custom_layout_t* manager = NULL;
static uint8_t keymap_rows = 0;
static uint8_t keymap_cols = 0;
static uint16_t keymap_num_keys = 0;

#if defined (AGNOSTIC_USE_1D_ARRAY)

void platform_layout_init_custom_1D_keymap_impl(void* layers, uint8_t num_layers, uint16_t num_keys, get_keycode_from_layer_def get_keycode_from_layer_fn) {
    // Allocate the manager structure
    manager = (custom_layout_t*) malloc(sizeof(custom_layout_t));
    if (!manager) {
        return;
    }

    // Set basic properties
    manager->num_layers = num_layers;
    manager->num_positions = num_keys;
    manager->current_layer = 0; // Initialize to the first layer

    // Allocate array of pointers to layouts
    manager->layouts = (platform_keycode_t **)malloc(sizeof(platform_keycode_t*) * num_layers);
    if (!manager->layouts) {
        return;
    }

    // Reference the external layouts (no copying)
    for (uint8_t i = 0; i < num_layers; i++) {
        manager->layouts[i] = layers[i];
    }

    manager->get_keycode_from_layer_fn = get_keycode_from_layer_fn;

    keymap_rows = 1; // For 1D array, we consider it as a single row
    keymap_cols = num_keys; // All keys in a single row
    keymap_num_keys = num_keys;
}

static platform_keycode_t platform_layout_get_keycode_from_layer_internal(uint8_t layer, platform_keypos_t position) {
    if (!manager || layer >= manager->num_layers || position >= manager->num_positions) {
        return 0;
    }
    return manager->layouts[layer][position];
}

void platform_layout_init_1d_keymap_impl(platform_keycode_t **layers, uint8_t num_layers, uint16_t num_keys) {
    platform_layout_init_custom_1D_keymap_impl((void*)layers, num_layers, num_keys, platform_layout_get_keycode_from_layer_internal);
}

#elif defined (AGNOSTIC_USE_2D_ARRAY)

void platform_layout_init_custom_2D_keymap_impl(void* layers, uint8_t num_layers, uint8_t rows, uint8_t cols, get_keycode_from_layer_def get_keycode_from_layer_fn) {
    // Allocate the manager structure
    manager = (custom_layout_t*) malloc(sizeof(custom_layout_t));
    if (!manager) {
        return;
    }

    // Set basic properties
    manager->num_layers = num_layers;
    manager->rows = rows;
    manager->cols = cols;
    manager->current_layer = 0; // Initialize to the first layer

    // Store direct reference to the flattened 2D layout data (no conversion needed)
    manager->layouts = layers;

    manager->get_keycode_from_layer_fn = get_keycode_from_layer_fn;

    keymap_rows = rows;
    keymap_cols = cols;
    keymap_num_keys = rows * cols;
}

static platform_keycode_t platform_layout_get_keycode_from_layer_internal(uint8_t layer, platform_keypos_t position) {
    if (!manager || layer >= manager->num_layers || position.row >= keymap_rows || position.col >= keymap_cols) {
        return 0;
    }

    uint32_t layer_size = keymap_rows * keymap_cols;
    uint32_t offset = layer * layer_size + position.row * keymap_cols + position.col;
    return (platform_keycode_t)manager->layouts[offset];
}

void platform_layout_init_2d_keymap_impl(const platform_keycode_t* layers, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    platform_layout_init_custom_2D_keymap_impl((void*)layers, num_layers, rows, cols, platform_layout_get_keycode_from_layer_internal);
}

#endif

#if defined(FRAMEWORK_QMK)

void platform_layout_init_qmk_keymap_impl(const platform_keycode_t layers[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers) {
    platform_layout_init_2d_keymap_impl((const platform_keycode_t*)layers, num_layers, MATRIX_ROWS, MATRIX_COLS);
}

#elif defined(FRAMEWORK_ZMK)

void platform_layout_init_zmk_keymap_impl(platform_keycode_t **layers, uint8_t num_layers, uint16_t num_keys) {
    platform_layout_init_1d_keymap_impl(layers, num_layers, num_keys);
}

#endif

bool platform_layout_is_valid_layer_impl(uint8_t layer) {
    if (!manager) {
        return false; // No layout initialized
    }
    return (layer < manager->num_layers);
}

void platform_layout_set_layer_impl(uint8_t layer) {
    if (!manager || layer >= manager->num_layers) {
        return; // Invalid layer, do nothing
    }
    manager->current_layer = layer; // Update the manager's current layer
}

uint8_t platform_layout_get_current_layer_impl(void) {
    if (!manager) {
        return 0; // or some "no layer" value
    }
    return manager->current_layer;
}

platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, platform_keypos_t position) {
    return manager->get_keycode_from_layer_fn(layer, position);
}

platform_keycode_t platform_layout_get_keycode_impl(platform_keypos_t position) {
    return platform_layout_get_keycode_from_layer_impl(manager->current_layer, position);
}
