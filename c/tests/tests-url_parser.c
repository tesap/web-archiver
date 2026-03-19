#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "./tests-common.h"
#include "../url_parser.h"


#define TEST_PARSE_PARTS(name, url, protocol_expected, host_expected, path_expected) \
    TEST(url_parser_##name) { \
        struct UrlParts parts; \
        parse_url_parts(url, &parts); \
        ASSERT_EQUAL_STR(parts.protocol, protocol_expected); \
        ASSERT_EQUAL_STR(parts.host, host_expected); \
        ASSERT_EQUAL_STR(parts.path, path_expected); \
    } \

#define TEST_PARSE_PATHS(name, url, dir_expected, path_expected) \
    TEST(url_parser_##name) { \
        struct UrlPaths paths; \
        parse_url_paths(url, &paths); \
        ASSERT_EQUAL_STR(paths.dir_path, dir_expected); \
        ASSERT_EQUAL_STR(paths.file_path, path_expected); \
    } \

TEST(url_parser_1) {
    char* url = "https://archlinux.org/asdf/we32";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_2) {
    char* url = "https://archlinux.org/asdf/we32\n\r";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_3) {
    char* url = "https://archlinux.org/asdf/we32#ASDF";
    struct UrlPtrs ptrs = get_url_pointers(url);
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
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
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
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

TEST_PARSE_PARTS(8, "/sdfsd/123/sdfsd/", "", "", "/sdfsd/123/sdfsd/");
TEST_PARSE_PARTS(9, "/", "", "", "/");
TEST_PARSE_PARTS(10, "https://archlinux.org", "https://", "archlinux.org", "");
TEST_PARSE_PARTS(10_2, "https://archlinux.org/", "https://", "archlinux.org", "/");
TEST_PARSE_PARTS(11, "http://archlinux.org/some/path/long#SDF", "http://", "archlinux.org", "/some/path/long");
TEST_PARSE_PARTS(12, "http://sub2.sub-sdf.archlinux.org", "http://", "sub2.sub-sdf.archlinux.org", "");
TEST_PARSE_PARTS(13, "http://archlinux.org#SDF", "http://", "archlinux.org", "");
TEST_PARSE_PARTS(14, "archlinux.org?SDF", "", "archlinux.org", "");
// TODO Fix logic for port if needed
// TEST_PARSE_PARTS(15, "0.0.0.0:1234/path/234", "", "0.0.0.0:1234", "/path/234");
TEST_PARSE_PARTS(16, "http://archlinux.org/\n", "http://", "archlinux.org", "/");
TEST_PARSE_PARTS(17, "http://archlinux.org/\r", "http://", "archlinux.org", "/");

TEST_PARSE_PATHS(18, "https://archlinux.org/releases/downloads",    "archlinux.org/releases/downloads", "archlinux.org/releases/downloads/index.html");
TEST_PARSE_PATHS(19, "https://archlinux.org/releases/downloads/",   "archlinux.org/releases/downloads", "archlinux.org/releases/downloads/index.html");
TEST_PARSE_PATHS(20, "https://archlinux.org/releases/page.html",    "archlinux.org/releases",           "archlinux.org/releases/page.html");
TEST_PARSE_PATHS(21, "https://archlinux.org/",                      "archlinux.org",                    "archlinux.org/index.html");
TEST_PARSE_PATHS(22, "https://archlinux.org",                       "archlinux.org",                    "archlinux.org/index.html");
TEST_PARSE_PATHS(23, "https://archlinux.org/packages/webkitgtk-6.0",      "archlinux.org/packages/webkitgtk-6.0",        "archlinux.org/packages/webkitgtk-6.0/index.html");
TEST_PARSE_PATHS(24, "archlinux.org/static/archweb.css",      "archlinux.org/static",        "archlinux.org/static/archweb.css");


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
    RUN_TEST(url_parser_10_2);
    RUN_TEST(url_parser_11);
    RUN_TEST(url_parser_12);
    RUN_TEST(url_parser_13);
    RUN_TEST(url_parser_14);
    // RUN_TEST(url_parser_15);
    RUN_TEST(url_parser_16);
    RUN_TEST(url_parser_17);
    RUN_TEST(url_parser_18);
    RUN_TEST(url_parser_19);
    RUN_TEST(url_parser_20);
    RUN_TEST(url_parser_21);
    RUN_TEST(url_parser_22);
    RUN_TEST(url_parser_23);
    RUN_TEST(url_parser_24);
}
