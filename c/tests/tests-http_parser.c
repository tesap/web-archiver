#include "./tests-common.h"
#include "../http_parser.h"
#include "../util.h"

#define TEST_HTTP_PARSER_GET_CONTENT_START(name, data, result_expected) \
    TEST(http_parser_##name) { \
        const char* result = get_content_start(data); \
        if (!result_expected) { \
            ASSERT_EQUAL_INT(result, result_expected); \
        } else { \
            ASSERT_EQUAL_STR(result, result_expected); \
        } \
    } \

#define TEST_HTTP_PARSER_GET_CONTENT_START_FILE(name, path, result_expected, n_compare) \
    TEST(http_parser_##name) { \
        char* buff; \
        read_file(path, &buff); \
        const char* start_ptr = get_content_start(buff); \
        ASSERT_EQUAL_STRN(start_ptr, result_expected, n_compare); \
    } \

#define TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(name, path, url, e1) \
    TEST(http_parser_##name) { \
        char* buff; \
        read_file(path, &buff); \
        char result[256]; \
        int ok = get_location_header(buff, url, result); \
        ASSERT_EQUAL_INT(ok, 0); \
        ASSERT_EQUAL_STR(result, e1); \
    } \

TEST_HTTP_PARSER_GET_CONTENT_START(1, "Location\r\n\r\nTEXT", "TEXT");
TEST_HTTP_PARSER_GET_CONTENT_START(2, "TEXT", NULL);
TEST_HTTP_PARSER_GET_CONTENT_START_FILE(3, "./out/files/archlinux.org.http", "<html>", 6);
TEST_HTTP_PARSER_GET_CONTENT_START_FILE(4, "./out/files/wiki.archlinux.org.http", "", 1);
TEST_HTTP_PARSER_GET_CONTENT_START_FILE(5, "./out/files/rebuildworld.net.http", "<?xml ", 6);
TEST_HTTP_PARSER_GET_CONTENT_START_FILE(6, "./out/files/archlinux.org-mirrors.http", "<!DOCT", 6);
TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(7, "./out/files/wiki.archlinux.org.http", "wiki.archlinux.org", "https://wiki.archlinux.org/title/Main_page");
TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(8, "./out/files/archlinux.org.http", "archlinux.org", "https://archlinux.org/");
TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(9, "./out/files/lists.archlinux.org.http", "https://lists.archlinux.org", "https://lists.archlinux.org/mailman3/lists/");

int main() {
    RUN_TEST(http_parser_1);
    RUN_TEST(http_parser_2);
    RUN_TEST(http_parser_3);
    RUN_TEST(http_parser_4);
    RUN_TEST(http_parser_5);
    RUN_TEST(http_parser_6);
    RUN_TEST(http_parser_7);
    RUN_TEST(http_parser_8);
}
