#pragma once

#ifndef DEFERRED_CALLBACKS_H
#define DEFERRED_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>

// Maximum number of deferred callbacks that can be queued
#ifndef MAX_DEFERRED_CALLBACKS
#define MAX_DEFERRED_CALLBACKS 16
#endif

// Token type for cancelling callbacks
typedef uint16_t deferred_token_t;
#define DEFERRED_INVALID_TOKEN 0

// Callback function type - takes one void* context parameter, returns void
typedef void (*deferred_callback_t)(void *context);

// Structure to hold callback information
typedef struct {
    deferred_callback_t callback;
    void               *context;       // Context passed to callback
    uint32_t            execute_time;
    uint32_t            add_order;      // For stable sorting when times are equal
    deferred_token_t    token;          // Unique token for this callback
    bool                active;
} deferred_callback_entry_t;

// Public API functions
deferred_token_t schedule_deferred_callback(uint32_t delay_ms, deferred_callback_t callback, void *context);
bool cancel_deferred_callback(deferred_token_t token);
void execute_deferred_executions(void);
void clear_all_deferred_callbacks(void);
uint8_t get_pending_callback_count(void);

#endif
