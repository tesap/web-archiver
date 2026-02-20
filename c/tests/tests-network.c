// #include <stdio.h>
// #include <string.h>
#include <assert.h>

#include "./tests-common.h"
#include "../network.h"

#define TEST_NETWORK(name, data, result_expected) \
    TEST(network_##name) { \
        char* result = find_http_content_start(data); \
        ASSERT_EQUAL_STR(result, result_expected); \
    } \


TEST_NETWORK(1, "Location\r\n\r\nsdf", "sdf");

int main() {
    RUN_TEST(network_1);
}
