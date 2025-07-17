#pragma once

#include <stddef.h>
#include <stdint.h>
#include "key_event_buffer.h"
#include "key_press_buffer.h"
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
    bool captured;
    size_t pipeline_index; // Index of the pipeline that captured the callback
    bool key_event_buffer;
    platform_key_event_buffer_t *key_buffer;
} capture_pipeline_t;

typedef struct {
    platform_key_event_buffer_t* key_events;
    pipeline_callback_type_t callback_type;
    uint16_t callback_time;
} pipeline_callback_params_t;

typedef void (*key_buffer_tap)(platform_keycode_t keycode, platform_keypos_t keypos);
typedef void (*key_buffer_untap)(platform_keycode_t keycode);
typedef void (*key_buffer_key)(platform_keycode_t keycode, platform_keypos_t keypos);
typedef bool (*is_pressed)(platform_keycode_t);

typedef struct {
    key_buffer_tap register_key_fn;
    key_buffer_untap unregister_key_fn;
    key_buffer_key tap_key_fn;
    is_pressed is_keycode_pressed;
} pipeline_actions_t;

typedef void (*pipeline_callback)(pipeline_callback_params_t*, pipeline_actions_t*, void*);

typedef struct {
    pipeline_callback callback;
    void* data;
} pipeline_t;

typedef struct {
    platform_key_press_buffer_t *key_press_buffer;
    platform_key_event_buffer_t *key_event_buffer;
    platform_key_event_buffer_t *key_event_buffer_swap; // Swap buffer for double buffering
    capture_pipeline_t capture_pipeline;
    size_t pipeline_index;
    platform_deferred_token deferred_exec_callback_token;
} pipeline_executor_state_t;

typedef struct {
    size_t length;
    pipeline_t *pipelines[];
} pipeline_executor_config_t;

extern pipeline_executor_config_t *pipeline_executor_config;

void pipeline_executor_global_state_create(void);

void pipeline_executor_capture_next_keys_or_callback_on_timeout(platform_time_t callback_time);
void pipeline_executor_capture_next_keys(void);

pipeline_t* add_pipeline(pipeline_callback callback, void* user_data);
bool pipeline_process_key(abskeyevent_t abskeyevent);

#ifdef __cplusplus
}
#endif
