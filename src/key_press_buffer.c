#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

platform_key_press_buffer_t* platform_key_press_create(void){
    platform_key_press_buffer_t* key_buffer = (platform_key_press_buffer_t*)malloc(sizeof(platform_key_press_buffer_t));
    if (key_buffer == NULL) {
        return NULL;
    }
    key_buffer->press_buffer_pos = 0;
    return key_buffer;
}

void platform_key_press_reset(platform_key_press_buffer_t* key_buffer) {
    if (key_buffer == NULL) {
        return;
    }
    key_buffer->press_buffer_pos = 0;
}


bool platform_key_press_keycode_is_pressed(platform_key_press_buffer_t *key_buffer, platform_keycode_t keycode){
    platform_key_press_key_press_t* only_press_buffer = key_buffer->press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->press_buffer_pos;

    for (size_t i = only_press_buffer_pos; i-- > 0;)
    {
        if (only_press_buffer[i].keycode == keycode) {
            return true;
        }
    }
    return false;
}

bool platform_key_press_keypos_is_pressed(platform_key_press_buffer_t *key_buffer, platform_keypos_t key) {
    platform_key_press_key_press_t* only_press_buffer = key_buffer->press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->press_buffer_pos;

    for (size_t i = only_press_buffer_pos; i-- > 0;)
    {
        if (platform_compare_keyposition(only_press_buffer[i].keypos, key)) {
            return true;
        }
    }
    return false;
}

bool platform_key_press_add_press(platform_key_press_buffer_t *key_buffer, platform_time_t time, uint8_t layer, platform_keypos_t key, bool is_press) {

    #ifdef DEBUG
    if (key_buffer == NULL) {
        printf("Error: Key buffer is NULL");
        return false;
    }
    printf("Adding press: Time: %u, Layer: %u, Keypos: (%u, %u), Press: %d\n",
           time, layer, key.row, key.col, is_press);
    #endif
    platform_key_press_key_press_t* only_press_buffer = key_buffer->press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->press_buffer_pos;

    platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, key);

    if (is_press == true) {
        if (only_press_buffer_pos < PLATFORM_KEY_BUFFER_MAX_ELEMENTS) {
            only_press_buffer[only_press_buffer_pos].keypos = key;
            only_press_buffer[only_press_buffer_pos].keycode = keycode;
            only_press_buffer[only_press_buffer_pos].time = time;
            only_press_buffer_pos = only_press_buffer_pos + 1;
            key_buffer->press_buffer_pos = only_press_buffer_pos;
        } else {
            return true; // The release is not added, but the press is still valid
        }
    }
    return false;
}

void platform_key_press_remove_press(platform_key_press_buffer_t *key_buffer, uint8_t pos) {

    platform_key_press_key_press_t* only_press_buffer = key_buffer->press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->press_buffer_pos;

    if (pos < (size_t)(only_press_buffer_pos - 1)) {
        memcpy(&only_press_buffer[pos], &only_press_buffer[pos + 1], sizeof(platform_key_press_key_press_t) * ((size_t)(only_press_buffer_pos - 1) -pos));
    }
    only_press_buffer_pos = only_press_buffer_pos - 1;
    key_buffer->press_buffer_pos = only_press_buffer_pos;
}
