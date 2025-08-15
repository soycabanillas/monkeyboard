#pragma once


// Platform-specific keycode definitions - only when needed
#if defined(QMK_KEYBOARD) || defined(QUANTUM_H)
    #define FRAMEWORK_QMK
#elif defined(CONFIG_ZMK) || defined(__ZEPHYR__)
    #define FRAMEWORK_ZMK
#elif defined(UNIT_TESTING)
    #define FRAMEWORK_UNIT_TEST
#endif
