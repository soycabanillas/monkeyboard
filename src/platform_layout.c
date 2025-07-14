#include "platform_layout.h"
#include <stdint.h>
#include <stdlib.h>
#include "platform_interface.h"

custom_layout_t* manager = NULL;

void platform_layout_init_impl(uint8_t num_layers, uint32_t num_positions, platform_keycode_t **external_layouts) {
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
