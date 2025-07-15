#include "platform_layout.h"
#include <stdint.h>
#include <stdlib.h>

custom_layout_t* manager = NULL;

void platform_layout_init_keymap_impl(uint8_t num_layers, uint32_t num_positions, platform_keycode_t **external_layouts) {
        // Allocate the manager structure
    manager = (custom_layout_t*) malloc(sizeof(custom_layout_t));
    if (!manager) {
        return;
    }

    // Set basic properties
    manager->num_layers = num_layers;
    manager->num_positions = num_positions;

    // Allocate array of pointers to layouts
    manager->layouts = (platform_keycode_t **)malloc(sizeof(platform_keycode_t*) * num_layers);
    if (!manager->layouts) {
        free(manager);
        return;
    }

    // Reference the external layouts (no copying)
    for (uint8_t i = 0; i < num_layers; i++) {
        manager->layouts[i] = external_layouts[i];
    }
}

static platform_keycode_t** convert_keymap_impl(const void* keymap_ptr,
                                               uint8_t num_layers,
                                               uint8_t rows,
                                               uint8_t cols);

#if defined(FRAMEWORK_QMK)

void platform_layout_init_qmk_keymap_impl(const uint16_t keymap[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers) {
    platform_keycode_t** layouts_1d = convert_keymap_impl((const void*)keymap, num_layers, MATRIX_ROWS, MATRIX_COLS);
    platform_layout_init_keymap_impl(num_layers, MATRIX_ROWS * MATRIX_COLS, layouts_1d);
}

#elif defined(FRAMEWORK_UNIT_TEST)

void platform_layout_init_2d_keymap_impl(const uint16_t* keymap_array, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    platform_keycode_t** layouts_1d = convert_keymap_impl((const void*)keymap_array, num_layers, rows, cols);
    platform_layout_init_keymap_impl(num_layers, rows * cols, layouts_1d);
}

#endif

void platform_layout_set_layer_impl(uint8_t layer) {
    manager->current_layer = layer; // Update the manager's current layer
}

uint8_t platform_layout_get_current_layer_impl(void) {
    if (!manager) {
        return 0; // or some "no layer" value
    }
    return manager->current_layer;
}

platform_keycode_t platform_layout_get_keycode_impl(uint32_t position) {
    if (!manager || position >= manager->num_positions) {
        return 0; // or some "no key" value
    }
    return manager->layouts[manager->current_layer][position];
}

platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, uint32_t position) {
    if (!manager || layer >= manager->num_layers || position >= manager->num_positions) {
        return 0;
    }
    return manager->layouts[layer][position];
}

static platform_keycode_t** convert_keymap_impl(const void* keymap_ptr,
                                               uint8_t num_layers,
                                               uint8_t rows,
                                               uint8_t cols) {
    uint32_t layer_size = rows * cols;
    platform_keycode_t** layouts_1d = malloc(num_layers * sizeof(platform_keycode_t*));

    if (!layouts_1d) return NULL;

    for (uint8_t layer = 0; layer < num_layers; layer++) {
        layouts_1d[layer] = malloc(layer_size * sizeof(platform_keycode_t));
        if (!layouts_1d[layer]) {
            for (uint8_t i = 0; i < layer; i++) {
                free(layouts_1d[i]);
            }
            free(layouts_1d);
            return NULL;
        }

        const uint16_t* src = (const uint16_t*)keymap_ptr + (layer * layer_size);
        for (uint32_t i = 0; i < layer_size; i++) {
            layouts_1d[layer][i] = (platform_keycode_t)src[i];
        }
    }

    return layouts_1d;
}
