#include "key_event_buffer.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

platform_key_event_buffer_t* platform_key_event_create(void) {
    platform_key_event_buffer_t* key_buffer = (platform_key_event_buffer_t*)malloc(sizeof(platform_key_event_buffer_t));
    if (key_buffer == NULL) {
        return NULL;
    }
    key_buffer->event_buffer_pos = 0;
    return key_buffer;
}

void platform_key_event_reset(platform_key_event_buffer_t* event_buffer) {
    if (event_buffer == NULL) {
        return;
    }
    event_buffer->event_buffer_pos = 0;
}

bool platform_key_event_add_event(platform_key_event_buffer_t *event_buffer, platform_time_t time, uint8_t layer, platform_keypos_t keypos, platform_keycode_t keycode, bool is_press) {
    #ifdef DEBUG
    if (event_buffer == NULL) {
        printf("Error: Event buffer is NULL");
        return false;
    }
    printf("Adding event: Time: %u, Layer: %u, Keypos: (%u, %u), Keycode: %u, Press: %d\n",
           time, layer, keypos.row, keypos.col, keycode, is_press);
    #endif
    if (event_buffer->event_buffer_pos >= PLATFORM_KEY_EVENT_MAX_ELEMENTS) {
        return false; // Buffer is full
    }
    // Add the key event to the buffer
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    uint8_t press_buffer_pos = event_buffer->event_buffer_pos;
    press_buffer[press_buffer_pos].keypos = keypos;
    press_buffer[press_buffer_pos].keycode = keycode;
    press_buffer[press_buffer_pos].layer = layer;
    press_buffer[press_buffer_pos].is_press = is_press;
    press_buffer[press_buffer_pos].time = time;
    press_buffer_pos = press_buffer_pos + 1;
    event_buffer->event_buffer_pos = press_buffer_pos; // Fix: Update the key_buffer structure
    return true;
}

void platform_key_event_remove_event(platform_key_event_buffer_t *event_buffer, uint8_t pos) {
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    uint8_t press_buffer_pos = event_buffer->event_buffer_pos;
    if (pos < press_buffer_pos - 1) {
        memcpy(&press_buffer[pos], &press_buffer[pos + 1], sizeof(platform_key_event_t) * (press_buffer_pos - 1 - pos));
    }
    press_buffer_pos = press_buffer_pos - 1;
    event_buffer->event_buffer_pos = press_buffer_pos; // Fix: Update the key_buffer structure
}

#ifdef DEBUG
void print_key_event_buffer(platform_key_event_buffer_t *event_buffer, size_t n_elements) {
    if (event_buffer == NULL) {
        return;
    }
    platform_key_event_t* press_buffer = event_buffer->event_buffer;
    for (size_t i = 0; i < n_elements && i < event_buffer->event_buffer_pos; i++) {
        printf("Event %zu: Keycode: %u, Layer: %u, Press: %d, Time: %u\n",
               i, press_buffer[i].keycode, press_buffer[i].layer,
               press_buffer[i].is_press, press_buffer[i].time);
    }
}
#endif
