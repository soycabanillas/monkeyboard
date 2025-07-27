#include "key_virtual_buffer.h"
#include "platform_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

platform_virtual_event_buffer_t* platform_virtual_event_create(void){
    platform_virtual_event_buffer_t* virtual_buffer = (platform_virtual_event_buffer_t*)malloc(sizeof(platform_virtual_event_buffer_t));
    if (virtual_buffer == NULL) {
        return NULL;
    }
    virtual_buffer->press_buffer_pos = 0;
    return virtual_buffer;
}

void platform_virtual_event_reset(platform_virtual_event_buffer_t* virtual_buffer) {
    if (virtual_buffer == NULL) {
        return;
    }
    virtual_buffer->press_buffer_pos = 0;
}

bool platform_virtual_event_add_press(platform_virtual_event_buffer_t *virtual_buffer, platform_keycode_t keycode) {

    if (virtual_buffer == NULL) {
        return false;
    }
    platform_virtual_buffer_virtual_event_t* only_press_buffer = virtual_buffer->press_buffer;

    if (virtual_buffer->press_buffer_pos < PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS) {
        only_press_buffer[virtual_buffer->press_buffer_pos].keycode = keycode;
        only_press_buffer[virtual_buffer->press_buffer_pos].is_press = true;
        ++virtual_buffer->press_buffer_pos;
        return true;
    }
    return false;
}

bool platform_virtual_event_add_release(platform_virtual_event_buffer_t *virtual_buffer, platform_keycode_t keycode) {

    if (virtual_buffer == NULL) {
        return false;
    }
    platform_virtual_buffer_virtual_event_t* only_press_buffer = virtual_buffer->press_buffer;

    if (virtual_buffer->press_buffer_pos < PLATFORM_KEY_VIRTUAL_BUFFER_MAX_ELEMENTS) {
        only_press_buffer[virtual_buffer->press_buffer_pos].keycode = keycode;
        only_press_buffer[virtual_buffer->press_buffer_pos].is_press = false;
        ++virtual_buffer->press_buffer_pos;
        return true;
    }
    return false;
}
