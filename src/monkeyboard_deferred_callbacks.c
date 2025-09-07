#include "monkeyboard_deferred_callbacks.h"
#include "monkeyboard_time_manager.h"
#include <stdint.h>
#include <string.h>
#include "platform_interface.h"

// Static storage for callback entries
static deferred_callback_entry_t callback_queue[MAX_DEFERRED_CALLBACKS];
static uint32_t next_add_order = 0;
static deferred_token_t next_token = 1; // Start at 1, 0 is invalid

// Helper function to find an empty slot
static int8_t find_empty_slot(void) {
    for (int8_t i = 0; i < MAX_DEFERRED_CALLBACKS; i++) {
        if (!callback_queue[i].active) {
            return i;
        }
    }
    return -1; // No empty slots
}

// Helper function to find a callback by token
static int8_t find_callback_by_token(deferred_token_t token) {
    if (token == DEFERRED_INVALID_TOKEN) return -1;

    for (int8_t i = 0; i < MAX_DEFERRED_CALLBACKS; i++) {
        if (callback_queue[i].active && callback_queue[i].token == token) {
            return i;
        }
    }
    return -1; // Token not found
}

// Helper function to sort callbacks by execution time, then by add order
static void sort_callbacks(void) {
    // Simple bubble sort - efficient enough for small arrays
    for (int i = 0; i < MAX_DEFERRED_CALLBACKS - 1; i++) {
        for (int j = 0; j < MAX_DEFERRED_CALLBACKS - 1 - i; j++) {
            if (!callback_queue[j].active) continue;
            if (!callback_queue[j + 1].active) continue;

            // Sort by execute_time first, then by add_order for stable sorting
            bool should_swap = false;
            if (callback_queue[j].execute_time > callback_queue[j + 1].execute_time) {
                should_swap = true;
            } else if (callback_queue[j].execute_time == callback_queue[j + 1].execute_time &&
                      callback_queue[j].add_order > callback_queue[j + 1].add_order) {
                should_swap = true;
            }

            if (should_swap) {
                deferred_callback_entry_t temp = callback_queue[j];
                callback_queue[j] = callback_queue[j + 1];
                callback_queue[j + 1] = temp;
            }
        }
    }
}

// Schedule a callback to be executed after delay_ms milliseconds
// Returns a token that can be used to cancel the callback, or DEFERRED_INVALID_TOKEN on failure
deferred_token_t schedule_deferred_callback(uint32_t delay_ms, deferred_callback_t callback, void *context) {
    if (!callback) return DEFERRED_INVALID_TOKEN;

    int8_t slot = find_empty_slot();
    if (slot < 0) return DEFERRED_INVALID_TOKEN; // Queue is full

    uint32_t current_time = monkeyboard_get_time_32();
    deferred_token_t token = next_token++;

    // Handle token wraparound (skip 0 as it's invalid)
    if (next_token == DEFERRED_INVALID_TOKEN) {
        next_token = 1;
    }

    callback_queue[slot].callback = callback;
    callback_queue[slot].context = context;
    callback_queue[slot].execute_time = current_time + delay_ms;
    callback_queue[slot].add_order = next_add_order++;
    callback_queue[slot].token = token;
    callback_queue[slot].active = true;

    // Keep the queue sorted
    sort_callbacks();

    return token;
}

// Cancel a scheduled callback by token
// Returns true if the callback was found and cancelled, false otherwise
bool cancel_deferred_callback(deferred_token_t token) {
    int8_t slot = find_callback_by_token(token);
    if (slot < 0) return false; // Token not found

    // Mark slot as inactive
    callback_queue[slot].active = false;

    // Clear the callback data for safety
    callback_queue[slot].callback = NULL;
    callback_queue[slot].context = NULL;
    callback_queue[slot].execute_time = 0;
    callback_queue[slot].add_order = 0;
    callback_queue[slot].token = DEFERRED_INVALID_TOKEN;

    return true;
}

// Execute the next pending callback immediately
void execute_callback(deferred_callback_entry_t *callback) {
    if (callback != NULL) {
        callback->callback(callback->context);
        // Mark slot as inactive
        callback->active = false;

        // Clear the callback data for safety
        callback->callback = NULL;
        callback->context = NULL;
        callback->execute_time = 0;
        callback->add_order = 0;
        callback->token = DEFERRED_INVALID_TOKEN;
    }
}

// Main task function - call this from housekeeping_task_user() or work queue
void execute_deferred_executions(void) {
    uint32_t current_time = monkeyboard_get_time_32();

    // Process callbacks in order (queue is kept sorted)
    for (int8_t i = 0; i < MAX_DEFERRED_CALLBACKS; i++) {
        if (!callback_queue[i].active) continue;

        // Check if this callback is due
        // Handle timer wraparound by checking if difference is in valid range
        if (time_is_after_or_equal(current_time, callback_queue[i].execute_time)) {
            // Execute the callback with its context
            execute_callback(&callback_queue[i]);
        } else {
            // Since the queue is sorted by time, we can break early
            // if this callback isn't due yet
            break;
        }
    }
}

// Get the first pending callback that is due to execute
deferred_callback_entry_t *get_next_deferred_callback(uint32_t current_time) {

    for (int8_t i = 0; i < MAX_DEFERRED_CALLBACKS; i++) {
        if (callback_queue[i].active && time_is_after_or_equal(current_time, callback_queue[i].execute_time)) {
            return &callback_queue[i];
        }
    }
    return NULL;
}



// Clear all pending callbacks
void clear_all_deferred_callbacks(void) {
    memset(callback_queue, 0, sizeof(callback_queue));
    // Reset token counter but keep it valid
    next_token = 1;
}

// Get the number of pending callbacks
uint8_t get_pending_callback_count(void) {
    uint8_t count = 0;
    for (int8_t i = 0; i < MAX_DEFERRED_CALLBACKS; i++) {
        if (callback_queue[i].active) {
            count++;
        }
    }
    return count;
}
