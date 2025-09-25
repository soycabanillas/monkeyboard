#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "gtest/gtest.h"
#include "platform_interface.h"
#include "platform_mock.hpp"
#include "platform_types.h"
#include "test_scenario.hpp"
#include "event_buffer_test_helpers.hpp"
#include "tap_dance_test_helpers.hpp"

extern "C" {
#include "pipeline_tap_dance.h"
#include "pipeline_tap_dance_initializer.h"
#include "pipeline_executor.h"
}

class EventBufferManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup is handled by TestScenario and EventBufferManager
    }

    void TearDown() override {
        // Cleanup is handled by destructors
    }
};

TEST_F(EventBufferManagementTest, CustomEventBufferDirectManipulation) {
    const platform_keycode_t TAP_DANCE_KEY = 3000;
    const platform_keycode_t OUTPUT_KEY = 3001;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }};

    // Create custom event buffer manager
    EventBufferManager custom_event_buffer;
    
    // Pre-populate the event buffer with some events
    uint8_t press_id1 = custom_event_buffer.add_physical_press(0, 0, 0);
    bool release_added = custom_event_buffer.add_physical_release(100, 0, 0);
    
    EXPECT_GT(press_id1, 0);
    EXPECT_TRUE(release_added);
    EXPECT_EQ(custom_event_buffer.get_event_count(), 2);
    EXPECT_EQ(custom_event_buffer.get_press_count(), 0); // Press should be removed after release

    // Create scenario with custom event buffer
    TestScenario scenario(keymap, custom_event_buffer);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}})
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Verify initial state
    EXPECT_EQ(scenario.event_buffer_manager().get_event_count(), 2);

    // Add more events through the keyboard simulator
    keyboard.press_key_at(TAP_DANCE_KEY, 200);
    keyboard.release_key_at(TAP_DANCE_KEY, 250);

    // Check the event buffer state after processing
    const auto& buffer_manager = scenario.event_buffer_manager();
    
    // Events should have been processed by the pipeline
    std::vector<platform_key_event_t> all_events = buffer_manager.get_all_events();
    std::vector<platform_key_press_key_press_t> all_presses = buffer_manager.get_all_presses();

    // Verify that tap dance was triggered
    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY, 250), td_release(OUTPUT_KEY, 250)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(EventBufferManagementTest, EventBufferQueryMethods) {
    const platform_keycode_t KEY_A = 4000;
    const platform_keycode_t KEY_B = 4001;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { KEY_A, KEY_B }
    }};

    EventBufferManager custom_event_buffer;
    TestScenario scenario(keymap, custom_event_buffer);
    
    // Don't add any pipelines for this test
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Add multiple key events
    keyboard.press_key_at(KEY_A, 0);
    keyboard.press_key_at(KEY_B, 10);
    keyboard.release_key_at(KEY_A, 20);
    keyboard.release_key_at(KEY_B, 30);

    const auto& buffer_manager = scenario.event_buffer_manager();

    // Test query methods
    EXPECT_EQ(buffer_manager.get_event_count(), 4);
    EXPECT_EQ(buffer_manager.get_press_count(), 0); // All should be released

    // Test event retrieval
    auto events = buffer_manager.get_all_events();
    EXPECT_EQ(events.size(), 4);

    // Verify event order and content
    EXPECT_EQ(events[0].keycode, KEY_A);
    EXPECT_TRUE(events[0].is_press);
    EXPECT_EQ(events[1].keycode, KEY_B);
    EXPECT_TRUE(events[1].is_press);
    EXPECT_EQ(events[2].keycode, KEY_A);
    EXPECT_FALSE(events[2].is_press);
    EXPECT_EQ(events[3].keycode, KEY_B);
    EXPECT_FALSE(events[3].is_press);

    // Test filtering by keycode
    auto key_a_events = buffer_manager.get_events_by_keycode(KEY_A);
    EXPECT_EQ(key_a_events.size(), 2);
    EXPECT_TRUE(key_a_events[0].is_press);
    EXPECT_FALSE(key_a_events[1].is_press);
}

