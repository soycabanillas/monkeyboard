#pragma once

#include "../src/platform_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Test utilities for controlling the mock platform
void mock_advance_timer(platform_time_t ms);
void mock_reset_timer(void);
void mock_set_layer(uint8_t layer);
void reset_mock_state(void);

#ifdef __cplusplus
}
#endif
