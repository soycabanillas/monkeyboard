#include "monkeyboard_layer_manager.h"
#include "monkeyboard_debug.h"
#include "platform_interface.h"
#include "platform_types.h"
#include <stdint.h>

pipeline_tap_dance_nested_layers_t nested_layers;
uint8_t original_layer;

void layout_manager_initialize_nested_layers() {
    nested_layers.layer_total = 0;
    original_layer = 0;
}

void layout_manager_add_layer(platform_keypos_t keypos, uint8_t press_id, uint8_t layer) {
    DEBUG_PRINT(">>>>>>>>>>>>>>>>>>> Adding layer %d for key at (%d, %d) with press ID %d", layer, keypos.row, keypos.col, press_id);
    if (nested_layers.layer_total < MAX_NUM_NESTED_LAYERS) {
        nested_layers.layer[nested_layers.layer_total].keypos = keypos;
        nested_layers.layer[nested_layers.layer_total].press_id = press_id;
        nested_layers.layer[nested_layers.layer_total].layer = layer;
        nested_layers.layer_total++;
        platform_layout_set_layer(layer);
    }
}

void layout_manager_remove_layer_by_keypos(platform_keypos_t keypos) {
    DEBUG_PRINT(">>>>>>>>>>>>>>>>>>> Removing layer for key at (%d, %d)", keypos.row, keypos.col);
    for (uint8_t i = 0; i < nested_layers.layer_total; i++) {
        if (platform_compare_keyposition(nested_layers.layer[i].keypos, keypos)) {
            bool change_layer = false;
            uint8_t layer_to_change;
            if (nested_layers.layer_total == 1) {
                change_layer = true;
                layer_to_change = original_layer;
            } else if (i > 0 && i == nested_layers.layer_total - 1) {
                change_layer = true;
                layer_to_change = nested_layers.layer[i - 1].layer;
            }

            // Shift remaining layers down
            for (uint8_t j = i; j < nested_layers.layer_total - 1; j++) {
                nested_layers.layer[j] = nested_layers.layer[j + 1];
            }
            nested_layers.layer_total--;
            if (change_layer) {
                platform_layout_set_layer(layer_to_change);
            }
            break;
        }
    }
}

void layout_manager_set_absolute_layer(uint8_t layer) {
    original_layer = layer;
    nested_layers.layer_total = 0; // Reset nested layers
    platform_layout_set_layer(layer);
}
