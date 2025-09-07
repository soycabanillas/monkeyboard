#include "monkeyboard_time_manager.h"
#include "platform_types.h"

// Define the overflow threshold based on platform timer size
#define TIMER_OVERFLOW_THRESHOLD (PLATFORM_TIME_MAX / 2 + 1)

/**
 * @brief Calculate time span between two timestamps
 * 
 * This function correctly handles timer overflow scenarios using unsigned arithmetic
 * properties. It works correctly regardless of how many times the timer has wrapped
 * around, as long as the actual time difference is less than half the timer range.
 * 
 * @param previous_time Earlier timestamp
 * @param next_time Later timestamp
 * @return Time difference in platform time units, or 0 if difference exceeds safe range
 * 
 * @note For 16-bit timers: Works for time differences up to ~32.8 seconds
 * @note For 32-bit timers: Works for time differences up to ~24.8 days
 * @note For 64-bit timers: Works for time differences up to ~292 million years
 */
platform_time_t calculate_time_span(platform_time_t previous_time, platform_time_t next_time) {
    // Use unsigned arithmetic overflow behavior to calculate difference
    platform_time_t diff = next_time - previous_time;
    
    // If difference is greater than half the timer range, we likely have overflow
    if (diff > TIMER_OVERFLOW_THRESHOLD) {
        // This indicates previous_time is actually after next_time due to overflow
        // Return 0 or handle as needed for your use case
        return 0;
    }
    
    return diff;
}

/**
 * @brief Check if first timestamp is chronologically after second timestamp (overflow-safe)
 * 
 * Uses the same overflow-safe technique as time_has_elapsed to determine chronological order.
 * Works correctly even after multiple timer overflows.
 * 
 * @param timestamp_a First timestamp
 * @param timestamp_b Second timestamp
 * @return true if timestamp_a comes chronologically after timestamp_b
 */
bool time_is_after(platform_time_t timestamp_a, platform_time_t timestamp_b) {
    return (timestamp_a - timestamp_b) < TIMER_OVERFLOW_THRESHOLD;
}

/**
 * @brief Check if first timestamp is chronologically after or equal to second timestamp (overflow-safe)
 * 
 * Uses the same overflow-safe technique to determine chronological order.
 * Works correctly even after multiple timer overflows.
 * 
 * @param timestamp_a First timestamp
 * @param timestamp_b Second timestamp
 * @return true if timestamp_a comes chronologically after or is equal to timestamp_b
 */
bool time_is_after_or_equal(platform_time_t timestamp_a, platform_time_t timestamp_b) {
    platform_time_t diff = timestamp_a - timestamp_b;
    return (diff < TIMER_OVERFLOW_THRESHOLD) || (diff == 0);
}

/**
 * @brief Check if first timestamp is chronologically before second timestamp (overflow-safe)
 * 
 * Uses the same overflow-safe technique to determine chronological order.
 * Works correctly even after multiple timer overflows.
 * 
 * @param timestamp_a First timestamp
 * @param timestamp_b Second timestamp
 * @return true if timestamp_a comes chronologically before timestamp_b
 */
bool time_is_before(platform_time_t timestamp_a, platform_time_t timestamp_b) {
    return time_is_after(timestamp_b, timestamp_a);
}

/**
 * @brief Check if first timestamp is chronologically before or equal to second timestamp (overflow-safe)
 * 
 * Uses the same overflow-safe technique to determine chronological order.
 * Works correctly even after multiple timer overflows.
 * 
 * @param timestamp_a First timestamp
 * @param timestamp_b Second timestamp
 * @return true if timestamp_a comes chronologically before or is equal to timestamp_b
 */
bool time_is_before_or_equal(platform_time_t timestamp_a, platform_time_t timestamp_b) {
    return time_is_after_or_equal(timestamp_b, timestamp_a);
}
