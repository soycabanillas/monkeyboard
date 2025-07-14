#include <gtest/gtest.h>
#include "tap_dance_test_framework.hpp"

TEST(DebugTest, SimplePipelineTest) {
    reset_mock_state();

    // Initialize pipeline executor
    size_t n_pipelines = 3;
    pipeline_executor_config = static_cast<pipeline_executor_config_t*>(
        malloc(sizeof(pipeline_executor_config_t) + n_pipelines * sizeof(pipeline_t*)));
    pipeline_executor_config->length = n_pipelines;
    pipeline_executor_global_state_create();

    // Create simple tap dance config
    auto config = TapDanceTestConfig().add_tap_key(TEST_KEY_TAP_DANCE_1, 1, OUT_KEY_X);
    auto* global_config = config.build();
    pipeline_tap_dance_global_state_create();
    pipeline_executor_config->pipelines[1] = add_pipeline(&pipeline_tap_dance_callback, global_config);

    printf("=== Before key press ===\n");
    mock_print_state();

    // Simulate key press
    abskeyevent_t event = {
        .key = {.row = 0, .col = 0},
        .pressed = true,
        .time = platform_timer_read()
    };
    printf("=== Calling pipeline_process_key for PRESS ===\n");
    bool result = pipeline_process_key(TEST_KEY_TAP_DANCE_1, event);
    printf("Pipeline process result: %s\n", result ? "true" : "false");

    printf("=== After key press ===\n");
    mock_print_state();

    // Simulate key release
    event.pressed = false;
    printf("=== Calling pipeline_process_key for RELEASE ===\n");
    result = pipeline_process_key(TEST_KEY_TAP_DANCE_1, event);
    printf("Pipeline process result: %s\n", result ? "true" : "false");

    printf("=== After key release ===\n");
    mock_print_state();

    // Wait for timeout
    platform_wait_ms(250);
    printf("=== After timeout ===\n");
    mock_print_state();

    EXPECT_GT(g_mock_state.send_key_calls_count(), 0);
}
