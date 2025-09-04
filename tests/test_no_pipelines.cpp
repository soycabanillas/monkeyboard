#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "keyboard_simulator.hpp"
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "test_scenario.hpp"
#include "oneshot_test_helpers.hpp"

extern "C" {
#include "pipeline_executor.h"
}

class NoPipelines : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is now handled by TestScenario class
    }

    void TearDown() override {
        // Cleanup is now handled by TestScenario destructor
    }
};

// Test no pipelines
TEST_F(NoPipelines, BasicKeyPressRelease_NoPipelines) {
    const uint16_t KEY_A = 100;
    const uint16_t KEY_B = 101;

    std::vector<std::vector<std::vector<uint16_t>>> keymap = {{
        {{ KEY_A, KEY_B }}
    }};

    TestScenario scenario(keymap);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    keyboard.press_key_at(KEY_A, 0);
    keyboard.release_key_at(KEY_A, 10);
    keyboard.press_key_at(KEY_B, 20);
    keyboard.release_key_at(KEY_B, 30);

    std::vector<event_t> expected_events = {
        td_press(KEY_A, 0),
        td_release(KEY_A, 10),
        td_press(KEY_B, 20),
        td_release(KEY_B, 30),
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
    EXPECT_TRUE(false);
}
