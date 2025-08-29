#include "monkeyboard_debug.h"
#include "key_event_buffer.h"
#include "key_press_buffer.h"
#include "platform_interface.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// This function is used to get a unique keypress ID for each key event
// It ensures that the ID is always between 1 and 255, wrapping around if necessary
// This is useful for identifying key events in the buffer
static uint8_t get_keypress_id(platform_key_event_buffer_t* event_buffer, platform_key_press_buffer_t* press_buffer) {
    static uint8_t keypress_id = 0;

    // Find a unique keypress ID that is not already in use

    // Check the press buffer for the same keypress ID
    // A press can exist without an event buffer when the event press has been processed
    // A press can not exist at the same time than its event release.
    bool already_in_press_buffer;
    do {
        already_in_press_buffer = false;
        keypress_id = (keypress_id % 255) + 1;

        uint8_t press_buffer_pos = press_buffer->press_buffer_pos;
        uint8_t i;
        for (i = 0; i < press_buffer_pos; i++) {
            uint8_t index = press_buffer_pos - 1 - i;
            if (press_buffer->press_buffer[index].press_id == keypress_id) {
                already_in_press_buffer = true;
                break;
            }
        }
    } while (already_in_press_buffer);

    // Check the event buffer for the same keypress ID
    bool already_in_event_buffer;
    do {
        already_in_event_buffer = false;

        uint8_t event_buffer_pos = event_buffer->event_buffer_pos;
        uint8_t i;
        for (i = 0; i < event_buffer_pos; i++) {
            uint8_t index = event_buffer_pos - 1 - i;
            if (event_buffer->event_buffer[index].press_id == keypress_id) {
                already_in_event_buffer = true;
                break;
            }
        }

        if (already_in_event_buffer) {
            keypress_id = (keypress_id % 255) + 1; // Wrap around if necessary
        }
    } while (already_in_event_buffer);

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

static bool platform_key_event_add_event_internal(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos, platform_keycode_t keycode, bool is_press, uint8_t press_id, bool* buffer_full) {
    if (event_buffer->event_buffer_pos >= PLATFORM_KEY_EVENT_MAX_ELEMENTS) {
        *buffer_full = true;
        return false; // Buffer is full
    }
    // Add the key event to the buffer
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    uint8_t press_buffer_pos = event_buffer->event_buffer_pos;

    press_buffer[press_buffer_pos].keypos = keypos;
    press_buffer[press_buffer_pos].keycode = keycode;
    press_buffer[press_buffer_pos].is_press = is_press;
    press_buffer[press_buffer_pos].time = time;
    press_buffer[press_buffer_pos].press_id = press_id; // Assign a unique ID to the key event

    press_buffer_pos = press_buffer_pos + 1;
    event_buffer->event_buffer_pos = press_buffer_pos;
    return true;
}

uint8_t platform_key_event_add_physical_press(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos, bool* buffer_full) {
    uint8_t press_id = get_keypress_id(event_buffer, event_buffer->key_press_buffer);
    uint8_t layer = platform_layout_get_current_layer();
    platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, keypos);

    platform_key_press_key_press_t* key_press = platform_key_press_add_press(event_buffer->key_press_buffer, keypos, keycode, press_id);
    if (key_press == NULL) {
        return 0; // Failed to add press to key press buffer
    }
    bool press_added = platform_key_event_add_event_internal(event_buffer, time, keypos, keycode, true, press_id, buffer_full);
    if (!press_added) {
        #if defined(AGNOSTIC_USE_1D_ARRAY)
            DEBUG_PRINT_ERROR("Failed to add press event for keypos: %d", keypos);
        #elif defined(AGNOSTIC_USE_2D_ARRAY)
            DEBUG_PRINT_ERROR("Failed to add press event for keypos: %d, %d", keypos.row, keypos.col);
        #endif
        platform_key_press_remove_press(event_buffer->key_press_buffer, keypos); // Clean up if event could not be added
        return 0; // Failed to add press to event buffer
    }
    return (press_added ? press_id : 0);
}

bool platform_key_event_add_physical_release(platform_key_event_buffer_t *event_buffer, platform_time_t time, platform_keypos_t keypos, bool* buffer_full) {
    platform_key_press_buffer_t *key_press_buffer = event_buffer->key_press_buffer;
    if (key_press_buffer == NULL) {
        DEBUG_PRINT_ERROR("Key press buffer is NULL");
        return false; // Key press buffer is not initialized
    }
    platform_key_press_key_press_t* key_press = platform_key_press_get_press_from_keypos(key_press_buffer, keypos);
    if (key_press == NULL) {
        #if defined(AGNOSTIC_USE_1D_ARRAY)
            DEBUG_PRINT_ERROR("Key press not found for keypos: %d", keypos);
        #elif defined(AGNOSTIC_USE_2D_ARRAY)
            DEBUG_PRINT_ERROR("Key press not found for keypos: %d, %d", keypos.row, keypos.col);
        #endif
        return false; // Press ID not found
    }
    if (key_press->ignore_release) {
        platform_key_press_remove_press(key_press_buffer, key_press->keypos);
        return false; // Ignore the release event
    }
    platform_keycode_t keycode = key_press->keycode; // Use the keycode from the key press;
    bool release_added = platform_key_event_add_event_internal(event_buffer, time, key_press->keypos, keycode, false, key_press->press_id, buffer_full);
    if (!release_added) {
        #if defined(AGNOSTIC_USE_1D_ARRAY)
            DEBUG_PRINT_ERROR("Failed to add release event for keypos: %d", keypos);
        #elif defined(AGNOSTIC_USE_2D_ARRAY)
            DEBUG_PRINT_ERROR("Failed to add release event for keypos: %d, %d", keypos.row, keypos.col);
        #endif
        platform_key_press_remove_press(event_buffer->key_press_buffer, key_press->keypos);
        return false;
    } else {
        platform_key_press_remove_press(event_buffer->key_press_buffer, key_press->keypos);
    }
    return true;
}

