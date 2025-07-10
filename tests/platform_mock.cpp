#include "../src/platform_interface.h"
#include <stdlib.h>
#include <stdio.h>

// Mock implementation of platform interface for testing

// Global state for mocking
static platform_time_t mock_timer = 0;
static uint8_t mock_current_layer = 0;
static platform_deferred_token mock_next_token = 1;

// Mock key operations
void platform_send_key(platform_keycode_t keycode) {
    printf("MOCK: Send key %u\n", keycode);
}

void platform_register_key(platform_keycode_t keycode) {
    printf("MOCK: Register key %u\n", keycode);
}

void platform_unregister_key(platform_keycode_t keycode) {
    printf("MOCK: Unregister key %u\n", keycode);
}

// Mock layer operations
void platform_layer_on(uint8_t layer) {
    printf("MOCK: Layer on %u\n", layer);
    mock_current_layer = layer;
}

void platform_layer_off(uint8_t layer) {
    printf("MOCK: Layer off %u\n", layer);
    if (mock_current_layer == layer) {
        mock_current_layer = 0;
    }
}

void platform_layer_set(uint8_t layer) {
    printf("MOCK: Layer set %u\n", layer);
    mock_current_layer = layer;
}

void platform_layer_select(uint8_t layer) {
    printf("MOCK: Layer select %u\n", layer);
    platform_layer_set(layer);
}

uint8_t platform_get_highest_layer(void) {
    return mock_current_layer;
}

// Mock time operations
platform_time_t platform_timer_read(void) {
    return mock_timer;
}

platform_time_t platform_timer_elapsed(platform_time_t last) {
    return mock_timer - last;
}

// Mock deferred execution
platform_deferred_token platform_defer_exec(uint32_t delay_ms, void (*callback)(void*), void* data) {
    printf("MOCK: Defer exec for %u ms\n", delay_ms);
    (void)callback;
    (void)data;
    return mock_next_token++;
}

bool platform_cancel_deferred_exec(platform_deferred_token token) {
    printf("MOCK: Cancel deferred exec token %u\n", token);
    return true;
}

// Mock memory operations
void* platform_malloc(size_t size) {
    return malloc(size);
}

void platform_free(void* ptr) {
    free(ptr);
}

// Test utilities
void mock_advance_timer(platform_time_t ms) {
    mock_timer += ms;
}

void mock_reset_timer(void) {
    mock_timer = 0;
}

void mock_set_layer(uint8_t layer) {
    mock_current_layer = layer;
}
