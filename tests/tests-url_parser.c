#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "./tests-common.h"
#include "../url_parser.h"
#include "../util.h"


#define TEST_PARSE_URL_PARTS(name, url, protocol_expected, host_expected, path_expected) \
    TEST(url_parser_##name) { \
        struct UrlParts parts; \
        parse_url_parts(url, &parts); \
        ASSERT_EQUAL_STR(parts.protocol, protocol_expected); \
        ASSERT_EQUAL_STR(parts.host, host_expected); \
        ASSERT_EQUAL_STR(parts.path, path_expected); \
    } \

#define TEST_URL_TO_FILEPATH(name, is_file, url, path_expected, dir_len_expected) \
    TEST(url_parser_##name) { \
        char file_path[MAX_URL_LENGTH]; \
        struct vec _v = {file_path, 0}; \
        url_to_filepath(vec_wrap(url), is_file, &_v); \
        file_path[_v.size] = '\0'; \
        int dir_len = dirname_len(file_path); \
        ASSERT_EQUAL_STR(file_path, path_expected); \
        ASSERT_EQUAL_INT(dir_len, dir_len_expected); \
    } \

#define TEST_GET_RELPATH(name, path_from, path_to, expected_result) \
    TEST(url_parser_##name) { \
        char _result[1024]; \
        struct vec result = {_result, 0}; \
        get_relpath(vec_wrap(path_from), vec_wrap(path_to), &result); \
        result.ptr[result.size] = '\0'; \
        ASSERT_EQUAL_STR(result.ptr, expected_result); \
    } \

#define TEST_DIRNAME_LEN(name, input, expected_result) \
    TEST(url_parser_##name) { \
        int res = dirname_len(input); \
        ASSERT_EQUAL_INT(res, expected_result); \
    } \

#define TEST_DIR_COUNT(name, input, expected_result) \
    TEST(url_parser_##name) { \
        int res = get_dir_count(vec_wrap(input)); \
        ASSERT_EQUAL_INT(res, expected_result); \
    } \

TEST(url_parser_1) {
    char* url = "https://archlinux.org/asdf/we32";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_2) {
    char* url = "https://archlinux.org/asdf/we32\n\r";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_3) {
    char* url = "https://archlinux.org/asdf/we32#ASDF";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser_4) {
    char* url = "archlinux.org";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, url);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_start, NULL);
    ASSERT_EQUAL_INT(ptrs.path_end, NULL);
}

TEST(url_parser_5) {
    char* url = "archlinux.org/path/123";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, url);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 22);
}

TEST(url_parser_6) {
    char* url = "https://archlinux.org";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, NULL);
    ASSERT_EQUAL_INT(ptrs.path_end, NULL);
}


TEST(url_parser_7) {
    char* url = "/asdf/we32/";
    struct UrlPtrs ptrs = get_url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, NULL);
    ASSERT_EQUAL_INT(ptrs.host_end, NULL);
    ASSERT_EQUAL_INT(ptrs.path_start, url);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 11);
}

TEST_PARSE_URL_PARTS(8, "/sdfsd/123/sdfsd/", "", "", "/sdfsd/123/sdfsd/");
TEST_PARSE_URL_PARTS(9, "/", "", "", "/");
TEST_PARSE_URL_PARTS(10, "https://archlinux.org", "https://", "archlinux.org", "");
TEST_PARSE_URL_PARTS(10_2, "https://archlinux.org/", "https://", "archlinux.org", "/");
TEST_PARSE_URL_PARTS(11, "http://archlinux.org/some/path/long#SDF", "http://", "archlinux.org", "/some/path/long");
TEST_PARSE_URL_PARTS(12, "http://sub2.sub-sdf.archlinux.org", "http://", "sub2.sub-sdf.archlinux.org", "");
TEST_PARSE_URL_PARTS(13, "http://archlinux.org#SDF", "http://", "archlinux.org", "");
TEST_PARSE_URL_PARTS(14, "archlinux.org?SDF", "", "archlinux.org", "");
// TODO Fix logic for port if needed
// TEST_PARSE_URL_PARTS(15, "0.0.0.0:1234/path/234", "", "0.0.0.0:1234", "/path/234");
TEST_PARSE_URL_PARTS(16, "http://archlinux.org/\n", "http://", "archlinux.org", "/");
TEST_PARSE_URL_PARTS(17, "http://archlinux.org/\r", "http://", "archlinux.org", "/");

