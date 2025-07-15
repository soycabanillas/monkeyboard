#include "key_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void print_press_buffers(int16_t num_keys) {
    uint8_t num_keys_to_print = num_keys;
    if (num_keys_to_print > ONLY_PRESS_BUFFER_MAX) num_keys_to_print = ONLY_PRESS_BUFFER_MAX;
    // platform_log_debug("-only -  pos: %2u || ", only_press_buffer_pos);
    for (size_t i = 0; i < num_keys_to_print; i++)
    {
        // platform_log_debug("col: %3u row: %2u key:0x%04X || ", only_press_buffer[i].key.col, only_press_buffer[i].key.row, only_press_buffer[i].keycode);
    }
    // platform_log_debug("");

    if (num_keys_to_print > PRESS_BUFFER_MAX) num_keys_to_print = PRESS_BUFFER_MAX;
    // platform_log_debug("-press-  pos: %2u || ", press_buffer_pos);
    for (size_t i = 0; i < num_keys_to_print; i++)
    {
        // platform_log_debug("col: %3u row: %2u key:0x%04X || ", press_buffer[i].key.col, press_buffer[i].key.row, press_buffer[i].keycode);
    }
    // platform_log_debug("");
}

key_buffer_t* pipeline_key_buffer_create(void){
    key_buffer_t* key_buffer = (key_buffer_t*)malloc(sizeof(key_buffer_t));
    if (key_buffer == NULL) {
        return NULL;
    }
    key_buffer->only_press_buffer_pos = 0;
    key_buffer->press_buffer_pos = 0;
    return key_buffer;
}

void pipeline_key_buffer_destroy(key_buffer_t* key_buffer) {
    if (key_buffer == NULL) {
        return;
    }
    free(key_buffer);
}

key_buffer_t* key_buffer_create(void){
    key_buffer_t* key_buffer = (key_buffer_t*)malloc(sizeof(key_buffer_t));
    if (key_buffer == NULL) {
        return NULL;
    }
    key_buffer->only_press_buffer_pos = 0;
    key_buffer->press_buffer_pos = 0;
    return key_buffer;
}


bool platform_keycode_is_pressed(key_buffer_t *key_buffer, platform_keycode_t keycode){
    only_press_buffer_item_t* only_press_buffer = key_buffer->only_press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->only_press_buffer_pos;

    for (size_t i = only_press_buffer_pos; i-- > 0;)
    {
        if (only_press_buffer[i].keycode == keycode) {
            return true;
        }
    }
    return false;
}

bool platform_keypos_is_pressed(key_buffer_t *key_buffer, platform_keypos_t key) {
    only_press_buffer_item_t* only_press_buffer = key_buffer->only_press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->only_press_buffer_pos;

    for (size_t i = only_press_buffer_pos; i-- > 0;)
    {
        if (platform_compare_keyposition(only_press_buffer[i].key, key)) {
            return true;
        }
    }
    return false;
}

