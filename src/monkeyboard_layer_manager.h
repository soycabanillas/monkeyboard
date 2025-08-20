#pragma once

#include "platform_types.h"
#include <stdint.h>

typedef struct {
    platform_keypos_t keypos;
    uint8_t press_id;
    uint8_t layer;
} pipeline_tap_dance_layer_info_t;

#define MAX_NUM_NESTED_LAYERS 10 // Maximum number of events in the buffer

typedef struct {
    pipeline_tap_dance_layer_info_t layer[MAX_NUM_NESTED_LAYERS];
    uint8_t layer_total;
} pipeline_tap_dance_nested_layers_t;

void layout_manager_initialize_nested_layers(void);
void layout_manager_add_layer(platform_keypos_t keypos, uint8_t press_id, uint8_t layer);
void layout_manager_remove_layer_by_keypos(platform_keypos_t keypos);
void layout_manager_set_absolute_layer(uint8_t layer);
