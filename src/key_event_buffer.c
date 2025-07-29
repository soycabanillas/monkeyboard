#include "key_event_buffer.h"
#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

// This function is used to get a unique keypress ID for each key event
// It ensures that the ID is always between 1 and 255, wrapping around if necessary
// This is useful for identifying key events in the buffer
static uint8_t get_keypress_id(void) {
    static uint8_t keypress_id = 0;
    // Use 255 so the ID wraps from 1 to 255; 0 is reserved and never returned as a keypress ID
    keypress_id = (keypress_id % 255) + 1;
    return keypress_id;
}

platform_key_event_buffer_t* platform_key_event_create(void) {
    platform_key_event_buffer_t* key_buffer = (platform_key_event_buffer_t*)malloc(sizeof(platform_key_event_buffer_t));
    if (key_buffer == NULL) {
        return NULL;
    }
    key_buffer->event_buffer_pos = 0;
    key_buffer->key_press_buffer = platform_key_press_create();
    return key_buffer;
}

void platform_key_event_reset(platform_key_event_buffer_t* event_buffer) {
    if (event_buffer == NULL) {
        return;
    }
    event_buffer->event_buffer_pos = 0;
    platform_key_press_reset(event_buffer->key_press_buffer);
}

void platform_key_event_remove_event_keys(platform_key_event_buffer_t* event_buffer) {
    if (event_buffer == NULL) {
        return;
    }
    event_buffer->event_buffer_pos = 0;
}

static bool platform_key_event_add_event_internal(platform_key_event_buffer_t *event_buffer, platform_time_t time, uint8_t layer, platform_keypos_t keypos, platform_keycode_t keycode, bool is_press, uint8_t press_id, bool is_physical, bool* buffer_full) {
    if (event_buffer->event_buffer_pos >= PLATFORM_KEY_EVENT_MAX_ELEMENTS) {
        *buffer_full = true;
        return false; // Buffer is full
    }
    // Add the key event to the buffer
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    uint8_t press_buffer_pos = event_buffer->event_buffer_pos;

    press_buffer[press_buffer_pos].is_physical = is_physical;
    press_buffer[press_buffer_pos].keypos = keypos;
    press_buffer[press_buffer_pos].keycode = keycode;
    press_buffer[press_buffer_pos].layer = layer;
    press_buffer[press_buffer_pos].is_press = is_press;
    press_buffer[press_buffer_pos].time = time;
    press_buffer[press_buffer_pos].press_id = press_id; // Assign a unique ID to the key event

    press_buffer_pos = press_buffer_pos + 1;
    event_buffer->event_buffer_pos = press_buffer_pos;
    return true;
}

uint8_t platform_key_event_add_physical_press(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos, bool* buffer_full) {
    uint8_t press_id = get_keypress_id();
    uint8_t layer = platform_layout_get_current_layer();
    platform_key_press_key_press_t* key_press = platform_key_press_add_press(event_buffer->key_press_buffer, keypos, layer, press_id);
    if (key_press == NULL) {
        return 0; // Failed to add press to key press buffer
    }
    platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, keypos);
    bool press_added = platform_key_event_add_event_internal(event_buffer, time, layer, keypos, keycode, true, press_id, true, buffer_full);
    if (!press_added) {
        DEBUG_PRINT_ERROR("Failed to add press event for keypos: %d, %d", keypos.row, keypos.col);
        platform_key_press_remove_press(event_buffer->key_press_buffer, keypos); // Clean up if event could not be added
        return 0; // Failed to add press to event buffer
    }
    return (press_added ? press_id : 0);
}

bool platform_key_event_add_virtual_press(platform_key_event_buffer_t *event_buffer, platform_keycode_t keycode, bool* buffer_full) {
    bool press_added = platform_key_event_add_event_internal(event_buffer, 0, 0, dummy_keypos, keycode, true, 0, false, buffer_full);
    return press_added;
}

bool platform_key_event_add_physical_release(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos, bool* buffer_full) {
    platform_key_press_buffer_t *key_press_buffer = event_buffer->key_press_buffer;
    if (key_press_buffer == NULL) {
        DEBUG_PRINT_ERROR("Key press buffer is NULL");
        return false; // Key press buffer is not initialized
    }
    platform_key_press_key_press_t* key_press = platform_key_press_get_press_from_keypos(key_press_buffer, keypos);
    if (key_press == NULL) {
        DEBUG_PRINT_ERROR("Key press not found for keypos: %d, %d", keypos.row, keypos.col);
        return false; // Press ID not found
    }
    if (key_press->ignore_release) {
        platform_key_press_remove_press(key_press_buffer, key_press->keypos);
        return false; // Ignore the release event
    }
    platform_keycode_t keycode = platform_layout_get_keycode_from_layer(key_press->layer, key_press->keypos);
    bool press_added = platform_key_event_add_event_internal(event_buffer, time, key_press->layer, key_press->keypos, keycode, false, key_press->press_id, true, buffer_full);
    if (!press_added) {
        DEBUG_PRINT_ERROR("Failed to add release event for keypos: %d, %d", keypos.row, keypos.col);
        platform_key_press_remove_press(event_buffer->key_press_buffer, key_press->keypos);
        return false;
    } else {
        platform_key_press_remove_press(event_buffer->key_press_buffer, key_press->keypos);
    }
    return true;
}