static bool try_get_position_by_press_id(platform_key_event_buffer_t *event_buffer, uint8_t press_id, bool is_press, uint8_t* position) {
    uint8_t event_buffer_pos = event_buffer->event_buffer_pos;
    uint8_t i;
    for (i = 0; i < event_buffer_pos; i++) {
        uint8_t index = event_buffer_pos - 1 - i;
        if (event_buffer->event_buffer[index].press_id == press_id && event_buffer->event_buffer[index].is_press == is_press) {
            *position = index;
            return true;
        }
    }
    return false;
}

static bool try_get_press_position_by_press_id(platform_key_event_buffer_t *event_buffer, uint8_t press_id, uint8_t* position) {
    return try_get_position_by_press_id(event_buffer, press_id, true, position);
}

static bool try_get_release_position_by_press_id(platform_key_event_buffer_t *event_buffer, uint8_t press_id, uint8_t* position) {
    return try_get_position_by_press_id(event_buffer, press_id, false, position);
}

void internal_platform_key_event_remove_event(platform_key_event_buffer_t *event_buffer, uint8_t position) {
    if (position < event_buffer->event_buffer_pos - 1) {
        memcpy(&event_buffer->event_buffer[position], &event_buffer->event_buffer[position + 1], sizeof(platform_key_event_t) * (event_buffer->event_buffer_pos - 1 - position));
    }
    event_buffer->event_buffer_pos--;
}

platform_key_event_position_t platform_key_event_remove_physical_press_by_press_id(platform_key_event_buffer_t *event_buffer, uint8_t press_id) {
    uint8_t position;
    if (try_get_press_position_by_press_id(event_buffer, press_id, &position)) {
        internal_platform_key_event_remove_event(event_buffer, position);
        return (platform_key_event_position_t){ .position = position, .found = true };
    }
    return (platform_key_event_position_t){ .found = false };
}

platform_key_event_position_t platform_key_event_remove_physical_release_by_press_id(platform_key_event_buffer_t *event_buffer, uint8_t press_id) {
    uint8_t position;
    if (try_get_release_position_by_press_id(event_buffer, press_id, &position)) {
        internal_platform_key_event_remove_event(event_buffer, position);
        return (platform_key_event_position_t){ .position = position, .found = true };
    } else {
        bool found = platform_key_press_ignore_release_by_press_id(event_buffer->key_press_buffer, press_id);
        return (platform_key_event_position_t){ .found = found };
    }
}

void platform_key_event_change_keycode(platform_key_event_buffer_t *event_buffer, uint8_t press_id, platform_keycode_t keycode) {
    platform_key_press_key_press_t* key_press = platform_key_press_get_press_from_press_id(event_buffer->key_press_buffer, press_id);
    if (key_press != NULL) {
        key_press->keycode = keycode; // Update the keycode in the key press buffer
    }
    bool press_found_on_event_buffer = false;
    for (uint8_t i = 0; i < event_buffer->event_buffer_pos; i++) {
        platform_key_event_t* event = &event_buffer->event_buffer[i];
        if (event->press_id == press_id) {
            if (event->is_press) {
                press_found_on_event_buffer = true;
                event->keycode = keycode; // Update the keycode for the event
            } else{
                if (press_found_on_event_buffer) {
                    event->keycode = keycode; // Update the keycode for the release event
                } else {
                    // Do nothing. The press has already been processed, so we must keep the release keycode the same as the press keycode
                }
            }
        }
    }
}

void platform_key_event_update_layer_for_physical_events(platform_key_event_buffer_t *event_buffer, uint8_t layer, uint8_t pos) {
    if (event_buffer == NULL) {
        return;
    }
    if (pos >= event_buffer->event_buffer_pos) {
        return; // Position out of bounds
    }
    for (uint8_t i = pos; i < event_buffer->event_buffer_pos; i++) {
        platform_keycode_t keycode = platform_layout_get_keycode_from_layer(layer, event_buffer->event_buffer[i].keypos);
        platform_key_event_change_keycode(event_buffer, event_buffer->event_buffer[i].press_id, keycode);
    }
}

#ifdef MONKEYBOARD_DEBUG
void print_key_event_buffer(platform_key_event_buffer_t *event_buffer) {
    if (event_buffer == NULL) {
        DEBUG_PRINT_ERROR("Key event buffer is NULL\n");
        return;
    }
    DEBUG_PRINT_RAW("EVENT: | %03hhu", event_buffer->event_buffer_pos);
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    for (size_t i = 0; i < event_buffer->event_buffer_pos; i++) {

        #if defined(AGNOSTIC_USE_1D_ARRAY)
            DEBUG_PRINT_RAW(" | %zu KP, K:%04u, P:%d, Id:%u, T:%04u",
               i, press_buffer[i].keypos,
               press_buffer[i].keycode, press_buffer[i].is_press, press_buffer[i].press_id, press_buffer[i].time);
        #elif defined(AGNOSTIC_USE_2D_ARRAY)
            DEBUG_PRINT_RAW(" | %zu R:%u, C:%u, K:%04u, P:%d, Id:%03u, T:%04u",
               i, press_buffer[i].keypos.row, press_buffer[i].keypos.col,
               press_buffer[i].keycode, press_buffer[i].is_press, press_buffer[i].press_id, press_buffer[i].time);
        #endif
    }
    DEBUG_PRINT_NL();
}
#endif
