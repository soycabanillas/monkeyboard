#pragma once

// Platform-agnostic key code definitions
// These will be mapped to actual platform-specific codes by the platform implementation

// Basic key codes (these are just placeholders - real values depend on platform)
#define KC_NO           0x00
#define KC_TRNS         0x01

// Example key codes - replace with actual values from your platform
#define KC_A            0x04
#define KC_B            0x05
#define KC_C            0x06
// ... add more as needed

// Modifier keys
#define KC_LCTL         0xE0
#define KC_LSFT         0xE1
#define KC_LALT         0xE2
#define KC_LGUI         0xE3
#define KC_RCTL         0xE4
#define KC_RSFT         0xE5
#define KC_RALT         0xE6
#define KC_RGUI         0xE7

// Layer keys
#define TO(layer)       (0x5100 | (layer))
#define MO(layer)       (0x5200 | (layer))
#define TG(layer)       (0x5300 | (layer))

// One-shot keys
#define OSM(mod)        (0x5400 | (mod))
#define OSL(layer)      (0x5500 | (layer))

// Tap dance
#define TD(index)       (0x5600 | (index))
