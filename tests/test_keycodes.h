#pragma once

// Test-specific keycode definitions
// This file provides all the necessary keycode definitions for tests
// without conflicts with the real QMK headers

#include <stdbool.h>

// Basic keycodes needed for tests
#define KC_NO 0x00
#define KC_TRANSPARENT 0x01
#define KC_A 0x04
#define KC_B 0x05
#define KC_C 0x06
#define KC_D 0x07
#define KC_E 0x08
#define KC_F 0x09
#define KC_G 0x0A
#define KC_H 0x0B
#define KC_I 0x0C
#define KC_J 0x0D
#define KC_K 0x0E
#define KC_L 0x0F
#define KC_M 0x10
#define KC_N 0x11
#define KC_O 0x12
#define KC_P 0x13
#define KC_Q 0x14
#define KC_R 0x15
#define KC_S 0x16
#define KC_T 0x17
#define KC_U 0x18
#define KC_V 0x19
#define KC_W 0x1A
#define KC_X 0x1B
#define KC_Y 0x1C
#define KC_Z 0x1D

// Additional keycodes
#define KC_1 0x1E
#define KC_2 0x1F
#define KC_3 0x20
#define KC_4 0x21
#define KC_5 0x22
#define KC_6 0x23
#define KC_7 0x24
#define KC_8 0x25
#define KC_9 0x26
#define KC_0 0x27

#define KC_ENTER 0x28
#define KC_ESCAPE 0x29
#define KC_BACKSPACE 0x2A
#define KC_TAB 0x2B
#define KC_SPACE 0x2C

// Special characters
#define KC_EXLM 0x21  // !
#define KC_QUES 0x3F  // ?

// Modifier keys
#define KC_LEFT_CTRL 0xE0
#define KC_LEFT_SHIFT 0xE1
#define KC_LEFT_ALT 0xE2
#define KC_LEFT_GUI 0xE3
#define KC_RIGHT_CTRL 0xE4
#define KC_RIGHT_SHIFT 0xE5
#define KC_RIGHT_ALT 0xE6
#define KC_RIGHT_GUI 0xE7

// Additional keycodes for completeness
#define KC_SLSH 0x38  // /
#define KC_BSLS 0x31  // backslash

// Modifier macros for test environment
#define S(kc) (0x0100 | (kc))
#define LSFT(kc) (0x0100 | (kc))
#define RALT(kc) (0x0600 | (kc))

// Layer definitions will come from commons.h to avoid conflicts
// The real layer enum is defined in commons.h as planck_layers

// Tap Dance function IDs
typedef enum {
    TD_NONE,
    TD_UNKNOWN = -1
} td_state_t;

typedef enum {
    TD_TAP,
    TD_HOLD,
    TD_DOUBLE_TAP,
    TD_TAP_HOLD,
    TD_DOUBLE_HOLD,
    TD_DOUBLE_SINGLE_TAP,
    TD_TRIPLE_TAP,
    TD_TRIPLE_HOLD,
    TD_UNKNOWN_STATE = -1
} td_action_t;
