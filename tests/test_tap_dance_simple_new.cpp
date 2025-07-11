#include <gtest/gtest.h>
#include <cstdlib>
#include "platform_interface.h"
#include "platform_mock.h"

extern "C" {
#include "keycodes.h"
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "key_buffer.h"
}

class TapDanceSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {
        reset_mock_state();

        // Initialize the global tap dance state
        pipeline_tap_dance_global_state_create();
    }

    void TearDown() override {
        // Clean up if needed
    }
};

TEST_F(TapDanceSimpleTest, BasicInitialization) {
    // Test that tap dance global state can be created
    // This is a simple smoke test to verify the basic structure works
    SUCCEED();
}

TEST_F(TapDanceSimpleTest, CreateAction) {
    // Test creating a basic tap dance action
    pipeline_tap_dance_action_config_t* action = createbehaviouraction(
        1, TDCL_TAP_KEY_SENDKEY, KC_A, 0);

    ASSERT_NE(action, nullptr);
    ASSERT_EQ(action->repetitions, 1);
    ASSERT_EQ(action->action, TDCL_TAP_KEY_SENDKEY);
    ASSERT_EQ(action->keycode, KC_A);
    ASSERT_EQ(action->layer, 0);

    // Clean up
    free(action);
}

TEST_F(TapDanceSimpleTest, CreateBehaviour) {
    // Test creating a basic tap dance behavior
    pipeline_tap_dance_action_config_t* action1 = createbehaviouraction(
        1, TDCL_TAP_KEY_SENDKEY, KC_A, 0);
    pipeline_tap_dance_action_config_t* action2 = createbehaviouraction(
        2, TDCL_TAP_KEY_SENDKEY, KC_B, 0);

    pipeline_tap_dance_action_config_t* actions[] = {action1, action2};

    pipeline_tap_dance_behaviour_t* behavior = createbehaviour(TD(0), actions, 2);

    ASSERT_NE(behavior, nullptr);
    ASSERT_NE(behavior->config, nullptr);
    ASSERT_EQ(behavior->config->keycodemodifier, TD(0));
    ASSERT_EQ(behavior->config->actionslength, 2);

    // Test the actions
    ASSERT_EQ(behavior->config->actions[0]->repetitions, 1);
    ASSERT_EQ(behavior->config->actions[0]->keycode, KC_A);
    ASSERT_EQ(behavior->config->actions[1]->repetitions, 2);
    ASSERT_EQ(behavior->config->actions[1]->keycode, KC_B);

    // Clean up
    free(action1);
    free(action2);
    free(behavior->config);
    free(behavior->status);
    free(behavior);
}

TEST_F(TapDanceSimpleTest, KeyBufferOperations) {
    // Test basic key buffer operations
    key_buffer_t* buffer = pipeline_key_buffer_create();

    ASSERT_NE(buffer, nullptr);

    // Test adding a key press
    platform_keypos_t pos = {.row = 0, .col = 0};
    bool result = add_to_press_buffer(buffer, KC_A, pos, 0, 0, true, false, 0);
    ASSERT_TRUE(result);

    // Clean up
    pipeline_key_buffer_destroy(buffer);
}
