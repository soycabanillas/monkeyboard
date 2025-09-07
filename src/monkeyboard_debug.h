#pragma once

#include "monkeyboard_platform_detection.h"

#if defined(FRAMEWORK_UNIT_TEST)
    #if !defined(MONKEYBOARD_DEBUG)
        #define MONKEYBOARD_DEBUG
    #endif

    #include <stdio.h>

    #define DEBUG_PRINT_NL() printf("\n")
    #define DEBUG_PRINT(fmt, ...) printf("" fmt "\n", ##__VA_ARGS__)
    #define DEBUG_PRINT_RAW(fmt, ...) printf("" fmt "", ##__VA_ARGS__)
    #define DEBUG_PRINT_PREFIX(prefix, fmt, ...) printf(prefix fmt "\n", ##__VA_ARGS__)
    #define DEBUG_PRINT_RAW_PREFIX(prefix, fmt, ...) printf(prefix fmt "", ##__VA_ARGS__)
    #define DEBUG_PRINT_ERROR(fmt, ...) printf("# ERROR #: " fmt "\n", ##__VA_ARGS__)
#elif defined(FRAMEWORK_QMK) && defined(CONSOLE_ENABLE)
    #if !defined(MONKEYBOARD_DEBUG)
        #define MONKEYBOARD_DEBUG
    #endif

    #include "print.h"

    #define DEBUG_PRINT_NL() uprintf("\n")
    #define DEBUG_PRINT(fmt, ...) uprintf("" fmt "\n", ##__VA_ARGS__)
    #define DEBUG_PRINT_RAW(fmt, ...) uprintf("" fmt "", ##__VA_ARGS__)
    #define DEBUG_PRINT_PREFIX(prefix, fmt, ...) uprintf(prefix fmt "\n", ##__VA_ARGS__)
    #define DEBUG_PRINT_RAW_PREFIX(prefix, fmt, ...) uprintf(prefix fmt "", ##__VA_ARGS__)
    #define DEBUG_PRINT_ERROR(fmt, ...) uprintf("# ERROR #: " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT_NL() ((void)0)
    #define DEBUG_PRINT(fmt, ...) ((void)0)
    #define DEBUG_PRINT_RAW(fmt, ...) ((void)0)
    #define DEBUG_PRINT_PREFIX(prefix, fmt, ...) ((void)0)
    #define DEBUG_PRINT_RAW_PREFIX(prefix, fmt, ...) ((void)0)
    #define DEBUG_PRINT_ERROR(fmt, ...) ((void)0)
#endif
