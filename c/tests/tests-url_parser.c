#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "./tests-common.h"
#include "../url_parser.h"


#define TEST_URL_PARSER(name, url, protocol_expected, host_expected, path_expected) \
    TEST(url_parser_##name) { \
        struct UrlParts parts; \
        parse_url(url, &parts); \
        ASSERT_EQUAL_STR(parts.protocol, protocol_expected); \
        ASSERT_EQUAL_STR(parts.host, host_expected); \
        ASSERT_EQUAL_STR(parts.path, path_expected); \
    } \


TEST(url_parser_1) {
    char* url = "https://archlinux.org/asdf/we32";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 5);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_2) {
    char* url = "https://archlinux.org/asdf/we32\n\r";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 5);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_3) {
    char* url = "https://archlinux.org/asdf/we32#ASDF";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 5);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_4) {
    char* url = "archlinux.org";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, url);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_start, NULL);
    ASSERT_EQUAL_INT(ptrs.path_end, NULL);
}

TEST(url_parser_5) {
    char* url = "archlinux.org/path/123";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, url);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 22);
}

TEST(url_parser_6) {
    char* url = "https://archlinux.org";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 5);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, NULL);
    ASSERT_EQUAL_INT(ptrs.path_end, NULL);
}


TEST(url_parser_7) {
    char* url = "/asdf/we32/";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, NULL);
    ASSERT_EQUAL_INT(ptrs.host_end, NULL);
    ASSERT_EQUAL_INT(ptrs.path_start, url);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 11);
}

TEST_URL_PARSER(8, "/sdfsd/123/sdfsd/", "", "", "/sdfsd/123/sdfsd/");
TEST_URL_PARSER(9, "/", "", "", "/");
TEST_URL_PARSER(10, "https://archlinux.org", "https", "archlinux.org", "");
TEST_URL_PARSER(11, "http://archlinux.org/some/path/long#SDF", "http", "archlinux.org", "/some/path/long");
TEST_URL_PARSER(12, "http://sub2.sub-sdf.archlinux.org", "http", "sub2.sub-sdf.archlinux.org", "");
TEST_URL_PARSER(13, "http://archlinux.org#SDF", "http", "archlinux.org", "");
TEST_URL_PARSER(14, "archlinux.org?SDF", "", "archlinux.org", "");
TEST_URL_PARSER(15, "0.0.0.0:1234/path/234", "", "0.0.0.0:1234", "/path/234");
TEST_URL_PARSER(16, "http://archlinux.org/\n", "http", "archlinux.org", "/");
TEST_URL_PARSER(17, "http://archlinux.org/\r", "http", "archlinux.org", "/");

int main() {
    RUN_TEST(url_parser_1);
    RUN_TEST(url_parser_2);
    RUN_TEST(url_parser_3);
    RUN_TEST(url_parser_4);
    RUN_TEST(url_parser_5);
    RUN_TEST(url_parser_6);
    RUN_TEST(url_parser_7);
    RUN_TEST(url_parser_8);
    RUN_TEST(url_parser_9);
    RUN_TEST(url_parser_10);
    RUN_TEST(url_parser_11);
    RUN_TEST(url_parser_12);
    RUN_TEST(url_parser_13);
    RUN_TEST(url_parser_14);
    RUN_TEST(url_parser_15);
    RUN_TEST(url_parser_16);
    RUN_TEST(url_parser_17);
}
