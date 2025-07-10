# QMK Integration Example

This directory contains documentation and examples for integrating the tap dance engine with QMK firmware.

## Integration Steps

1. **Copy the library**: Add the tap dance engine as a submodule or copy source files
2. **Implement platform interface**: Create `platform_qmk_impl.c` with QMK-specific implementations
3. **Configure keymap**: Set up your tap dance behaviors in `keymap.c`
4. **Build configuration**: Update `rules.mk` to include the tap dance engine

## Example Implementation

See the main QMK userspace repository for a complete working example at:
`keyboards/crkbd/keymaps/soycabanillas/`

## Platform Implementation

The QMK platform implementation maps the platform interface to QMK functions:

```c
void platform_send_key(platform_keycode_t keycode) {
    tap_code(keycode);
}

void platform_layer_on(uint8_t layer) {
    layer_on(layer);
}

// ... etc
```

## Build Integration

Add to your `rules.mk`:

```make
# Enable tap dance engine
TAP_DANCE_ENGINE_ENABLE = yes

# Include source files
SRC += tap_dance_engine/src/pipeline_tap_dance.c
SRC += tap_dance_engine/src/key_buffer.c
SRC += tap_dance_engine/src/pipeline_executor.c
# ... other source files

# Include directories
VPATH += tap_dance_engine/src
```
