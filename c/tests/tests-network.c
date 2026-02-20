// #include <stdio.h>
// #include <string.h>
#include <assert.h>

#include "./tests-common.h"
#include "../network.h"

// #define TEST_NETWORK(name, url, protocol_expected, host_expected, path_expected) \
//     TEST(network_##name) { \
//         find_http_content_start(char* data) {
//         struct UrlParts parts; \
//         parse_url(url, &parts); \
//         ASSERT_EQUAL_STR(parts.protocol, protocol_expected); \
//         ASSERT_EQUAL_STR(parts.host, host_expected); \
//         ASSERT_EQUAL_STR(parts.path, path_expected); \
//     } \

TEST(network_1) {
    char* data = "Location\r\n\r\nsdf";
    char* result = find_http_content_start(data);
    char* expected = "sdf";

    ASSERT_EQUAL_STR(result, expected);
}

int main() {
    RUN_TEST(network_1);
}
