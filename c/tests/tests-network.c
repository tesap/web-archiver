#include "./tests-common.h"
#include "../network.h"
#include "../util.h"

#define TEST_NETWORK_FIND_CONTENT_START(name, data, result_expected) \
    TEST(network_##name) { \
        const char* result = find_content_start(data); \
        ASSERT_EQUAL_STR(result, result_expected); \
    } \

#define TEST_NETWORK_FIND_CONTENT_START_FILE(name, path, result_expected, n_compare) \
    TEST(network_##name) { \
        char* buff; \
        read_file(path, &buff); \
        const char* start_ptr = find_content_start(buff); \
        ASSERT_EQUAL_STRN(start_ptr, result_expected, n_compare); \
    } \

TEST_NETWORK_FIND_CONTENT_START(1, "Location\r\n\r\nTEXT", "TEXT");
TEST_NETWORK_FIND_CONTENT_START_FILE(2, "./out/files/archlinux.org.http", "<html>", 6);
TEST_NETWORK_FIND_CONTENT_START_FILE(3, "./out/files/rebuildworld.net.http", "<?xml ", 6);

int main() {
    RUN_TEST(network_1);
    RUN_TEST(network_2);
    RUN_TEST(network_3);
}
