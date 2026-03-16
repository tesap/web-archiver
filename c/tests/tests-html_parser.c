#include "./tests-common.h"
#include "../html_parser.h"
#include "../util.h"

#include <assert.h>

struct HrefAttrs {
    char* value;
    HrefType type;
};

struct HrefAttrs test1_ha_list[] = {
    {"/static/archlinux_common_style/navbar.css", HREF_TYPE_STYLE},
    {"/static/archweb.css", HREF_TYPE_STYLE},
    {"/static/archlinux_common_style/favicon.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/favicon.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-57x57.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-72x72.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-114x114.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-144x144.png", HREF_TYPE_IMG},
    {"/opensearch/packages/", HREF_TYPE_UNKNOWN},
    {"/feeds/news/", HREF_TYPE_UNKNOWN},
    {"/feeds/packages/", HREF_TYPE_UNKNOWN},
    {"/static/homepage.js", HREF_TYPE_SCRIPT},
    // {"https://fosstodon.org/@archlinux", HREF_TYPE_HTML}, // Not appropriate
    {"/", HREF_TYPE_HTML},
    {"/", HREF_TYPE_HTML},
    {"/packages/", HREF_TYPE_HTML},
    {"https://bbs.archlinux.org/", HREF_TYPE_HTML},
    {"https://wiki.archlinux.org/", HREF_TYPE_HTML},
    {"https://gitlab.archlinux.org/archlinux", HREF_TYPE_HTML},
    {"https://security.archlinux.org/", HREF_TYPE_HTML},
    {"https://aur.archlinux.org/", HREF_TYPE_HTML},
    {"/download/", HREF_TYPE_HTML},
    {"https://aur.archlinux.org/", HREF_TYPE_HTML},
    // Other hrefs are omitted
};
int test1_ha_list_size = sizeof(test1_ha_list) / sizeof(struct HrefAttrs);

void html_parser_1_callback(const char* found_url, HrefType ht, void* ctx) {
    int* href_index = (int*)ctx;
    if (*href_index >= test1_ha_list_size) {
        return;
    }

    printf("=== (%d) URL: %s\n", *href_index, found_url);
    // print_href_type(ht);

    ASSERT_EQUAL_STR(found_url, test1_ha_list[*href_index].value);
    ASSERT_EQUAL_INT(ht, test1_ha_list[*href_index].type);
    (*href_index)++;
}

TEST(html_parser_1) {
    char* path = "./out/files/archlinux.org.html";
    char* buff;
    int size = read_file(path, &buff);

    int href_index = 0;
    search_html_hrefs(buff, size, html_parser_1_callback, &href_index);
}

struct HrefAttrs test2_ha_list[] = {
    {"HREF1", HREF_TYPE_HTML},
    {"HREF2", HREF_TYPE_HTML},
};
int test2_ha_list_size = sizeof(test2_ha_list) / sizeof(struct HrefAttrs);

void html_parser_2_callback(const char* found_url, HrefType ht, void* ctx) {
    int* href_index = (int*)ctx;
    printf("=== (%d) URL: %s\n", *href_index, found_url);

    ASSERT_EQUAL_STR(found_url, test2_ha_list[*href_index].value);
    ASSERT_EQUAL_INT(ht, test2_ha_list[*href_index].type);
    (*href_index)++;
}

TEST(html_parser_2) {
    char* buff = "<a href=\"HREF1\"/><a href=\"HREF2\"/>";

    int href_index = 0;
    search_html_hrefs(buff, strlen(buff), html_parser_2_callback, &href_index);
    ASSERT_EQUAL_INT(href_index, test2_ha_list_size);
}

TEST(html_parser_3) { ASSERT_EQUAL_INT(href_type("https://aur.archlinux.org/", NULL, "a"), HREF_TYPE_HTML); }
TEST(html_parser_4) { ASSERT_EQUAL_INT(href_type("/feed/packages/", NULL, "a"), HREF_TYPE_HTML); }
TEST(html_parser_5) { ASSERT_EQUAL_INT(href_type("#title-anything", NULL, "a"), HREF_TYPE_HTML); }
TEST(html_parser_6) { ASSERT_EQUAL_INT(href_type("magnet:?xt=urn:btih:a4373c326657898d0c588c3ff892a0fac97ffa20&amp;dn=archlinux-2026.03.01-x86_64.iso", NULL, "a"), HREF_TYPE_UNKNOWN); }
TEST(html_parser_7) { ASSERT_EQUAL_INT(href_type("mailto:jvinet@zeroflux.org", NULL, "a"), HREF_TYPE_UNKNOWN); }

int main() {
    RUN_TEST(html_parser_1);
    RUN_TEST(html_parser_2);
    RUN_TEST(html_parser_3);
    RUN_TEST(html_parser_4);
    RUN_TEST(html_parser_5);
    RUN_TEST(html_parser_6);
}
