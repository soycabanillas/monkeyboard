#include "key_virtual_buffer.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

platform_virtual_press_buffer_t* platform_virtual_press_create(void){
    platform_virtual_press_buffer_t* virtual_buffer = (platform_virtual_press_buffer_t*)malloc(sizeof(platform_virtual_press_buffer_t));
    if (virtual_buffer == NULL) {
        return NULL;
    }
    virtual_buffer->press_buffer_pos = 0;
    return virtual_buffer;
}

void platform_virtual_press_reset(platform_virtual_press_buffer_t* virtual_buffer) {
    if (virtual_buffer == NULL) {
        return;
    }
    virtual_buffer->press_buffer_pos = 0;
}

platform_virtual_press_virtual_press_t* platform_virtual_press_add_press(platform_virtual_press_buffer_t *virtual_buffer, platform_keycode_t keycode, uint8_t press_id) {

    if (virtual_buffer == NULL) {
        return NULL;
    }
    platform_virtual_press_virtual_press_t* only_press_buffer = virtual_buffer->press_buffer;

    for (size_t pos = 0; pos < virtual_buffer->press_buffer_pos; pos++) {
        if (only_press_buffer[pos].press_id == press_id) {
            return NULL;
        }
    }

    if (virtual_buffer->press_buffer_pos < PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS) {
        only_press_buffer[virtual_buffer->press_buffer_pos].keycode = keycode;
        only_press_buffer[virtual_buffer->press_buffer_pos].press_id = press_id;
        only_press_buffer[virtual_buffer->press_buffer_pos].ignore_release = false; // Default to not ignoring release
        ++virtual_buffer->press_buffer_pos;
        return &only_press_buffer[virtual_buffer->press_buffer_pos - 1]; // Return the newly added press
    }
    return NULL;
}

// Removes a key press from the key buffer corresponding to the given key position.
// Returns true if the key was found and removed, false if the key was not found or the buffer is empty.
// Parameters:
//   key_buffer - pointer to the key press buffer structure
//   keypos     - key position to remove from the buffer (matches by key position, not keycode)
bool platform_virtual_press_remove_press(platform_virtual_press_buffer_t *virtual_buffer, uint8_t press_id) {

    if (virtual_buffer == NULL) {
        return false;
    }
    platform_virtual_press_virtual_press_t* only_press_buffer = virtual_buffer->press_buffer;

    if (virtual_buffer->press_buffer_pos > 0){
        size_t pos = 0;
        for (pos = 0; pos < virtual_buffer->press_buffer_pos; pos++) {
            if (only_press_buffer[pos].press_id == press_id) {
                break;
            }
        }

        if (pos < virtual_buffer->press_buffer_pos) {
            size_t num_elements_to_shift = virtual_buffer->press_buffer_pos - pos - 1;
            if (num_elements_to_shift > 0) {
                memcpy(&only_press_buffer[pos], &only_press_buffer[pos + 1], sizeof(platform_virtual_press_virtual_press_t) * num_elements_to_shift);
            }
            // No memcpy needed if pos is the last element; just decrement the count.
            virtual_buffer->press_buffer_pos--;
            return true;
        }
    }
    return false;
}

platform_virtual_press_virtual_press_t* platform_virtual_press_get_press_from_press_id(platform_virtual_press_buffer_t *virtual_buffer, uint8_t press_id) {
    if (virtual_buffer == NULL) {
        return NULL;
    }
    platform_virtual_press_virtual_press_t* only_press_buffer = virtual_buffer->press_buffer;
    uint8_t only_press_buffer_pos = virtual_buffer->press_buffer_pos;

    for (size_t i = 0; i < only_press_buffer_pos; i++) {
        if (only_press_buffer[i].press_id == press_id) {
            return &only_press_buffer[i];
        }
    }
    return NULL; // Press ID not found
}

bool platform_virtual_press_ignore_release(platform_virtual_press_buffer_t *virtual_buffer, uint8_t press_id) {
    if (virtual_buffer == NULL) {
        return false;
    }
    platform_virtual_press_virtual_press_t* only_press_buffer = virtual_buffer->press_buffer;
    uint8_t only_press_buffer_pos = virtual_buffer->press_buffer_pos;

    for (size_t i = 0; i < only_press_buffer_pos; i++) {
        if (only_press_buffer[i].press_id == press_id) {
            only_press_buffer[i].ignore_release = true;
            return true;
        }
    }

    return false;
}
