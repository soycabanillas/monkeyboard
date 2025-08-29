#pragma once

#include "platform_types.h"

platform_time_t calculate_time_span(platform_time_t previous_time, platform_time_t next_time);
bool time_is_after(platform_time_t timestamp_a, platform_time_t timestamp_b);
bool time_is_after_or_equal(platform_time_t timestamp_a, platform_time_t timestamp_b);
bool time_is_before(platform_time_t timestamp_a, platform_time_t timestamp_b);
bool time_is_before_or_equal(platform_time_t timestamp_a, platform_time_t timestamp_b);
