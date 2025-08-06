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

typedef enum {
    PIPELINE_EXECUTOR_TIMEOUT_NONE,
    PIPELINE_EXECUTOR_TIMEOUT_NEW,
    PIPELINE_EXECUTOR_TIMEOUT_PREVIOUS
} pipeline_executor_timer_behavior_t;

typedef struct {
    pipeline_executor_timer_behavior_t timer_behavior;
    platform_time_t callback_time;
    bool capture_key_events;
} capture_pipeline_t;

typedef struct {
    pipeline_callback_type_t callback_type;
    platform_key_event_t* key_event; // Only used for PIPELINE_CALLBACK_KEY_EVENT
    platform_time_t callback_time; // Only used for PIPELINE_CALLBACK_TIMER
    bool is_capturing_keys; // Indicates if the pipeline is capturing key events
} pipeline_physical_callback_params_t;

typedef struct {
    platform_virtual_event_buffer_t* key_events;
} pipeline_virtual_callback_params_t;

typedef void (*key_buffer_tap)(platform_keycode_t keycode);
typedef void (*key_buffer_untap)(platform_keycode_t keycode);
typedef void (*key_buffer_key)(platform_keycode_t keycode);
typedef uint8_t (*key_buffer_get_physical_key_event_count)(void);
typedef platform_key_event_t* (*key_buffer_get_physical_key_event)(uint8_t index);
typedef void (*key_buffer_remove_physical_press)(uint8_t press_id);
typedef void (*key_buffer_remove_physical_release)(uint8_t press_id);
typedef void (*key_buffer_remove_physical_tap)(uint8_t press_id);
typedef void (*key_buffer_change_key_code)(uint8_t pos, platform_keycode_t keycode);

typedef struct {
    key_buffer_tap register_key_fn;
    key_buffer_untap unregister_key_fn;
    key_buffer_key tap_key_fn;
    key_buffer_get_physical_key_event_count get_physical_key_event_count_fn;
    key_buffer_get_physical_key_event get_physical_key_event_fn;
    key_buffer_remove_physical_press remove_physical_press_fn;
    key_buffer_remove_physical_release remove_physical_release_fn;
    key_buffer_remove_physical_tap remove_physical_tap_fn;
    key_buffer_change_key_code change_key_code_fn;
} pipeline_physical_actions_t;

typedef struct {
    key_buffer_tap register_key_fn;
    key_buffer_untap unregister_key_fn;
    key_buffer_key tap_key_fn;
} pipeline_virtual_actions_t;

typedef void (*pipeline_executor_return_key_capture)(pipeline_executor_timer_behavior_t timer_behavior, platform_time_t time);
typedef void (*pipeline_executor_return_no_capture)(void);

typedef struct {
    pipeline_executor_return_key_capture key_capture_fn;
    pipeline_executor_return_no_capture no_capture_fn;
} pipeline_physical_return_actions_t;

typedef void (*pipeline_physical_callback)(pipeline_physical_callback_params_t*, pipeline_physical_actions_t*, pipeline_physical_return_actions_t*, void*);
typedef void (*pipeline_virtual_callback)(pipeline_virtual_callback_params_t*, pipeline_virtual_actions_t*, void*);
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
    size_t physical_pipeline_index; // Index of the current pipeline being executed
    platform_deferred_token deferred_exec_callback_token;
    bool is_callback_set; // Indicates if a callback is set for deferred execution
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
void pipeline_executor_add_physical_pipeline(uint8_t pipeline_position, pipeline_physical_callback callback, pipeline_callback_reset callback_reset, void* user_data);
void pipeline_executor_add_virtual_pipeline(uint8_t pipeline_position, pipeline_virtual_callback callback, pipeline_callback_reset callback_reset, void* user_data);

void pipeline_process_key(abskeyevent_t abskeyevent);

#ifdef __cplusplus
}
#endif
