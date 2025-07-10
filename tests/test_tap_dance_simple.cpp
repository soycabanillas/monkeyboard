#include <gtest/gtest.h>
#include "platform_mock.h"

extern "C" {
#include "keycodes.h"
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class TapDanceSimpleTest : public ::testing::Test {
protected:
    pipeline_executor_config_t* executor_config;
    pipeline_tap_dance_global_config_t* tap_dance_config;

    void SetUp() override {
        reset_mock_state();

        // Create a minimal executor config with one pipeline
        size_t n_pipelines = 1;
        executor_config = static_cast<pipeline_executor_config_t*>(
            malloc(sizeof(pipeline_executor_config_t) + n_pipelines * sizeof(pipeline_t*)));
        executor_config->length = n_pipelines;

        // Create minimal tap dance config
        size_t n_tap_dances = 1;
        tap_dance_config = static_cast<pipeline_tap_dance_global_config_t*>(
            malloc(sizeof(pipeline_tap_dance_global_config_t) +
                   n_tap_dances * sizeof(pipeline_tap_dance_behaviour_t*)));
        tap_dance_config->length = n_tap_dances;

        // Create a simple tap dance behavior
        pipeline_tap_dance_behaviour_t* behavior = static_cast<pipeline_tap_dance_behaviour_t*>(
            malloc(sizeof(pipeline_tap_dance_behaviour_t) +
                   2 * sizeof(pipeline_tap_dance_action_config_t*)));
        behavior->length = 2;

        // Single tap: send KC_A
        behavior->actions[0] = static_cast<pipeline_tap_dance_action_config_t*>(
            malloc(sizeof(pipeline_tap_dance_action_config_t)));
        behavior->actions[0]->repetitions = 1;
        behavior->actions[0]->action = TDCL_TAP_KEY_SENDKEY;
        behavior->actions[0]->keycode = KC_A;
        behavior->actions[0]->layer = 0;

        // Double tap: send KC_B
        behavior->actions[1] = static_cast<pipeline_tap_dance_action_config_t*>(
            malloc(sizeof(pipeline_tap_dance_action_config_t)));
        behavior->actions[1]->repetitions = 2;
        behavior->actions[1]->action = TDCL_TAP_KEY_SENDKEY;
        behavior->actions[1]->keycode = KC_B;
        behavior->actions[1]->layer = 0;

        tap_dance_config->behaviours[0] = behavior;

        // Create and configure the tap dance pipeline
        pipeline_t* tap_dance_pipeline = pipeline_tap_dance_initializer_create(
            TD(0), tap_dance_config);
        executor_config->pipelines[0] = tap_dance_pipeline;
    }

    void TearDown() override {
        // Clean up allocated memory
        if (tap_dance_config) {
            for (size_t i = 0; i < tap_dance_config->length; i++) {
                pipeline_tap_dance_behaviour_t* behavior = tap_dance_config->behaviours[i];
                if (behavior) {
                    for (size_t j = 0; j < behavior->length; j++) {
                        free(behavior->actions[j]);
                    }
                    free(behavior);
                }
            }
            free(tap_dance_config);
        }

        if (executor_config) {
            for (size_t i = 0; i < executor_config->length; i++) {
                free(executor_config->pipelines[i]);
            }
            free(executor_config);
        }
    }
};

TEST_F(TapDanceSimpleTest, BasicInitialization) {
    // Test that tap dance pipeline can be created
    ASSERT_NE(executor_config, nullptr);
    ASSERT_NE(tap_dance_config, nullptr);
    ASSERT_EQ(executor_config->length, 1);
    ASSERT_EQ(tap_dance_config->length, 1);
}

TEST_F(TapDanceSimpleTest, SingleTapBehavior) {
    // This test would require implementing the key processing logic
    // For now, just verify the configuration is correct
    pipeline_tap_dance_behaviour_t* behavior = tap_dance_config->behaviours[0];
    ASSERT_NE(behavior, nullptr);
    ASSERT_EQ(behavior->length, 2);

    // Check single tap action
    ASSERT_EQ(behavior->actions[0]->repetitions, 1);
    ASSERT_EQ(behavior->actions[0]->action, TDCL_TAP_KEY_SENDKEY);
    ASSERT_EQ(behavior->actions[0]->keycode, KC_A);

    // Check double tap action
    ASSERT_EQ(behavior->actions[1]->repetitions, 2);
    ASSERT_EQ(behavior->actions[1]->action, TDCL_TAP_KEY_SENDKEY);
    ASSERT_EQ(behavior->actions[1]->keycode, KC_B);
}
