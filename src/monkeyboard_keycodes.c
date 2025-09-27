#include "monkeyboard_keycodes.h"

/**
 * Determine the type of a 32-bit keycode based on its range
 */
keycode_type_t monkeeb_keycodes_get_keycode_type(platform_keycode_t keycode) {
    if (keycode <= BASIC_KEYCODE_MAX) return KEYCODE_BASIC;
    if (keycode <= MODIFIED_KEYCODE_MAX) return KEYCODE_MODIFIED;  
    if (keycode >= UNICODE_KEYCODE_MIN && keycode <= UNICODE_KEYCODE_MAX) return KEYCODE_UNICODE;
    if (keycode >= CUSTOM_KEYCODE_MIN && keycode <= CUSTOM_KEYCODE_MAX) return KEYCODE_CUSTOM;
    return KEYCODE_INVALID;
}

/**
 * Extract the basic key (0-255) from basic or modified keycodes
 * Returns 0 if keycode is not basic or modified
 */
uint8_t monkeeb_keycodes_get_basic_key(platform_keycode_t keycode) {
    if (keycode > MODIFIED_KEYCODE_MAX) return 0;  // Not a basic/modified keycode
    return keycode & 0xFF;
}

/**
 * Extract modifier bits from modified keycodes
 * Returns 0 if keycode is not a modified keycode
 */
uint8_t monkeeb_keycodes_get_modifiers(platform_keycode_t keycode) {
    if (keycode <= BASIC_KEYCODE_MAX || keycode > MODIFIED_KEYCODE_MAX) return 0;  // Not modified
    return (keycode >> 8) & 0xFF;
}

/**
 * Extract Unicode codepoint from Unicode keycodes
 * Returns 0 if keycode is not a Unicode keycode
 */
uint32_t monkeeb_keycodes_get_unicode_codepoint(platform_keycode_t keycode) {
    if (keycode < UNICODE_KEYCODE_MIN || keycode > UNICODE_KEYCODE_MAX) return 0;  // Not Unicode
    return keycode - UNICODE_KEYCODE_MIN;  // Remove offset to get actual codepoint
}

/**
 * Extract custom function code from custom keycodes
 * Returns 0 if keycode is not a custom keycode
 */
uint32_t monkeeb_keycodes_get_custom_function(platform_keycode_t keycode) {
    if (keycode < CUSTOM_KEYCODE_MIN || keycode > CUSTOM_KEYCODE_MAX) return 0;  // Not custom
    return keycode - CUSTOM_KEYCODE_MIN;  // Remove offset to get function ID
}

/**
 * Check if a keycode has modifiers
 */
bool monkeeb_keycodes_has_modifiers(platform_keycode_t keycode) {
    return monkeeb_keycodes_get_keycode_type(keycode) == KEYCODE_MODIFIED;
}

/**
 * Check if a specific modifier is active
 */
bool monkeeb_keycodes_has_modifier(platform_keycode_t keycode, uint8_t modifier_bit) {
    uint8_t mods = monkeeb_keycodes_get_modifiers(keycode);
    return (mods & modifier_bit) != 0;
}

// Modifier bit definitions for convenience
#define MOD_LCTL  (1 << 7)  // Bit 15 in keycode
#define MOD_LSFT  (1 << 6)  // Bit 14 in keycode  
#define MOD_LALT  (1 << 5)  // Bit 13 in keycode
#define MOD_LGUI  (1 << 4)  // Bit 12 in keycode
#define MOD_RCTL  (1 << 3)  // Bit 11 in keycode
#define MOD_RSFT  (1 << 2)  // Bit 10 in keycode
#define MOD_RALT  (1 << 1)  // Bit 9 in keycode  
#define MOD_RGUI  (1 << 0)  // Bit 8 in keycode

/**
 * Comprehensive keycode decoder that returns all information
 */
keycode_info_t monkeeb_keycodes_decode_keycode(platform_keycode_t keycode) {
    keycode_info_t info = {0};
    info.type = monkeeb_keycodes_get_keycode_type(keycode);
    
    switch (info.type) {
        case KEYCODE_BASIC:
            info.basic_key = monkeeb_keycodes_get_basic_key(keycode);
            break;
            
        case KEYCODE_MODIFIED:
            info.basic_key = monkeeb_keycodes_get_basic_key(keycode);
            info.modifiers = monkeeb_keycodes_get_modifiers(keycode);
            break;
            
        case KEYCODE_UNICODE:
            info.unicode_cp = monkeeb_keycodes_get_unicode_codepoint(keycode);
            break;
            
        case KEYCODE_CUSTOM:
            info.custom_func = monkeeb_keycodes_get_custom_function(keycode);
            break;
            
        case KEYCODE_INVALID:
        default:
            // All fields remain 0
            break;
    }
    
    return info;
}

// =============================================================================
// ENCODING FUNCTIONS - Create 32-bit keycodes from components
// =============================================================================

/**
 * Create a basic keycode (0-255, no modifiers)
 */
platform_keycode_t monkeeb_keycodes_make_basic_keycode(uint8_t key) {
    return key;  // Direct mapping to 0x00000000-0x000000FF range
}

/**
 * Create a modified keycode (basic key + modifiers)
 */
platform_keycode_t monkeeb_keycodes_make_modified_keycode(uint8_t key, uint8_t modifiers) {
    if (modifiers == 0) {
        return monkeeb_keycodes_make_basic_keycode(key);  // No modifiers, use basic range
    }
    return MODIFIED_KEYCODE_MIN + ((platform_keycode_t)modifiers << 8) + key;
}

/**
 * Create a Unicode keycode from a Unicode codepoint
 */