// Add the key press or release to the press_buffer buffer.
// Add the key to the only_press_buffer when is a pressed or update the key to set the key as released.
// This buffers can be consumed after several key presses and releases. The same key (position) can be pressed and released and stored on this buffers. This is needed for example if a pipeline requires the information of repetition of a key.
bool add_to_press_buffer(key_buffer_t *key_buffer, platform_keypos_t key, platform_time_t time, uint8_t layer, bool is_press) {
    // Checks the available space in press_buffer.
    // If there is a press, ensure that the buffer has enough space to store both the current key press and a future key release. Otherwise, if the buffer becomes full, releasing keys will be impossible.
    // This only applies in basic situations, such as when too many keys are pressed simultaneously. If a pipeline fails to remove keys from the buffer, it can still become full.

    press_buffer_item_t* press_buffer = key_buffer->press_buffer;
    uint8_t press_buffer_pos = key_buffer->press_buffer_pos;
    only_press_buffer_item_t* only_press_buffer = key_buffer->only_press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->only_press_buffer_pos;

    platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, key);

    uint8_t untap_layer = 0; // This variable is used to store the layer of the key press that is being released. If the key is released, it will be the layer of the press that was buffered.
    if ((is_press == true && press_buffer_pos + 1 < PRESS_BUFFER_MAX) || (is_press == false && press_buffer_pos < PRESS_BUFFER_MAX)) {
        // Before adding the key to the press_buffer, make sure there is space available in only_press_buffer.
        if (is_press == true) {
            if (only_press_buffer_pos < ONLY_PRESS_BUFFER_MAX) {
                only_press_buffer[only_press_buffer_pos].key = key;
                only_press_buffer[only_press_buffer_pos].keycode = keycode;
                only_press_buffer[only_press_buffer_pos].layer = layer;
                only_press_buffer[only_press_buffer_pos].release_on_buffer = false;
                only_press_buffer[only_press_buffer_pos].time = time;
                only_press_buffer_pos = only_press_buffer_pos + 1;
                key_buffer->only_press_buffer_pos = only_press_buffer_pos; // Fix: Update the key_buffer structure
            } else {
                return false;
            }
        } else {
            //Check if the new release has a press already buffered
            bool key_is_in_buffer = false;
            for (size_t i = only_press_buffer_pos; i-- > 0;)
            {
                if (platform_compare_keyposition(only_press_buffer[i].key, key) && only_press_buffer[i].release_on_buffer == false) {
                    only_press_buffer[i].release_on_buffer = true;
                    key_is_in_buffer = true;
                    untap_layer = only_press_buffer[i].layer; // Use the layer from the press
                    break;
                }
            }
            if (key_is_in_buffer == false) return false;
        }
        press_buffer[press_buffer_pos].key = key;
        press_buffer[press_buffer_pos].keycode = keycode;
        press_buffer[press_buffer_pos].layer = untap_layer;
        press_buffer[press_buffer_pos].is_press = is_press;
        press_buffer[press_buffer_pos].time = time;
        press_buffer_pos = press_buffer_pos + 1;
        key_buffer->press_buffer_pos = press_buffer_pos; // Fix: Update the key_buffer structure
        return true;
    }
    return false;
}

void remove_from_press_buffer(key_buffer_t *key_buffer, uint8_t pos) {

    press_buffer_item_t* press_buffer = key_buffer->press_buffer;
    uint8_t press_buffer_pos = key_buffer->press_buffer_pos;
    only_press_buffer_item_t* only_press_buffer = key_buffer->only_press_buffer;
    uint8_t only_press_buffer_pos = key_buffer->only_press_buffer_pos;

    if (press_buffer[pos].is_press == false) {
        for (size_t i = only_press_buffer_pos; i-- > 0;)
        {
            if (platform_compare_keyposition(only_press_buffer[i].key, press_buffer[pos].key)) {
                if (i < (size_t)(only_press_buffer_pos - 1)) {
                    memcpy(&only_press_buffer[i], &only_press_buffer[i + 1], sizeof(only_press_buffer_item_t) * ((size_t)(only_press_buffer_pos - 1) - i));
                }
                only_press_buffer_pos = only_press_buffer_pos - 1;
                key_buffer->only_press_buffer_pos = only_press_buffer_pos; // Fix: Update the key_buffer structure
                break;
            }
        }
    }

    if (pos < press_buffer_pos - 1) {
        memcpy(&press_buffer[pos], &press_buffer[pos + 1], sizeof(press_buffer_item_t) * (press_buffer_pos - 1 - pos));
    }
    press_buffer_pos = press_buffer_pos - 1;
    key_buffer->press_buffer_pos = press_buffer_pos; // Fix: Update the key_buffer structure
}

// bool check_if_release_is_before_press(abskeypos_t key_release, abskeypos_t key_press) {
//     //It just checks the order of the keys (if they exists) on the only_press_buffer.
//     //It will return true for key_release="A" and key_press="B" if called at the end of the sequence "press A" -> "press B" -> "release A".
//     //In this case, the only_press_buffer will have ["A", "B"] when the function is called
//     //It will return true for key_release="A" and key_press="B" if called at the end of the sequence "press B" -> "press A" -> "release A" because on the only_press_buffer there will be ["A", "B"]
//     //In this case, the only_press_buffer will have ["B", "A"] when the function is called
//     for (size_t i = 0; i < only_press_buffer_pos; i++)
//     {
//         if (only_press_buffer[i].key.col == key_press.col && only_press_buffer[i].key.row == key_press.row) return false;
//         if (only_press_buffer[i].key.col == key_release.col && only_press_buffer[i].key.row == key_release.row) return true;
//     }
//     return false;
// }



//bool process_timer(void) {
//    execute_pipeline(true, i, 15, NULL);
//}




