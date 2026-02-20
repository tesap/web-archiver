#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s: ", #name); \
    test_##name(); \
    printf("✓\n", #name); \
} while(0)

#define ASSERT_EQUAL_STR(actual, expected) do { \
    if (strcmp(actual, expected) != 0) { \
        printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: \"%s\"\n\tbut got:  \"%s\"\n", __FILE__, __LINE__, __func__, expected, actual); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQUAL_INT(actual, expected) do { \
    if (actual != expected) { \
        printf("ASSERTION FAILED (%s:%d, %s)\n\tExpected: \"%d\"\n\tbut got:  \"%d\"\n", __FILE__, __LINE__, __func__, expected, actual); \
        exit(1); \
    } \
} while(0)
