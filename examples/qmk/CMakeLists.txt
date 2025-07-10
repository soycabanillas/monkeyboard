# QMK integration example

# This is just a placeholder - the actual QMK integration
# would be in the QMK userspace repository
add_library(qmk_example INTERFACE)

# Add documentation about QMK integration
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    COPYONLY
)
