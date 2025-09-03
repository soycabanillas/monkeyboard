cmake_minimum_required(VERSION 3.14)
project(tap_dance_engine VERSION 1.0.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build tests" ON)

# Core library - platform-agnostic files (only core tap dance functionality)
set(CORE_SOURCES
    src/key_event_buffer.c
    src/key_press_buffer.c
    src/key_virtual_buffer.c
    src/monkeyboard_deferred_callbacks.c
    src/monkeyboard_layer_manager.c
    src/monkeyboard_time_manager.c
    src/pipeline_combo.c
    src/pipeline_combo_initializer.c
    src/pipeline_executor.c
    src/pipeline_tap_dance.c
    src/pipeline_tap_dance_initializer.c
    src/pipeline_oneshot_modifier.c
    src/pipeline_oneshot_modifier_initializer.c
    src/pipeline_key_replacer.c
    src/pipeline_key_replacer_initializer.c
    src/platform_layout.c
    src/platform_types.c
)

# Create core headers list
set(CORE_HEADERS
    src/key_event_buffer.h
    src/key_press_buffer.h
    src/key_virtual_buffer.h
    src/monkeyboard_deferred_callbacks.h
    src/monkeyboard_layer_manager.h
    src/monkeyboard_time_manager.h
    src/pipeline_combo.h
    src/pipeline_combo_initializer.h
    src/pipeline_executor.h
    src/pipeline_tap_dance.h
    src/pipeline_tap_dance_initializer.h
    src/pipeline_oneshot_modifier.h
    src/pipeline_oneshot_modifier_initializer.h
    src/pipeline_key_replacer.h
    src/pipeline_key_replacer_initializer.h
    src/platform_interface.h
    src/platform_layout.h
    src/platform_types.h
)

add_library(tap_dance_engine ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(tap_dance_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Set compiler flags for better debugging and warnings
target_compile_options(tap_dance_engine PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wpedantic -Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-c99-designator -Wno-vla-extension>
)

# Add debug symbols for development
target_compile_options(tap_dance_engine PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

# Add DEBUG definition for debug builds
target_compile_definitions(tap_dance_engine PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

# Tests
if(BUILD_TESTS)
    # Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    # Define UNIT_TESTING for all test-related targets
    add_compile_definitions(UNIT_TESTING)

    # Create platform mock library
    # Testing
    set(TEST_SOURCES
        tests/keyboard_simulator.cpp
        tests/platform_mock.cpp
    )

    # Create core headers list
    set(TEST_HEADERS
        tests/keyboard_simulator.hpp
    )
    add_library(platform_mock ${TEST_SOURCES} ${TEST_HEADERS})
    target_include_directories(platform_mock
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    # Link the platform mock with the core library and Google Test
    target_link_libraries(platform_mock PRIVATE tap_dance_engine gtest)

    # Find test files (excluding platform_mock.cpp since it's now a separate library)
    file(GLOB_RECURSE TEST_SOURCES
        "tests/test_*.cpp"
    )

    # Test executable - include all test files
    add_executable(tests ${TEST_SOURCES})

    target_link_libraries(tests
        PRIVATE
            tap_dance_engine
            platform_mock
            gtest_main
    )

    target_include_directories(tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    # Explicitly set C++ standard for tests and separate from C library flags
    set_target_properties(tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

    # Set C++ compile options for tests (separate from C library)
    target_compile_options(tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-c99-designator -Wno-vla-extension -Wno-gnu-statement-expression>
    )

    # Enable testing
    enable_testing()
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()
