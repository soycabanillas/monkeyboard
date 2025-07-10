<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Tap Dance Engine - Custom Instructions

This is a platform-agnostic C/C++ library for implementing tap dance functionality in keyboard firmware.

## Project Guidelines

- **Language**: C for core library, C++ for tests
- **Build System**: CMake
- **Testing**: Google Test framework
- **Code Style**: Follow consistent C naming conventions (snake_case)
- **Platform Independence**: Keep core algorithm separate from platform-specific implementations
- **Header Guards**: Use `#pragma once` for all headers
- **Documentation**: Include clear function documentation and usage examples

## Architecture Principles

- Core tap dance logic should be platform-agnostic
- Use a thin platform interface layer for integration
- Maintain separation between algorithm and platform concerns
- Prefer composition over inheritance for behavior configuration
- Keep memory management explicit and controlled

## Testing Strategy

- Unit tests for all core functionality
- Mock platform interface for isolated testing
- Integration tests for complete scenarios
- Performance tests for timing-critical paths
