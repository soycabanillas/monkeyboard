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

#define PLATFORM_KC_LEFT_CTRL   0xE0
#define PLATFORM_KC_LEFT_SHIFT  0xE1
#define PLATFORM_KC_LEFT_ALT    0xE2
#define PLATFORM_KC_LEFT_GUI    0xE3
#define PLATFORM_KC_RIGHT_CTRL  0xE4
#define PLATFORM_KC_RIGHT_SHIFT 0xE5
#define PLATFORM_KC_RIGHT_ALT   0xE6
#define PLATFORM_KC_RIGHT_GUI   0xE7

// Modifier bit in 32-bit keycode format
#define MOONKEEB_BIT_LCTL  (1 << 8)  // Bit 15 in keycode
#define MOONKEEB_BIT_LSFT  (1 << 9)  // Bit 14 in keycode
#define MOONKEEB_BIT_LALT  (1 << 10)  // Bit 13 in keycode
#define MOONKEEB_BIT_LGUI  (1 << 11)  // Bit 12 in keycode
#define MOONKEEB_BIT_RCTL  (1 << 12)  // Bit 11 in keycode
#define MOONKEEB_BIT_RSFT  (1 << 13)  // Bit 10 in keycode
#define MOONKEEB_BIT_RALT  (1 << 14)  // Bit 9 in keycode
#define MOONKEEB_BIT_RGUI  (1 << 15)  // Bit 8 in keycode

#define MONKEEB_LCTL(key) (MOONKEEB_BIT_LCTL | (key))
#define MONKEEB_LSFT(key) (MOONKEEB_BIT_LSFT | (key))
#define MONKEEB_LALT(key) (MOONKEEB_BIT_LALT | (key))
#define MONKEEB_LGUI(key) (MOONKEEB_BIT_LGUI | (key))

#define MONKEEB_RCTL(key) (MOONKEEB_MOONKEEB_BIT_RCTL | (key))
#define MONKEEB_RSFT(key) (MOONKEEB_BIT_RSFT | (key))
#define MONKEEB_RALT(key) (MOONKEEB_BIT_RALT | (key))
#define MONKEEB_RGUI(key) (MOONKEEB_BIT_RGUI | (key))


#define MONKEEB_LCS(kc) (MOONKEEB_BIT_LCTL | MOONKEEB_BIT_LSFT | (kc))
#define MONKEEB_LCA(kc) (MOONKEEB_BIT_LCTL | MOONKEEB_BIT_LALT | (kc))
#define MONKEEB_LCG(kc) (MOONKEEB_BIT_LCTL | MOONKEEB_BIT_LGUI | (kc))
#define MONKEEB_LSA(kc) (MOONKEEB_BIT_LSFT | MOONKEEB_BIT_LALT | (kc))
#define MONKEEB_LSG(kc) (MOONKEEB_BIT_LSFT | MOONKEEB_BIT_LGUI | (kc))
#define MONKEEB_LAG(kc) (MOONKEEB_BIT_LALT | MOONKEEB_BIT_LGUI | (kc))
#define MONKEEB_LCSG(kc) (MOONKEEB_BIT_LCTL | MOONKEEB_BIT_LSFT | MOONKEEB_BIT_LGUI | (kc))
#define MONKEEB_LCAG(kc) (MOONKEEB_BIT_LCTL | MOONKEEB_BIT_LALT | MOONKEEB_BIT_LGUI | (kc))
#define MONKEEB_LSAG(kc) (MOONKEEB_BIT_LSFT | MOONKEEB_BIT_LALT | MOONKEEB_BIT_LGUI | (kc))

#define MONKEEB_RCA(kc) (MOONKEEB_BIT_RCTL | MOONKEEB_BIT_RALT | (kc))
#define MONKEEB_RCS(kc) (MOONKEEB_BIT_RCTL | MOONKEEB_BIT_RSFT | (kc))
#define MONKEEB_RCG(kc) (MOONKEEB_BIT_RCTL | MOONKEEB_BIT_RGUI | (kc))
#define MONKEEB_RSA(kc) (MOONKEEB_BIT_RSFT | MOONKEEB_BIT_RALT | (kc))
#define MONKEEB_RSG(kc) (MOONKEEB_BIT_RSFT | MOONKEEB_BIT_RGUI | (kc))
#define MONKEEB_RAG(kc) (MOONKEEB_BIT_RALT | MOONKEEB_BIT_RGUI | (kc))
#define MONKEEB_RCSG(kc) (MOONKEEB_BIT_RCTL | MOONKEEB_BIT_RSFT | MOONKEEB_BIT_RGUI | (kc))
#define MONKEEB_RCAG(kc) (MOONKEEB_BIT_RCTL | MOONKEEB_BIT_RALT | MOONKEEB_BIT_RGUI | (kc))
#define MONKEEB_RSAG(kc) (MOONKEEB_BIT_RSFT | MOONKEEB_BIT_RALT | MOONKEEB_BIT_RGUI | (kc))

// Modifier bit when extracted to 8-bit modifier field
#define MONKEEB_MOD_LCTL  (1 << 0)
#define MONKEEB_MOD_LSFT  (1 << 1)
#define MONKEEB_MOD_LALT  (1 << 2)
#define MONKEEB_MOD_LGUI  (1 << 3)
#define MONKEEB_MOD_RCTL  (1 << 4)
#define MONKEEB_MOD_RSFT  (1 << 5)
#define MONKEEB_MOD_RALT  (1 << 6)
#define MONKEEB_MOD_RGUI  (1 << 7)

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