bool platform_key_event_add_virtual_release(platform_key_event_buffer_t *event_buffer, platform_keycode_t keycode, bool* buffer_full) {
    bool press_added = platform_key_event_add_event_internal(event_buffer, 0, 0, dummy_keypos, keycode, false, 0, false, buffer_full);
    return press_added;
}

platform_key_event_remove_type_t platform_key_event_remove_physical_press_and_release(platform_key_event_buffer_t *event_buffer, platform_keypos_t keypos) {
    uint8_t event_buffer_pos = event_buffer->event_buffer_pos;
    uint8_t i;
    bool found_press = false;
    for (i = 0; i < event_buffer_pos; i++) {
        if (event_buffer->event_buffer[i].is_physical && platform_compare_keyposition(event_buffer->event_buffer[i].keypos, keypos)) {
            if (i < event_buffer_pos - 1) {
                memcpy(&event_buffer->event_buffer[i], &event_buffer->event_buffer[i + 1], sizeof(platform_key_event_t) * (event_buffer_pos - 1 - i));
            }
            event_buffer_pos--;
            event_buffer->event_buffer_pos = event_buffer_pos;
            found_press = true;
            break;
        }
    }
    if (found_press == false) {
        return PLATFORM_KEY_EVENT_TYPE_PHYSICAL_NONE_REMOVED; // No press found
    }
    bool found_release = false;
    for (uint8_t j = i; j < event_buffer_pos; j++) {
        if (event_buffer->event_buffer[j].is_physical && platform_compare_keyposition(event_buffer->event_buffer[j].keypos, keypos)) {
            if (j < event_buffer_pos - 1) {
                memcpy(&event_buffer->event_buffer[j], &event_buffer->event_buffer[j + 1], sizeof(platform_key_event_t) * (event_buffer_pos - 1 - j));
            }
            event_buffer_pos--;
            event_buffer->event_buffer_pos = event_buffer_pos;
            found_release = true;
            break;
        }
    }
    if (found_release == false) {
        platform_key_press_ignore_release(event_buffer->key_press_buffer, keypos);
        return PLATFORM_KEY_EVENT_TYPE_PHYSICAL_PRESS_REMOVED; // No release found
    }
    return PLATFORM_KEY_EVENT_TYPE_PHYSICAL_PRESS_AND_RELEASE_REMOVED; // Both press and release found and removed
}

void platform_key_event_update_layer_for_physical_events(platform_key_event_buffer_t *event_buffer, uint8_t layer, uint8_t pos) {
    if (event_buffer == NULL) {
        return;
    }
    for (uint8_t i = pos; i < event_buffer->event_buffer_pos; i++) {
        bool is_physical = event_buffer->event_buffer[i].is_physical;
        bool is_press = event_buffer->event_buffer[i].is_press;
        if (is_physical && is_press) {
            uint8_t press_id = event_buffer->event_buffer[i].press_id;
            // Update the layer for the press event
            event_buffer->event_buffer[i].layer = layer;
            event_buffer->event_buffer[i].keycode = platform_layout_get_keycode_from_layer(layer, event_buffer->event_buffer[i].keypos);
            // Update the layer for the release event
            for (uint8_t j = i + 1; j < event_buffer->event_buffer_pos; j++) {
                if (event_buffer->event_buffer[j].press_id == press_id && event_buffer->event_buffer[j].is_physical) {
                    if (event_buffer->event_buffer[j].is_press) {
                        break;
                    } else {
                        event_buffer->event_buffer[j].layer = layer;
                        event_buffer->event_buffer[j].keycode = event_buffer->event_buffer[i].keycode;
                        break;
                    }
                }
            }
        }
    }
}

#ifdef DEBUG
void print_key_event_buffer(platform_key_event_buffer_t *event_buffer) {
    if (event_buffer == NULL) {
        DEBUG_PRINT_ERROR("Key event buffer is NULL\n");
        return;
    }
    DEBUG_PRINT_RAW("| %03hhu", event_buffer->event_buffer_pos);
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    for (size_t i = 0; i < event_buffer->event_buffer_pos; i++) {
        DEBUG_PRINT_RAW(" | %zu R:%u, C:%u, K:%02u, L:%u, P:%d, Id:%u, T:%04u",
               i, press_buffer[i].keypos.row, press_buffer[i].keypos.col,
               press_buffer[i].keycode, press_buffer[i].layer, press_buffer[i].is_press, press_buffer[i].press_id, press_buffer[i].time);
    }
    DEBUG_PRINT_NL();
}
#endif
