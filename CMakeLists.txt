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

# Create core headers list
set(CORE_HEADERS
    src/key_buffer.h
    src/pipeline_executor.h
    src/pipeline_tap_dance.h
    src/pipeline_tap_dance_initializer.h
    src/platform_interface.h
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
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Wno-unused-parameter>
)

# Add debug symbols for development
target_compile_options(tap_dance_engine PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
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

    # Create platform mock library
    add_library(platform_mock tests/platform_mock.cpp)
    target_include_directories(platform_mock
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_compile_definitions(platform_mock PRIVATE UNIT_TESTING)

    # Define UNIT_TESTING for the core library when building tests
    target_compile_definitions(tap_dance_engine PRIVATE UNIT_TESTING)

    # Link the core library with the platform mock for tests
    target_link_libraries(tap_dance_engine PRIVATE platform_mock)

    # Find test files (excluding platform_mock.cpp since it's now a separate library)
    file(GLOB_RECURSE TEST_SOURCES
        "tests/test_tap_dance_basic_tap*.cpp"
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
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Wno-unused-parameter>
    )

    # Enable testing
    enable_testing()
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install targets
install(TARGETS tap_dance_engine
    EXPORT tap_dance_engine_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${CORE_HEADERS}
    DESTINATION include
)
