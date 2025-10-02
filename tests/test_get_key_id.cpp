#include "key_event_buffer.h"
#include <gtest/gtest.h>
#include <stdint.h>

class Event_Buffer_Get_Key_Id : public ::testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
};


TEST_F(Event_Buffer_Get_Key_Id, FirstIdIs1) {
    platform_key_event_buffer_t* event_buffer = platform_key_event_create();
    platform_keypos_t keypos = {0, 0};
    uint8_t press_id = platform_key_event_add_physical_press(event_buffer, 0, keypos, 0, nullptr);

    EXPECT_EQ(press_id, 1);
}

TEST_F(Event_Buffer_Get_Key_Id, LastIdIs1) {
    platform_key_event_buffer_t* event_buffer = platform_key_event_create();
    platform_keypos_t keypos = {0, 0};
    uint8_t press_id;
    for (uint8_t i = 0; i < 120; i++) {
        keypos = {0, 0};
        press_id = platform_key_event_add_physical_press(event_buffer, 0, keypos, 0, nullptr);
        platform_key_event_add_physical_release(event_buffer, 0, keypos, nullptr);
    }

    EXPECT_EQ(press_id, 255);
}


TEST_F(Event_Buffer_Get_Key_Id, AvoidReusingNumbersOnTheBuffer) {
    platform_key_event_buffer_t* event_buffer = platform_key_event_create();
    platform_keypos_t keypos;
    uint8_t press_id;
    for (uint8_t i = 0; i < PLATFORM_KEY_EVENT_MAX_ELEMENTS / 2; i++) {
        keypos = {i, i};
        press_id = platform_key_event_add_physical_press(event_buffer, 0, keypos, 0, nullptr);
        platform_key_event_add_physical_release(event_buffer, 0, keypos, nullptr);
    }
    EXPECT_EQ(press_id, 10);
}

TEST_F(Event_Buffer_Get_Key_Id, AvoidReusingNumbersOnTheBuffer2) {
    platform_key_event_buffer_t* event_buffer = platform_key_event_create();
    platform_keypos_t keypos;
    uint8_t press_id;
    for (uint8_t i = 0; i < 255 / 2; i++) {
        keypos = {i, i};
        press_id = platform_key_event_add_physical_press(event_buffer, 0, keypos, 0, nullptr);
        platform_key_event_add_physical_release(event_buffer, 0, keypos, nullptr);
    }

    EXPECT_EQ(press_id, 10);
}