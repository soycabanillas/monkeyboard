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
option(BUILD_EXAMPLES "Build examples" ON)

# Core library - platform-agnostic files (only core tap dance functionality)
set(CORE_SOURCES
    src/key_buffer.c
    src/pipeline_executor.c
    src/pipeline_tap_dance.c
    src/pipeline_tap_dance_initializer.c
)

add_library(tap_dance_engine ${CORE_SOURCES})
target_include_directories(tap_dance_engine PUBLIC src)

# Set compiler flags for better debugging and warnings
target_compile_options(tap_dance_engine PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
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

    # Test executable - only include our simple test for now
    add_executable(tests
        tests/platform_mock.cpp
        tests/test_tap_dance_simple.cpp
    )
    # file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    # add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests tap_dance_engine gtest_main)
    target_include_directories(tests PRIVATE src tests)

    # Enable testing
    enable_testing()
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