TEST_URL_TO_FILEPATH(18, false, "https://archlinux.org/releases/downloads",     "archlinux.org/releases/downloads/index.html", 33);
TEST_URL_TO_FILEPATH(19, false, "https://archlinux.org/releases/downloads/",    "archlinux.org/releases/downloads/index.html", 33);
// Yes, we want to trfalses as a directories. This is because TODO
TEST_URL_TO_FILEPATH(20, false, "https://archlinux.org/releases/page.html",     "archlinux.org/releases/page.html/index.html", 33);
TEST_URL_TO_FILEPATH(21, false, "https://archlinux.org/",                       "archlinux.org/index.html", 14);
TEST_URL_TO_FILEPATH(22, true,  "https://archlinux.org",                        "archlinux.org", 0);
TEST_URL_TO_FILEPATH(23, false, "archlinux.org",                                "archlinux.org/index.html", 14);
TEST_URL_TO_FILEPATH(24, false, "archlinux.org",                                "archlinux.org/index.html", 14);
TEST_URL_TO_FILEPATH(25, false, "https://archlinux.org/packages/webkitgtk-6.0", "archlinux.org/packages/webkitgtk-6.0/index.html", 37);
TEST_URL_TO_FILEPATH(26, false, "archlinux.org/link.net",                       "archlinux.org/link.net/index.html", 23);
TEST_URL_TO_FILEPATH(27, false, "archlinux.org/link.net/",                      "archlinux.org/link.net/index.html", 23);
TEST_URL_TO_FILEPATH(28, true, "archlinux.org/link.net/",                       "archlinux.org/link.net", 14);
TEST_URL_TO_FILEPATH(29, true, "archlinux.org/static/archweb.css",              "archlinux.org/static/archweb.css", 21);
TEST_URL_TO_FILEPATH(30, true, "archlinux.org/static/script.js",                "archlinux.org/static/script.js", 21);
TEST_URL_TO_FILEPATH(31, true, "archlinux.org/static/archlinux_common_style/apple-touch-icon-144x144.png",                "archlinux.org/static/archlinux_common_style/apple-touch-icon-144x144.png", 44);
TEST_URL_TO_FILEPATH(32, false, "https://www.archlinux.org/path/to/smth/",                "www.archlinux.org/path/to/smth/index.html", 31);
TEST_URL_TO_FILEPATH(33, true, "https://www.archlinux.org/path/to/smth/",                "www.archlinux.org/path/to/smth", 26);

TEST_GET_RELPATH(34, "1/2/3", "4", "../../../4");
TEST_GET_RELPATH(35, "dir1/dir2/one", "dir1/dir2/second", "../../../dir1/dir2/second");
TEST_GET_RELPATH(36, "1/2", "3/4/5/6/", "../../3/4/5/6/");
TEST_GET_RELPATH(37, "1/",  "1/2/3/4",  "../1/2/3/4");
TEST_GET_RELPATH(38, "1/",  "1/2/3/4/", "../1/2/3/4/");
TEST_GET_RELPATH(39, "1",   "1/2/3/4",  "../1/2/3/4");
TEST_GET_RELPATH(40, "1",   "1/2/3/4/", "../1/2/3/4/");
TEST_GET_RELPATH(41, "1/2/3/4", "1", "../../../../1");
TEST_GET_RELPATH(42, "abc/abc", "abc/a12", "../../abc/a12");
TEST_GET_RELPATH(43, "archlinux.org/pages/path/", "archlinux.org/other/index.html", "../../../archlinux.org/other/index.html");
TEST_GET_RELPATH(44, "./archlinux.org/pages/path/", "archlinux.org/other/index.html", "../../../archlinux.org/other/index.html");
TEST_GET_RELPATH(45, "./archlinux.org/feeds/news/", "archlinux.org/feeds/news/", "../../../archlinux.org/feeds/news/");

TEST_DIRNAME_LEN(46, "1/2", 2);
TEST_DIRNAME_LEN(47, "1/2/", 4);
TEST_DIRNAME_LEN(48, "1", 0);
TEST_DIRNAME_LEN(49, "1/", 2);

TEST_DIR_COUNT(50, "1/2/3/", 3);
TEST_DIR_COUNT(51, "1/2/3", 3);
TEST_DIR_COUNT(52, "1/2/3/././", 3);
TEST_DIR_COUNT(53, "1/2/3/../", 2);
TEST_DIR_COUNT(54, "./archlinux.org/pages/path/", 3);


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
    RUN_TEST(url_parser_25);
    RUN_TEST(url_parser_26);
    RUN_TEST(url_parser_27);
    RUN_TEST(url_parser_28);
    RUN_TEST(url_parser_29);
    RUN_TEST(url_parser_30);
    RUN_TEST(url_parser_31);
    RUN_TEST(url_parser_34);
    RUN_TEST(url_parser_35);
    RUN_TEST(url_parser_36);
    RUN_TEST(url_parser_37);
    RUN_TEST(url_parser_38);
    RUN_TEST(url_parser_39);
    RUN_TEST(url_parser_41);
    RUN_TEST(url_parser_42);
    RUN_TEST(url_parser_43);
    RUN_TEST(url_parser_44);
    RUN_TEST(url_parser_45);
    RUN_TEST(url_parser_46);
    RUN_TEST(url_parser_47);
    RUN_TEST(url_parser_48);
    RUN_TEST(url_parser_49);
    RUN_TEST(url_parser_50);
    RUN_TEST(url_parser_51);
    RUN_TEST(url_parser_52);
    RUN_TEST(url_parser_53);
    RUN_TEST(url_parser_54);
}
