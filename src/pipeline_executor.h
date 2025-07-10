#pragma once

#include <stddef.h>
#include <stdint.h>
#include "key_buffer.h"
#include "platform_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIPELINE_CALLBACK_KEY_PRESS,
    PIPELINE_CALLBACK_KEY_RELEASE,
    PIPELINE_CALLBACK_TIMER
} pipeline_callback_type_t;

typedef bool (*is_pressed)(platform_keycode_t);
typedef struct {
    is_pressed is_pressed_fn;
} pipeline_info_t;

typedef struct {
    platform_time_t callback_time;
    bool captured;
    size_t pipeline_index; // Index of the pipeline that captured the callback
} capture_pipeline_t;

typedef struct {
    platform_keycode_t keycode;
    platform_keypos_t keypos;
    platform_time_t time;
    uint8_t layer;
    pipeline_callback_type_t callback_type;
    pipeline_info_t info;
} pipeline_callback_params_t;

typedef void (*key_buffer_tap)(platform_keycode_t keycode, platform_keypos_t keypos);
typedef void (*key_buffer_untap)(platform_keycode_t keycode);
typedef void (*key_buffer_key)(platform_keycode_t keycode, platform_keypos_t keypos);
typedef struct {
    key_buffer_tap add_tap_fn;
    key_buffer_untap add_untap_fn;
    key_buffer_key add_key_fn;
} pipeline_actions_t;

typedef void (*pipeline_callback)(pipeline_callback_params_t*, pipeline_actions_t*, void*);

typedef struct {
    pipeline_callback callback;
    void* data;
} pipeline_definition_t;

typedef struct {
    key_buffer_t *key_buffer; // Pointer to the key buffer used by the pipeline
    pipeline_definition_t* definition;
} pipeline_t;

typedef struct {
    key_buffer_t *key_buffer;
    capture_pipeline_t capture_pipeline;
    size_t pipeline_index;
    uint8_t deferred_exec_callback;
} pipeline_executor_state_t;

typedef struct {
    size_t length;
    pipeline_t *pipelines[];
} pipeline_executor_config_t;

extern pipeline_executor_config_t *pipeline_executor_config;

void pipeline_executor_global_state_create(void);
void pipeline_executor_global_state_destroy(void);

void pipeline_executor_capture_next_keys_or_callback_on_timeout(platform_time_t callback_time);
void pipeline_executor_capture_next_keys(void);

pipeline_t* add_pipeline(pipeline_callback callback, void* user_data);
bool pipeline_process_key(platform_keycode_t keycode, abskeyevent_t abskeyevent);

#ifdef __cplusplus
}
#endif
