#pragma once

#include <stddef.h>
#include <stdint.h>
#include "key_event_buffer.h"
#include "key_virtual_buffer.h"
#include "platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIPELINE_CALLBACK_KEY_EVENT,
    PIPELINE_CALLBACK_TIMER
} pipeline_callback_type_t;

typedef struct {
    platform_time_t callback_time;
    bool capture_key_events;
    bool key_buffer_changed;
} capture_pipeline_t;

typedef struct {
    platform_key_event_t* key_event;
    pipeline_callback_type_t callback_type;
    uint16_t callback_time;
} pipeline_physical_callback_params_t;

typedef struct {
    platform_key_event_buffer_t* key_events;
    pipeline_callback_type_t callback_type;
    uint16_t callback_time;
} pipeline_virtual_callback_params_t;

typedef void (*key_buffer_tap)(platform_keycode_t keycode);
typedef void (*key_buffer_untap)(platform_keycode_t keycode);
typedef void (*key_buffer_key)(platform_keycode_t keycode);
typedef void (*key_buffer_remove_physical_press_and_release)(platform_keypos_t keypos);
typedef void (*key_buffer_update_layer_for_physical_events)(uint8_t layer, uint8_t pos);

typedef struct {
    key_buffer_tap register_key_fn;
    key_buffer_untap unregister_key_fn;
    key_buffer_key tap_key_fn;
    key_buffer_remove_physical_press_and_release remove_physical_press_and_release_fn;
    key_buffer_update_layer_for_physical_events update_layer_for_physical_events_fn;
} pipeline_actions_t;

typedef void (*pipeline_physical_callback)(pipeline_physical_callback_params_t*, pipeline_actions_t*, void*);
typedef void (*pipeline_virtual_callback)(pipeline_virtual_callback_params_t*, pipeline_actions_t*, void*);
typedef void (*pipeline_callback_reset)(void*);

typedef struct {
    pipeline_physical_callback callback;
    pipeline_callback_reset callback_reset;
    void* data;
} physical_pipeline_t;

typedef struct {
    pipeline_virtual_callback callback;
    pipeline_callback_reset callback_reset;
    void* data;
} virtual_pipeline_t;

typedef struct {
    platform_key_event_buffer_t *key_event_buffer;
    platform_virtual_event_buffer_t *virtual_event_buffer;
    capture_pipeline_t return_data;
    size_t pipeline_index; // Index of the current pipeline being executed
    platform_deferred_token deferred_exec_callback_token;
} pipeline_executor_state_t;

typedef struct {
    size_t physical_pipelines_length;
    size_t virtual_pipelines_length;
    physical_pipeline_t **physical_pipelines;
    virtual_pipeline_t **virtual_pipelines;
} pipeline_executor_config_t;

extern pipeline_executor_config_t *pipeline_executor_config;

void pipeline_executor_reset_state(void);
void pipeline_executor_create_config(uint8_t physical_pipeline_count, uint8_t virtual_pipeline_count);
void pipeline_executor_add_pipeline(uint8_t pipeline_position, pipeline_physical_callback callback, pipeline_callback_reset callback_reset, void* user_data);

void pipeline_executor_end_with_capture_next_keys_or_callback_on_timeout(platform_time_t callback_time);
void pipeline_executor_end_with_capture_next_keys(void);

bool pipeline_process_key(abskeyevent_t abskeyevent);

#ifdef __cplusplus
}
#endif