platform_keycode_t monkeeb_keycodes_make_unicode_keycode(uint32_t unicode_codepoint) {
    if (unicode_codepoint > 0x10FFFF) {
        return 0;  // Invalid Unicode codepoint
    }
    return UNICODE_KEYCODE_MIN + unicode_codepoint;
}

/**
 * Create a custom function keycode
 */
platform_keycode_t monkeeb_keycodes_make_custom_keycode(uint32_t function_id) {
    if (function_id > (CUSTOM_KEYCODE_MAX - CUSTOM_KEYCODE_MIN)) {  // Max: 0x7FFFFFFF - 0x20000000
        return 0;  // Function ID too large
    }
    return CUSTOM_KEYCODE_MIN + function_id;
}

// =============================================================================
// CONVENIENCE FUNCTIONS - Common keycode creation patterns
// =============================================================================

/**
 * Create keycode with single modifier
 */
platform_keycode_t monkeeb_keycodes_make_keycode_with_mod(uint8_t key, uint8_t single_modifier) {
    return monkeeb_keycodes_make_modified_keycode(key, single_modifier);
}

/**
 * Create keycode with multiple modifiers (OR them together)
 */
platform_keycode_t monkeeb_keycodes_make_keycode_with_mods(uint8_t key, uint8_t mod1, uint8_t mod2) {
    return monkeeb_keycodes_make_modified_keycode(key, mod1 | mod2);
}

/**
 * Create Ctrl+key combination
 */
platform_keycode_t monkeeb_keycodes_make_ctrl_keycode(uint8_t key, bool left_ctrl) {
    uint8_t mod = left_ctrl ? MOD_LCTL : MOD_RCTL;
    return monkeeb_keycodes_make_modified_keycode(key, mod);
}

/**
 * Create Shift+key combination  
 */
platform_keycode_t monkeeb_keycodes_make_shift_keycode(uint8_t key, bool left_shift) {
    uint8_t mod = left_shift ? MOD_LSFT : MOD_RSFT;
    return monkeeb_keycodes_make_modified_keycode(key, mod);
}

/**
 * Create Alt+key combination
 */
platform_keycode_t monkeeb_keycodes_make_alt_keycode(uint8_t key, bool left_alt) {
    uint8_t mod = left_alt ? MOD_LALT : MOD_RALT;
    return monkeeb_keycodes_make_modified_keycode(key, mod);
}

/**
 * Create GUI/Cmd+key combination
 */
platform_keycode_t monkeeb_keycodes_make_gui_keycode(uint8_t key, bool left_gui) {
    uint8_t mod = left_gui ? MOD_LGUI : MOD_RGUI;
    return monkeeb_keycodes_make_modified_keycode(key, mod);
}

// =============================================================================
// KEYCODE MANIPULATION FUNCTIONS
// =============================================================================

/**
 * Add a modifier to an existing keycode (if it's basic or modified)
 */
platform_keycode_t monkeeb_keycodes_add_modifier(platform_keycode_t keycode, uint8_t modifier) {
    keycode_type_t type = monkeeb_keycodes_get_keycode_type(keycode);
    
    if (type == KEYCODE_BASIC) {
        uint8_t key = monkeeb_keycodes_get_basic_key(keycode);
        return monkeeb_keycodes_make_modified_keycode(key, modifier);
    }
    else if (type == KEYCODE_MODIFIED) {
        uint8_t key = monkeeb_keycodes_get_basic_key(keycode);
        uint8_t existing_mods = monkeeb_keycodes_get_modifiers(keycode);
        return monkeeb_keycodes_make_modified_keycode(key, existing_mods | modifier);
    }
    
    return keycode;  // Can't add modifiers to Unicode or custom keycodes
}

/**
 * Remove a modifier from an existing keycode
 */
platform_keycode_t monkeeb_keycodes_remove_modifier(platform_keycode_t keycode, uint8_t modifier) {
    if (monkeeb_keycodes_get_keycode_type(keycode) != KEYCODE_MODIFIED) {
        return keycode;  // Nothing to remove
    }
    
    uint8_t key = monkeeb_keycodes_get_basic_key(keycode);
    uint8_t existing_mods = monkeeb_keycodes_get_modifiers(keycode);
    uint8_t new_mods = existing_mods & ~modifier;  // Clear the specified bits
    
    if (new_mods == 0) {
        return monkeeb_keycodes_make_basic_keycode(key);  // No modifiers left
    }
    
    return monkeeb_keycodes_make_modified_keycode(key, new_mods);
}

/**
 * Toggle a modifier on an existing keycode
 */
platform_keycode_t monkeeb_keycodes_toggle_modifier(platform_keycode_t keycode, uint8_t modifier) {
    if (monkeeb_keycodes_has_modifier(keycode, modifier)) {
        return monkeeb_keycodes_remove_modifier(keycode, modifier);
    } else {
        return monkeeb_keycodes_add_modifier(keycode, modifier);
    }
}

// =============================================================================
// VALIDATION FUNCTIONS
// =============================================================================

/**
 * Validate that a keycode is well-formed
 */
bool monkeeb_keycodes_is_valid_keycode(platform_keycode_t keycode) {
    return monkeeb_keycodes_get_keycode_type(keycode) != KEYCODE_INVALID;
}

/**
 * Validate that a basic key value is in valid range
 */
bool monkeeb_keycodes_is_valid_basic_key(uint8_t key) {
    return true;  // All 8-bit values are valid basic keys
}

/**
 * Validate that a Unicode codepoint is valid
 */
bool monkeeb_keycodes_is_valid_unicode_codepoint(uint32_t codepoint) {
    return codepoint <= (UNICODE_KEYCODE_MAX - UNICODE_KEYCODE_MIN);  // Max Unicode value
}
