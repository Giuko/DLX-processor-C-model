#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include "../inc/utils.h"

// The do while is performed in order to maintain if-else oder
#define ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            printf( COLOR_RED "FAIL: %s (line %d): %s\n" COLOR_RESET, __FILE__, __LINE__, msg); \
            exit(1); \
        } else { \
            printf( COLOR_HIGHLIGHT "PASS: %s\n" COLOR_RESET, msg); \
        } \
    } while(0)


void compare_expected_values(void *handle);

#endif
