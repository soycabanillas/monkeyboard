#pragma once

#include <stddef.h>
#include "platform_interface.h"

typedef struct {
    platform_keypos_t key;
    platform_keycode_t keycode;
    uint8_t layer;
    bool release_on_buffer; //This flag marks that the key has been released, but the key is not erased until the buffer is processed. This way, info like the time the key was pressed, or the keycode can be obtained.
    platform_time_t time;
} only_press_buffer_item_t;

typedef struct {
    platform_keypos_t key;
    platform_keycode_t keycode;
    uint8_t layer;
    bool is_press;
    platform_time_t time;
    // The following fields are used to track the pipeline that created the keyboard event
    bool is_from_pipeline; // Indicates if the key event is from a pipeline
    size_t pipeline_index; // The index of the pipeline that created the keyboard event
} press_buffer_item_t;

#define ONLY_PRESS_BUFFER_MAX 5


#define PRESS_BUFFER_MAX 10

typedef struct {
    only_press_buffer_item_t only_press_buffer[ONLY_PRESS_BUFFER_MAX];
    uint8_t only_press_buffer_pos;
    press_buffer_item_t press_buffer[PRESS_BUFFER_MAX];
    uint8_t press_buffer_pos;
} key_buffer_t;

key_buffer_t* pipeline_key_buffer_create(void);
void pipeline_key_buffer_destroy(key_buffer_t* key_buffer);

bool key_buffer_is_pressed(key_buffer_t *key_buffer, platform_keycode_t keycode);
bool add_to_press_buffer(key_buffer_t *key_buffer, platform_keycode_t keycode, platform_keypos_t key, platform_time_t time, uint8_t layer, bool is_press, bool is_from_pipeline, size_t pipeline_index);
void remove_from_press_buffer(key_buffer_t *key_buffer, uint8_t pos);


