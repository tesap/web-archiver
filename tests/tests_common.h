#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "util.h"

#define TESTS_MAX_COUNT 200

typedef void (*test_func_t)(void);

static char tests_names[TESTS_MAX_COUNT][1024];
static test_func_t tests_ptrs[TESTS_MAX_COUNT];
static int test_func_i = 0;

#define TEST(name) \
    void test_##name(); \
    __attribute__((constructor)) \
    static void register_test_##name() { \
        assert(test_func_i < TESTS_MAX_COUNT); \
        strcpy(tests_names[test_func_i], #name); \
        tests_ptrs[test_func_i] = test_##name; \
        test_func_i++; \
    } \
    void test_##name() 

void run_tests() {
    for (int i = 0; i < TESTS_MAX_COUNT; i++) {
        if (tests_ptrs[i] != NULL) {
            printf("--> (%d) %s\t", i, tests_names[i]);
            fflush(0);
            tests_ptrs[i]();
            printf("✓\n");
        }
    }
}
    
#define ASSERT_EQUAL_STR(actual, expected) do { \
    if (strcmp(actual, expected) != 0) { \
        printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: \"%s\"\n\tbut got:  \"%s\"\n", __FILE__, __LINE__, __func__, expected, actual); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQUAL_VEC(actual, expected) do { \
    if (!vec_eq2(actual, expected)) { \
        printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: \"%.*s\"\n\tbut got:  \"%.*s\"\n", __FILE__, __LINE__, __func__, expected.size, expected.ptr, actual.size, actual.ptr); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQUAL_STRN(actual, expected, n) do { \
    if (strncmp(actual, expected, n) != 0) { \
        printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: \"%.*s\"\n\tbut got:  \"%.*s\"\n", __FILE__, __LINE__, __func__, n, expected, n, actual); \
        exit(1); \
    } \
} while(0)

#define STRN_LARGE_SHOW_SYMBOLS 50
#define ASSERT_EQUAL_STRN_LARGE(actual, expected, n) do { \
    int i = 0; \
    while (i < n) { \
        if (actual[i] != expected[i]) { \
            printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: ...\"%.*s\"\n\tbut got:  ...\"%.*s\"\n", __FILE__, __LINE__, __func__, STRN_LARGE_SHOW_SYMBOLS, expected + i - 10, STRN_LARGE_SHOW_SYMBOLS, actual + i - 10); \
            exit(1); \
        } \
        i++; \
    } \
} while(0)

#define ASSERT_EQUAL_INT(actual, expected) do { \
    if (actual != expected) { \
        printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: \"%d\"\n\tbut got:  \"%d\"\n", __FILE__, __LINE__, __func__, expected, actual); \
        exit(1); \
    } \
} while(0)

