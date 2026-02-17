#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "./tests-common.h"
#include "../url_parser.h"

TEST(url_parser_1) {
    struct UrlParts parts;
    parse_url("/sdfsd/123/sdfsd/", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "");
    ASSERT_EQUAL_STR(parts.host, "");
    ASSERT_EQUAL_STR(parts.path, "/sdfsd/123/sdfsd/");
}

TEST(url_parser_2) {
    struct UrlParts parts;
    parse_url("/", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "");
    ASSERT_EQUAL_STR(parts.host, "");
    ASSERT_EQUAL_STR(parts.path, "/");
}

TEST(url_parser_3) {
    struct UrlParts parts;
    parse_url("https://archlinux.org", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "https");
    ASSERT_EQUAL_STR(parts.host, "archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "");
}

TEST(url_parser_4) {
    struct UrlParts parts;
    parse_url("https://archlinux.org/", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "https");
    ASSERT_EQUAL_STR(parts.host, "archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "/");
}

TEST(url_parser_5) {
    struct UrlParts parts;
    parse_url("http://archlinux.org/some/path/long#SDF", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "http");
    ASSERT_EQUAL_STR(parts.host, "archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "/some/path/long");
}

TEST(url_parser_6) {
    struct UrlParts parts;
    parse_url("http://sub2.sub-sdf.archlinux.org", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "http");
    ASSERT_EQUAL_STR(parts.host, "sub2.sub-sdf.archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "");
}

TEST(url_parser_7) {
    struct UrlParts parts;
    parse_url("http://archlinux.org#SDF", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "http");
    ASSERT_EQUAL_STR(parts.host, "archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "");
}

TEST(url_parser_8) {
    struct UrlParts parts;
    parse_url("http://archlinux.org#SDF", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "http");
    ASSERT_EQUAL_STR(parts.host, "archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "");
}

TEST(url_parser_9) {
    struct UrlParts parts;
    parse_url("archlinux.org?SDF", &parts);

    ASSERT_EQUAL_STR(parts.protocol, "");
    ASSERT_EQUAL_STR(parts.host, "archlinux.org");
    ASSERT_EQUAL_STR(parts.path, "");
}

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
}
