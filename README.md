# Tap Dance Engine

A platform-agnostic C library for implementing tap dance functionality in keyboard firmware.

## Features

- 🎯 **Platform Independent**: Core algorithm separated from platform-specific code
- 🧪 **Well Tested**: Comprehensive test suite using Google Test
- 🔧 **Easy Integration**: Simple platform interface for porting to different firmware
- ⚡ **Performance**: Efficient implementation suitable for embedded systems
- 📚 **Documented**: Clear API documentation and usage examples

## Quick Start

### Building

```bash
# Configure the build
cmake -B build -S .

# Build the library and tests
cmake --build build

# Run tests
cd build && ctest
```

### Integration

To integrate with your keyboard firmware:

1. Implement the platform interface functions in `platform_interface.h`
2. Link with the tap dance engine library
3. Configure your tap dance behaviors
4. Call the pipeline in your key processing loop

See `examples/qmk/` for a complete QMK integration example.

## Architecture

```
┌─────────────────┐
│   Your Keymap   │
├─────────────────┤
│ Platform Layer  │  ← Implement platform_interface.h
├─────────────────┤
│ Tap Dance Core  │  ← This library
└─────────────────┘
```

## Platform Interface

The library requires implementation of these platform functions:

- **Key Operations**: `platform_send_key()`, `platform_register_key()`, etc.
- **Layer Operations**: `platform_layer_on()`, `platform_layer_off()`, etc.
- **Time Operations**: `platform_timer_read()`, `platform_timer_elapsed()`
- **Deferred Execution**: `platform_defer_exec()`, `platform_cancel_deferred_exec()`
- **Memory Operations**: `platform_malloc()`, `platform_free()`

## Development

### Requirements

- CMake 3.14+
- C11 compiler
- C++17 compiler (for tests)

### VS Code Setup

This project is configured for VS Code with clangd:

1. Install recommended extensions
2. Configure and build: `Ctrl+Shift+P` → "CMake: Configure"
3. Build: `Ctrl+Shift+P` → "CMake: Build"
4. Run tests: `Ctrl+Shift+P` → "CMake: Run Tests"

## License

[Add your license here]