TEST_F(EventBufferManagementTest, DefaultEventBufferStillWorks) {
    const platform_keycode_t TAP_DANCE_KEY = 6000;
    const platform_keycode_t OUTPUT_KEY = 6001;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }};

    // Create scenario with default event buffer (no custom buffer passed)
    TestScenario scenario(keymap);
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}})
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Test that existing functionality still works exactly the same
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);

    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY, 0), td_release(OUTPUT_KEY, 50)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));

    // Access event buffer manager even with default buffer (new capability)
    const auto& buffer_manager = scenario.event_buffer_manager();
    EXPECT_EQ(buffer_manager.get_press_count(), 0);
}

TEST_F(EventBufferManagementTest, CustomEventBufferWithPipelineProcessing) {
    const platform_keycode_t TAP_DANCE_KEY = 5000;
    const platform_keycode_t OUTPUT_KEY = 5001;
    const platform_keycode_t TARGET_LAYER = 1;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { TAP_DANCE_KEY }
    }, {
        { 5010 }
    }};

    EventBufferManager custom_event_buffer;
    TestScenario scenario(keymap, custom_event_buffer);
    
    TapDanceConfigBuilder config_builder;
    config_builder
        .add_tap_hold(TAP_DANCE_KEY, {{1, OUTPUT_KEY}}, {{1, TARGET_LAYER}}, 200, 200, TAP_DANCE_HOLD_PREFERRED)
        .add_to_scenario(scenario);
    
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Test quick tap
    keyboard.press_key_at(TAP_DANCE_KEY, 0);
    keyboard.release_key_at(TAP_DANCE_KEY, 50);

    // Verify tap behavior was triggered
    std::vector<event_t> expected_events = {
        td_press(OUTPUT_KEY, 50), td_release(OUTPUT_KEY, 50)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));

    // Reset for next test
    g_mock_state.reset();
    custom_event_buffer.reset();

    // Test hold behavior
    keyboard.press_key_at(TAP_DANCE_KEY, 100);
    keyboard.wait_ms(250); // Exceed hold timeout
    keyboard.release_key_at(TAP_DANCE_KEY, 350);

    // Verify hold behavior was triggered
    expected_events = {
        td_layer(TARGET_LAYER, 300), // 100 + 200ms timeout
        td_layer(0, 350)
    };
    EXPECT_TRUE(g_mock_state.event_actions_match_absolute(expected_events));
}

TEST_F(EventBufferManagementTest, InspectBufferStateAfterProcessing) {
    const platform_keycode_t KEY_A = 7000;
    const platform_keycode_t KEY_B = 7001;

    std::vector<std::vector<std::vector<platform_keycode_t>>> keymap = {{
        { KEY_A, KEY_B }
    }};

    EventBufferManager custom_event_buffer;
    TestScenario scenario(keymap, custom_event_buffer);
    
    // Add no pipelines so events pass through unchanged
    scenario.build();
    KeyboardSimulator& keyboard = scenario.keyboard();

    // Add events and inspect buffer state
    keyboard.press_key_at(KEY_A, 10);
    EXPECT_EQ(custom_event_buffer.get_event_count(), 1);
    EXPECT_EQ(custom_event_buffer.get_press_count(), 0); // Events processed immediately

    keyboard.press_key_at(KEY_B, 20);
    EXPECT_EQ(custom_event_buffer.get_event_count(), 2);
    
    keyboard.release_key_at(KEY_A, 30);
    EXPECT_EQ(custom_event_buffer.get_event_count(), 3);
    
    keyboard.release_key_at(KEY_B, 40);
    EXPECT_EQ(custom_event_buffer.get_event_count(), 4);

    // Verify all events are recorded in order
    auto all_events = custom_event_buffer.get_all_events();
    EXPECT_EQ(all_events.size(), 4);
    
    EXPECT_EQ(all_events[0].keycode, KEY_A);
    EXPECT_TRUE(all_events[0].is_press);
    EXPECT_EQ(all_events[0].time, 10);
    
    EXPECT_EQ(all_events[1].keycode, KEY_B);
    EXPECT_TRUE(all_events[1].is_press);
    EXPECT_EQ(all_events[1].time, 20);
    
    EXPECT_EQ(all_events[2].keycode, KEY_A);
    EXPECT_FALSE(all_events[2].is_press);
    EXPECT_EQ(all_events[2].time, 30);
    
    EXPECT_EQ(all_events[3].keycode, KEY_B);
    EXPECT_FALSE(all_events[3].is_press);
    EXPECT_EQ(all_events[3].time, 40);
}

