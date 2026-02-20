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

TEST_URL_PARSER(1, "/sdfsd/123/sdfsd/", "", "", "/sdfsd/123/sdfsd/");
TEST_URL_PARSER(2, "/", "", "", "/");
TEST_URL_PARSER(3, "https://archlinux.org", "https", "archlinux.org", "");
TEST_URL_PARSER(4, "http://archlinux.org/some/path/long#SDF", "http", "archlinux.org", "/some/path/long");
TEST_URL_PARSER(5, "http://sub2.sub-sdf.archlinux.org", "http", "sub2.sub-sdf.archlinux.org", "");
TEST_URL_PARSER(6, "http://archlinux.org#SDF", "http", "archlinux.org", "");
TEST_URL_PARSER(7, "archlinux.org?SDF", "", "archlinux.org", "");
TEST_URL_PARSER(8, "0.0.0.0:1234/path/234", "", "0.0.0.0:1234", "/path/234");
TEST_URL_PARSER(9, "http://archlinux.org/\n", "http", "archlinux.org", "/");
TEST_URL_PARSER(10, "http://archlinux.org/\r", "http", "archlinux.org", "/");

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
}
