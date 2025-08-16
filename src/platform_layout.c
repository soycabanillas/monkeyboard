#include "platform_layout.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "platform_types.h"

static custom_layout_t* manager = NULL;
static uint8_t keymap_rows = 0;
static uint8_t keymap_cols = 0;
static uint16_t keymap_num_keys = 0;

#if defined(AGNOSTIC_USE_1D_ARRAY)
// Initialize the keymap and layers for 1D or 2D array layouts
// This function allocates memory for the manager and initializes the keymap and layers
// It is called by platform_layout_init_2d_keymap_impl
// The function takes a pointer to the layers, number of layers, rows, and columns as parameters
// The function returns nothing, but it modifies the global manager and key_map variables
// The function assumes that the input parameters are valid and does not check for errors
// The function is defined as static to limit its visibility to this file
static void init_1D_map_and_layers(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
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

    keymap_rows = 1; // For 1D array, we consider it as a single row
    keymap_cols = num_keys; // All keys in a single row
    keymap_num_keys = num_keys;
}
#elif defined(AGNOSTIC_USE_2D_ARRAY)
// Convert a 2D keymap to a 1D array of platform_keycode_t pointers
// This is used to convert the keymap from the framework format to our internal format
// The conversion is done by copying the values from the 2D array to the 1D array
// This is necessary because our internal representation uses a 1D array of pointers to the keycodes
// The conversion is done in a separate function to keep the code clean and modular
// The function returns a pointer to the 1D array of platform_keycode_t pointers
// The caller is responsible for freeing the memory allocated for the 1D array
// If the conversion fails, the function returns NULL
// The function assumes that the input keymap is valid and has the correct number of layers, rows, and columns
// The function does not check for memory allocation errors, so the caller should ensure that there is enough memory available
// The function does not modify the input keymap, so it can be used with read-only keymaps
// The function does not return any error codes, so the caller should check for NULL return value to detect errors
// The function is used in the platform_layout_init_qmk_keymap_impl and platform_layout_init_2d_keymap_impl functions
// The function is defined as static to limit its visibility to this file
// The function is not part of the public API, so it does not need to be declared in the header file

static platform_keycode_t** convert_keymap_impl(const void* layers_ptr, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    uint32_t layer_size = rows * cols;
    platform_keycode_t** layouts_1d = malloc(num_layers * sizeof(platform_keycode_t*));

    if (!layouts_1d) return NULL;

    for (uint8_t layer = 0; layer < num_layers; layer++) {
        layouts_1d[layer] = malloc(layer_size * sizeof(platform_keycode_t));
        if (!layouts_1d[layer]) {
            return NULL;
        }

        const uint16_t* src = (const uint16_t*)layers_ptr + (layer * layer_size);
        for (uint32_t i = 0; i < layer_size; i++) {
            layouts_1d[layer][i] = (platform_keycode_t)src[i];
        }
    }

    return layouts_1d;
}

// Initialize the keymap and layers for 2D array layouts
// This function allocates memory for the manager and initializes the keymap and layers
// It is called by platform_layout_init_qmk_keymap_impl and platform_layout_init_2d_keymap_impl
// The function takes a pointer to the layers, number of layers, rows, and columns as parameters
// The function returns nothing, but it modifies the global manager and key_map variables
// The function assumes that the input parameters are valid and does not check for errors
// The function is defined as static to limit its visibility to this file
static void init_2D_map_and_layers(const uint16_t* layers, uint8_t num_layers, uint8_t rows, uint8_t cols) {

    platform_keycode_t** layouts_1d = convert_keymap_impl((const void*)layers, num_layers, rows, cols);

    // Allocate the manager structure
    manager = (custom_layout_t*) malloc(sizeof(custom_layout_t));
    if (!manager) {
        return;
    }

    // Set basic properties
    manager->num_layers = num_layers;
    manager->num_positions = rows * cols;
    manager->current_layer = 0; // Initialize to the first layer

    // Allocate array of pointers to layouts
    manager->layouts = (platform_keycode_t **)malloc(sizeof(platform_keycode_t*) * num_layers);
    if (!manager->layouts) {
        return;
    }

    // Reference the external layouts (no copying)
    for (uint8_t i = 0; i < num_layers; i++) {
        manager->layouts[i] = layouts_1d[i];
    }

    keymap_rows = rows;
    keymap_cols = cols;
    keymap_num_keys = rows * cols;
}
#endif

#if defined (AGNOSTIC_USE_1D_ARRAY)

void platform_layout_init_1d_keymap_impl(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
    init_1D_map_and_layers(layers, num_layers, key_map, num_keys);
}

#elif defined (AGNOSTIC_USE_2D_ARRAY)

void platform_layout_init_2d_keymap_impl(const uint16_t* layers, uint8_t num_layers, uint8_t rows, uint8_t cols) {
    init_2D_map_and_layers(layers, num_layers, rows, cols);
}

#endif

#if defined(FRAMEWORK_QMK)

void platform_layout_init_qmk_keymap_impl(const uint16_t layers[][MATRIX_ROWS][MATRIX_COLS], uint8_t num_layers) {
    platform_layout_init_2d_keymap_impl((const uint16_t*)layers, num_layers, MATRIX_ROWS, MATRIX_COLS);
}

#elif defined(FRAMEWORK_ZMK)

void platform_layout_init_zmk_keymap_impl(platform_keycode_t **layers, uint8_t num_layers, matrix_pos_t* key_map, uint16_t num_keys) {
    platform_layout_init_1d_keymap_impl(layers, num_layers, key_map, num_keys);
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

#if defined(AGNOSTIC_USE_1D_ARRAY)
platform_keycode_t platform_layout_get_keycode_impl(platform_keypos_t position) {
    if (!manager || position >= manager->num_positions) {
        return 0; // or some "no key" value
    }
    return manager->layouts[manager->current_layer][position];
}

platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, platform_keypos_t position) {
    if (!manager || layer >= manager->num_layers || position >= manager->num_positions) {
        return 0;
    }
    return manager->layouts[layer][position];
}
#elif defined(AGNOSTIC_USE_2D_ARRAY)
platform_keycode_t platform_layout_get_keycode_impl(platform_keypos_t position) {
    if (!manager || position.row >= keymap_rows || position.col >= keymap_cols) {
        return 0; // or some "no key" value
    }
    return manager->layouts[manager->current_layer][position.row * keymap_cols + position.col];
}

platform_keycode_t platform_layout_get_keycode_from_layer_impl(uint8_t layer, platform_keypos_t position) {
    if (!manager || layer >= manager->num_layers || position.row >= keymap_rows || position.col >= keymap_cols) {
        return 0;
    }
    return manager->layouts[layer][position.row * keymap_cols + position.col];
}
#endif

