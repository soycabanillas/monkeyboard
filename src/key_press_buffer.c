#include "monkeyboard_debug.h"
#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
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

platform_key_press_key_press_t* platform_key_press_add_press(platform_key_press_buffer_t *key_buffer, platform_keypos_t keypos, platform_keycode_t keycode, uint8_t press_id) {

    if (key_buffer == NULL) {
        return NULL;
    }
    platform_key_press_key_press_t* only_press_buffer = key_buffer->press_buffer;

    for (size_t pos = 0; pos < key_buffer->press_buffer_pos; pos++) {
        if (platform_compare_keyposition(only_press_buffer[pos].keypos, keypos)) {
            #if defined(AGNOSTIC_USE_1D_ARRAY)
                DEBUG_PRINT_ERROR("Failed to add physical press for keypos: %d, already exists", keypos);
            #elif defined(AGNOSTIC_USE_2D_ARRAY)
                DEBUG_PRINT_ERROR("Key Failed to add physical press for keypos: %d, %d, already exists", keypos.row, keypos.col);
            #endif
            return NULL;
        }
    }

    if (key_buffer->press_buffer_pos < PLATFORM_KEY_BUFFER_MAX_ELEMENTS) {
        only_press_buffer[key_buffer->press_buffer_pos].keypos = keypos;
        only_press_buffer[key_buffer->press_buffer_pos].press_id = press_id;
        only_press_buffer[key_buffer->press_buffer_pos].keycode = keycode;
        only_press_buffer[key_buffer->press_buffer_pos].ignore_release = false; // Default to not ignoring release
        ++key_buffer->press_buffer_pos;
        return &only_press_buffer[key_buffer->press_buffer_pos - 1]; // Return the newly added press
    }
    #if defined(AGNOSTIC_USE_1D_ARRAY)
        DEBUG_PRINT_ERROR("Failed to add physical press for keypos: %d, key press buffer is full", keypos);
    #elif defined(AGNOSTIC_USE_2D_ARRAY)
        DEBUG_PRINT_ERROR("Failed to add physical press for keypos: %d, %d, key press buffer is full", keypos.row, keypos.col);
    #endif
    return NULL;
}

// Removes a key press from the key buffer corresponding to the given key position.
// Returns true if the key was found and removed, false if the key was not found or the buffer is empty.
// Parameters:
//   key_buffer - pointer to the key press buffer structure
//   keypos     - key position to remove from the buffer (matches by key position, not keycode)
bool platform_key_press_remove_press(platform_key_press_buffer_t *key_buffer, platform_keypos_t keypos) {

    if (key_buffer == NULL) {
        return false;
    }
    platform_key_press_key_press_t* only_press_buffer = key_buffer->press_buffer;

    if (key_buffer->press_buffer_pos > 0){
        size_t pos = 0;
        for (pos = 0; pos < key_buffer->press_buffer_pos; pos++) {
            if (platform_compare_keyposition(only_press_buffer[pos].keypos, keypos)) {
                break;
            }
        }

        if (pos < key_buffer->press_buffer_pos) {
            size_t num_elements_to_shift = key_buffer->press_buffer_pos - pos - 1;
            if (num_elements_to_shift > 0) {
                memcpy(&only_press_buffer[pos], &only_press_buffer[pos + 1], sizeof(platform_key_press_key_press_t) * num_elements_to_shift);
            }
            // No memcpy needed if pos is the last element; just decrement the count.
            key_buffer->press_buffer_pos--;
            return true;
        }
    }
    return false;
}

platform_key_press_key_press_t* platform_key_press_get_press_from_keypos(platform_key_press_buffer_t *press_buffer, platform_keypos_t keypos) {
    if (press_buffer == NULL) {
        DEBUG_PRINT_ERROR("Key press buffer is NULL");
        return NULL;
    }
    platform_key_press_key_press_t* only_press_buffer = press_buffer->press_buffer;
    uint8_t only_press_buffer_pos = press_buffer->press_buffer_pos;
    for (size_t i = 0; i < only_press_buffer_pos; i++) {
        if (platform_compare_keyposition(only_press_buffer[i].keypos, keypos)) {
            return &only_press_buffer[i];
        }
    }
    #if defined(AGNOSTIC_USE_1D_ARRAY)
        DEBUG_PRINT_ERROR("Key press not found for keypos: %d", keypos);
    #elif defined(AGNOSTIC_USE_2D_ARRAY)
        DEBUG_PRINT_ERROR("Key press not found for keypos: %d, %d", keypos.row, keypos.col);
    #endif
    return NULL; // Key position not found
}

platform_key_press_key_press_t* platform_key_press_get_press_from_press_id(platform_key_press_buffer_t *press_buffer, uint8_t press_id) {
    if (press_buffer == NULL) {
        return NULL;
    }
    platform_key_press_key_press_t* only_press_buffer = press_buffer->press_buffer;
    uint8_t only_press_buffer_pos = press_buffer->press_buffer_pos;

    if (only_press_buffer_pos == 0) {
        DEBUG_PRINT("Key press buffer is empty, cannot find press ID %d", press_id);
        return NULL; // Buffer is empty
    }
    for (size_t i = 0; i < only_press_buffer_pos; i++) {
        if (only_press_buffer[i].press_id == press_id) {
            return &only_press_buffer[i];
        }
    }
    return NULL; // Press ID not found
}

bool platform_key_press_ignore_release_by_press_id(platform_key_press_buffer_t *press_buffer, uint8_t press_id) {
    if (press_buffer == NULL) {
        return false;
    }
    platform_key_press_key_press_t* only_press_buffer = press_buffer->press_buffer;
    uint8_t only_press_buffer_pos = press_buffer->press_buffer_pos;

    for (size_t i = 0; i < only_press_buffer_pos; i++) {
        if (only_press_buffer[i].press_id == press_id) {
            only_press_buffer[i].ignore_release = true;
            return true;
        }
    }

    return false;
}

#ifdef MONKEYBOARD_DEBUG
void print_key_press_buffer(platform_key_press_buffer_t *event_buffer) {
    if (event_buffer == NULL) {
        DEBUG_PRINT_ERROR("Key press buffer is NULL\n");
        return;
    }
    DEBUG_PRINT_RAW("PRESS: | %03hhu", event_buffer->press_buffer_pos);
    platform_key_press_key_press_t* press_buffer = event_buffer->press_buffer;
    for (size_t i = 0; i < event_buffer->press_buffer_pos; i++) {
        DEBUG_PRINT_RAW(" | %zu K:%04u, I:%d, Id:%03u",
               i, press_buffer[i].keycode,
               press_buffer[i].ignore_release, press_buffer[i].press_id);
    }
    DEBUG_PRINT_NL();
}
#endif
