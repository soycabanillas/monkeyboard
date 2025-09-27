#pragma once

#include "platform_types.h"
#include <stdint.h>
#include <stdbool.h>

#define BASIC_KEYCODE_MIN 0x00000000
#define BASIC_KEYCODE_MAX 0x000000FF
#define MODIFIED_KEYCODE_MIN 0x00000100
#define MODIFIED_KEYCODE_MAX 0x0000FFFF
#define UNICODE_KEYCODE_MIN 0x00010000
#define UNICODE_KEYCODE_MAX 0x001FFFFF
#define CUSTOM_KEYCODE_MIN 0x00200000
#define CUSTOM_KEYCODE_MAX 0x7FFFFFFF

typedef enum {
    KEYCODE_BASIC,
    KEYCODE_MODIFIED,
    KEYCODE_UNICODE,
    KEYCODE_CUSTOM,
    KEYCODE_INVALID
} keycode_type_t;

typedef struct {
    keycode_type_t type;
    uint8_t basic_key;      // Valid for BASIC and MODIFIED
    uint8_t modifiers;      // Valid for MODIFIED only
    uint32_t unicode_cp;    // Valid for UNICODE only  
    uint32_t custom_func;   // Valid for CUSTOM only
} keycode_info_t;

// Modifier bit definitions for convenience
#define MOD_LCTL  (1 << 7)  // Bit 15 in keycode
#define MOD_LSFT  (1 << 6)  // Bit 14 in keycode  
#define MOD_LALT  (1 << 5)  // Bit 13 in keycode
#define MOD_LGUI  (1 << 4)  // Bit 12 in keycode
#define MOD_RCTL  (1 << 3)  // Bit 11 in keycode
#define MOD_RSFT  (1 << 2)  // Bit 10 in keycode
#define MOD_RALT  (1 << 1)  // Bit 9 in keycode  
#define MOD_RGUI  (1 << 0)  // Bit 8 in keycode

// =============================================================================
// DECODING FUNCTIONS - Extract information from 32-bit keycodes
// =============================================================================

keycode_type_t monkeeb_keycodes_get_keycode_type(platform_keycode_t keycode);
uint8_t monkeeb_keycodes_get_basic_key(platform_keycode_t keycode);
uint8_t monkeeb_keycodes_get_modifiers(platform_keycode_t keycode);
uint32_t monkeeb_keycodes_get_unicode_codepoint(platform_keycode_t keycode);
uint32_t monkeeb_keycodes_get_custom_function(platform_keycode_t keycode);
bool monkeeb_keycodes_has_modifiers(platform_keycode_t keycode);
bool monkeeb_keycodes_has_modifier(platform_keycode_t keycode, uint8_t modifier_bit);
keycode_info_t monkeeb_keycodes_decode_keycode(platform_keycode_t keycode);

// =============================================================================
// ENCODING FUNCTIONS - Create 32-bit keycodes from components
// =============================================================================

platform_keycode_t monkeeb_keycodes_make_basic_keycode(uint8_t key);
platform_keycode_t monkeeb_keycodes_make_modified_keycode(uint8_t key, uint8_t modifiers);
platform_keycode_t monkeeb_keycodes_make_unicode_keycode(uint32_t unicode_codepoint);
platform_keycode_t monkeeb_keycodes_make_custom_keycode(uint32_t function_id);

// =============================================================================
// CONVENIENCE FUNCTIONS - Common keycode creation patterns
// =============================================================================

platform_keycode_t monkeeb_keycodes_make_keycode_with_mod(uint8_t key, uint8_t single_modifier);
platform_keycode_t monkeeb_keycodes_make_keycode_with_mods(uint8_t key, uint8_t mod1, uint8_t mod2);
platform_keycode_t monkeeb_keycodes_make_ctrl_keycode(uint8_t key, bool left_ctrl);
platform_keycode_t monkeeb_keycodes_make_shift_keycode(uint8_t key, bool left_shift);
platform_keycode_t monkeeb_keycodes_make_alt_keycode(uint8_t key, bool left_alt);
platform_keycode_t monkeeb_keycodes_make_gui_keycode(uint8_t key, bool left_gui);

// =============================================================================
// KEYCODE MANIPULATION FUNCTIONS
// =============================================================================

platform_keycode_t monkeeb_keycodes_add_modifier(platform_keycode_t keycode, uint8_t modifier);
platform_keycode_t monkeeb_keycodes_remove_modifier(platform_keycode_t keycode, uint8_t modifier);
platform_keycode_t monkeeb_keycodes_toggle_modifier(platform_keycode_t keycode, uint8_t modifier);

// =============================================================================
// VALIDATION FUNCTIONS
// =============================================================================

bool monkeeb_keycodes_is_valid_keycode(platform_keycode_t keycode);
bool monkeeb_keycodes_is_valid_basic_key(uint8_t key);
bool monkeeb_keycodes_is_valid_unicode_codepoint(uint32_t codepoint);
